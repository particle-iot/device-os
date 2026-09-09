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

#include <string.h>

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

namespace {

const size_t MAX_DUMP_WORDS = 8192;
const uint32_t REPLAY_MAGIC = 0x5a5a5a5a;

static retained uint32_t previousStream[8];
static retained uint32_t replayState;

int rngDumpRequest(ctrl_request* req) {
    String command;
    size_t size = 1024;
    const auto data = JSONValue::parse(req->request_data, req->request_size);
    CHECK_TRUE(data.isObject(), SYSTEM_ERROR_BAD_DATA);

    JSONObjectIterator it(data);
    while (it.next()) {
        if (it.name() == "c") {
            command = it.value().toString();
        } else if (it.name() == "n") {
            CHECK_TRUE(it.value().isNumber(), SYSTEM_ERROR_INVALID_ARGUMENT);
            size = it.value().toInt();
        }
    }

    CHECK_TRUE(command == "R", SYSTEM_ERROR_NOT_SUPPORTED);
    CHECK_TRUE(size > 0 && size <= MAX_DUMP_WORDS, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(system_ctrl_alloc_reply_data(req, size * sizeof(uint32_t), nullptr));

    auto output = static_cast<uint32_t*>(req->reply_data);
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
        assertLess(same, 2);
        return;
    }

    memcpy(previousStream, stream, sizeof(stream));
    replayState = REPLAY_MAGIC;
#ifdef PARTICLE_TEST_RUNNER
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
#endif
    System.reset();
}

test(RNG_03_host_side_statistical_analysis) {
}
