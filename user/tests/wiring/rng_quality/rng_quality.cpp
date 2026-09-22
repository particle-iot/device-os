/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"

#include "rng_hal.h"
#include "spark_wiring_json.h"
#include "system_control.h"

#if HAL_PLATFORM_RTL872X
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <string.h>

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

const size_t MAX_DUMP_WORDS = 16383; // 65532 bytes, the payload limit is 65535 (uint16_t wLength)
const uint32_t REPLAY_MAGIC = 0x5a5a5a5a;

#if HAL_PLATFORM_RTL872X
const char* const backupRamFilePath = "/sys/backup_ram.bin";

const uint32_t retainedSeedMagic = 0x524e4753;
const uint8_t retainedSeedVersion = 1;
const size_t retainedSeedStructSize = 40;
const size_t retainedSeedDataOffset = 8;

} // namespace

extern "C" uintptr_t platform_backup_ram_all_start[];
extern "C" uintptr_t platform_backup_ram_all_end;

namespace {

uint32_t* findRetainedSeed() {
    auto p = reinterpret_cast<uint32_t*>(platform_backup_ram_all_start);
    const auto end = reinterpret_cast<uint32_t*>(&platform_backup_ram_all_end);
    for (; p < end; ++p) {
        if (*p == retainedSeedMagic && reinterpret_cast<const uint8_t*>(p)[4] == retainedSeedVersion) {
            return p;
        }
    }
    return nullptr;
}
#endif

static retained uint32_t previousStream[8];
static retained uint32_t replayState;
static retained uint32_t corruptedSeedStream[8];

int rngDumpRequest(ctrl_request* req) {
    bool command = false;
    size_t size = 1024;
    const auto data = JSONValue::parse(req->request_data, req->request_size);
    CHECK_TRUE(data.isObject(), SYSTEM_ERROR_BAD_DATA);

    JSONObjectIterator it(data);
    while (it.next()) {
        if (it.name() == "c") {
            command = it.value().toString() == "R";
        } else if (it.name() == "n") {
            CHECK_TRUE(it.value().isNumber(), SYSTEM_ERROR_INVALID_ARGUMENT);
            size = it.value().toInt();
        }
    }

    CHECK_TRUE(command, SYSTEM_ERROR_NOT_SUPPORTED);
    CHECK_TRUE(size > 0 && size <= MAX_DUMP_WORDS, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(system_ctrl_alloc_reply_data(req, size * sizeof(uint32_t), nullptr));

    auto output = reinterpret_cast<uint32_t*>(req->reply_data);
    for (size_t i = 0; i < size; ++i) {
        output[i] = HAL_RNG_GetRandomNumber();
    }
    return SYSTEM_ERROR_NONE;
}

} // anonymous namespace

int test_app_ctrl_request_handler(ctrl_request* req) {
    return rngDumpRequest(req);
}

#ifndef PARTICLE_TEST_RUNNER

void ctrl_request_custom_handler(ctrl_request* req) {
    system_ctrl_set_result(req, test_app_ctrl_request_handler(req), nullptr, nullptr, nullptr);
}

#endif // !PARTICLE_TEST_RUNNER

test(RNG_01_not_stuck) {
    const uint32_t first = HAL_RNG_GetRandomNumber();
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (HAL_RNG_GetRandomNumber() == first) {
            ++same;
        }
    }
    assertLessOrEqual(same, 2);
}

test(RNG_02_replay_across_soft_reset) {
    uint32_t stream[8] = {};
    for (auto& value: stream) {
        value = HAL_RNG_GetRandomNumber();
    }

    if (replayState == REPLAY_MAGIC) {
        int same = 0;
        for (size_t i = 0; i < sizeof(stream) / sizeof(stream[0]); ++i) {
            if (stream[i] == previousStream[i]) {
                ++same;
            }
        }
        replayState = 0;
        assertEqual(RESET_REASON_USER, System.resetReason());
        assertLess(same, 2);
        return;
    }

    memcpy(previousStream, stream, sizeof(stream));
    replayState = REPLAY_MAGIC;
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_03_host_side_statistical_analysis) {
}

