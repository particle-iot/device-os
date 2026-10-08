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

#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "unit-test/unit-test.h"

#include "rng_hal.h"
#include "spark_wiring_json.h"
#include "system_control.h"

#if HAL_PLATFORM_RTL872X
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <memory>
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

uint8_t* findRetainedSeed() {
    auto p = reinterpret_cast<uint8_t*>(platform_backup_ram_all_start);
    const auto end = reinterpret_cast<uint8_t*>(&platform_backup_ram_all_end);
    for (; p + retainedSeedStructSize <= end; ++p) {
        uint32_t magic = 0;
        memcpy(&magic, p, sizeof(magic));
        if (magic == retainedSeedMagic && p[4] == retainedSeedVersion) {
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
    const int reseeds = 64;
    uint32_t previous = HAL_RNG_GetRandomNumber();
    int same = 0;
    for (int i = 0; i < reseeds; ++i) {
        assertEqual(0, hal_rng_reseed(nullptr));
        const uint32_t word = HAL_RNG_GetRandomNumber();
        if (word == previous) {
            ++same;
        }
        previous = word;
    }
    assertLessOrEqual(same, 1);
}

test(RNG_06_seed_corruption_data_zeros) {
    // Corrupt the seed data, but not magic/version: the next boot takes the
    // warm path, seeded from garbage plus the fresh ADC refresh bytes
    const auto seed = findRetainedSeed();
    assertTrue(seed != nullptr);
    memset(seed + retainedSeedDataOffset, 0, 32);
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
    memset(seed + retainedSeedDataOffset, 0, 32);

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

test(RNG_11_entropy_source_qualification) {
    const size_t sampleCount = 262144;
    const size_t chunkSize = 1024;
    const size_t codeCount = 4096;

    std::unique_ptr<uint32_t[]> histogram(new (std::nothrow) uint32_t[codeCount]());
    assertTrue((bool)histogram);
    std::unique_ptr<uint16_t[]> chunk(new (std::nothrow) uint16_t[chunkSize]);
    assertTrue((bool)chunk);

    uint64_t sum = 0;
    uint64_t sumSquares = 0;
    uint32_t ones = 0;
    uint32_t transitions[4] = {};
    uint32_t run = 0;
    uint32_t maxRun = 0;
    uint32_t aptReference = 0;
    uint32_t aptCount = 0;
    uint32_t aptPosition = 0;
    uint32_t aptMax = 0;
    uint32_t errors = 0;
    uint32_t samples = 0;
    int last = -1;

    const uint32_t startedAt = micros();
    for (size_t done = 0; done < sampleCount; done += chunkSize) {
        if (hal_rng_entropy_read_raw(chunk.get(), chunkSize, nullptr)) {
            ++errors;
            continue;
        }
        for (size_t i = 0; i < chunkSize; ++i) {
            const uint32_t code = chunk[i] & (codeCount - 1);
            ++histogram[code];
            sum += code;
            sumSquares += (uint64_t)code * code;
            ++samples;

            const int bit = code & 1;
            ones += bit;
            if (last >= 0) {
                ++transitions[last * 2 + bit];
                run = (bit == last) ? run + 1 : 1;
            } else {
                run = 1;
            }
            last = bit;
            if (run > maxRun) {
                maxRun = run;
            }

            if (aptPosition == 0) {
                aptReference = bit;
                aptCount = 1;
                aptPosition = 1;
            } else {
                if ((uint32_t)bit == aptReference) {
                    ++aptCount;
                }
                if (++aptPosition == HAL_RNG_ENTROPY_ADAPTIVE_PROPORTION_WINDOW) {
                    if (aptCount > aptMax) {
                        aptMax = aptCount;
                    }
                    aptPosition = 0;
                }
            }
        }
    }
    const uint32_t elapsed = micros() - startedAt;
    assertMore(samples, 0u);

    uint32_t distinct = 0;
    uint32_t topCode = 0;
    uint32_t topCount = 0;
    uint32_t codeMin = codeCount;
    uint32_t codeMax = 0;
    for (uint32_t code = 0; code < codeCount; ++code) {
        if (!histogram[code]) {
            continue;
        }
        ++distinct;
        if (code < codeMin) {
            codeMin = code;
        }
        if (code > codeMax) {
            codeMax = code;
        }
        if (histogram[code] > topCount) {
            topCount = histogram[code];
            topCode = code;
        }
    }

    Variant stats;
    stats.set("samples", samples);
    stats.set("requested", (uint32_t)sampleCount);
    stats.set("elapsed_us", elapsed);
    stats.set("errors", errors);
    stats.set("ones", ones);
    stats.set("t00", transitions[0]);
    stats.set("t01", transitions[1]);
    stats.set("t10", transitions[2]);
    stats.set("t11", transitions[3]);
    stats.set("max_run", maxRun);
    stats.set("apt_max", aptMax);
    stats.set("apt_window", (uint32_t)HAL_RNG_ENTROPY_ADAPTIVE_PROPORTION_WINDOW);
    stats.set("rct_cutoff", (uint32_t)HAL_RNG_ENTROPY_REPETITION_COUNT_CUTOFF);
    stats.set("apt_cutoff", (uint32_t)HAL_RNG_ENTROPY_ADAPTIVE_PROPORTION_CUTOFF);
    stats.set("code_min", codeMin);
    stats.set("code_max", codeMax);
    stats.set("code_distinct", distinct);
    stats.set("code_top", topCode);
    stats.set("code_top_count", topCount);
    stats.set("code_sum", (double)sum);
    stats.set("code_sum_squares", (double)sumSquares);
    assertEqual(0, pushMailboxMsg(stats.toJSON(), 20000));
}
#endif // HAL_PLATFORM_RTL872X