#if HAL_PLATFORM_RTL872X
test(RNG_04_initial_generation_1) {
    unlink(backupRamFilePath);
    extern uintptr_t platform_backup_ram_all_start[];
    extern uintptr_t platform_backup_ram_all_end;
    memset(platform_backup_ram_all_start, 0,
            (uintptr_t)&platform_backup_ram_all_end - (uintptr_t)platform_backup_ram_all_start);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_04_initial_generation_2) {
    assertEqual(RESET_REASON_USER, System.resetReason());
    struct stat st = {};
    assertEqual(0, stat(backupRamFilePath, &st));
    assertMoreOrEqual((int)st.st_size, 4);

    const uint32_t first = HAL_RNG_GetRandomNumber();
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (HAL_RNG_GetRandomNumber() == first) {
            ++same;
        }
    }
    assertLessOrEqual(same, 2);

    unlink(backupRamFilePath);
    extern uintptr_t platform_backup_ram_all_start[];
    extern uintptr_t platform_backup_ram_all_end;
    memset(platform_backup_ram_all_start, 0,
            (uintptr_t)&platform_backup_ram_all_end - (uintptr_t)platform_backup_ram_all_start);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_05_reseed_stress) {
    // One auto-reseed per 10001 calls: reseed_interval is 10000
    const int cycles = 50;
    uint32_t reference = 0;
    int same = 0;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        uint32_t word = 0;
        for (int i = 0; i < 10001; ++i) {
            word = HAL_RNG_GetRandomNumber();
        }
        if (cycle == 1) {
            reference = word;
        } else if (cycle > 1 && word == reference) {
            ++same;
        }
    }
    assertLessOrEqual(same, 1);
}

test(RNG_06_seed_corruption_data_zeros) {
    // Corrupt the seed data, but not magic/version: the next boot takes the
    // warm path, seeded from garbage plus the fresh ADC refresh bytes
    const auto seed = findRetainedSeed();
    assertTrue(seed != nullptr);
    memset(reinterpret_cast<uint8_t*>(seed) + retainedSeedDataOffset, 0, 32);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_07_seed_corruption_data_zeros_replay) {
    // Corrupt the seed data again the same way, save the stream generated on
    // this boot and reboot: both this and the next boot are seeded from the
    // same zeros plus fresh refresh bytes, so a broken refresh harvest would
    // make the two streams identical
    const auto seed = findRetainedSeed();
    assertTrue(seed != nullptr);
    memset(reinterpret_cast<uint8_t*>(seed) + retainedSeedDataOffset, 0, 32);

    uint32_t stream[8] = {};
    for (auto& value: stream) {
        value = HAL_RNG_GetRandomNumber();
    }
    memcpy(corruptedSeedStream, stream, sizeof(stream));
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_08_seed_corruption_data_zeros_replay) {
    uint32_t stream[8] = {};
    for (auto& value: stream) {
        value = HAL_RNG_GetRandomNumber();
    }
    int same = 0;
    for (size_t i = 0; i < sizeof(stream) / sizeof(stream[0]); ++i) {
        if (stream[i] == corruptedSeedStream[i]) {
            ++same;
        }
    }
    assertLess(same, 2);
}

test(RNG_09_seed_corruption_struct_garbage) {
    // Corrupt the whole seed structure: the magic check must fence it off and
    // the next boot must reseed from a full ADC harvest
    const auto seed = findRetainedSeed();
    assertTrue(seed != nullptr);
    memset(seed, 0xff, retainedSeedStructSize);
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    System.reset();
}

test(RNG_10_seed_corruption_struct_garbage_replay) {
    assertEqual(RESET_REASON_USER, System.resetReason());
    const uint32_t first = HAL_RNG_GetRandomNumber();
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (HAL_RNG_GetRandomNumber() == first) {
            ++same;
        }
    }
    assertLessOrEqual(same, 2);
}
#endif // HAL_PLATFORM_RTL872X