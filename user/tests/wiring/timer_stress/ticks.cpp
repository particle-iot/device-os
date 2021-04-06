/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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
#include "simple_ntp_client.h"
#include <chrono>

test(TICKS_01_micros_busy_loop_monotonically_increases)
{
    // Using NTP time as a separate independent timing source
    auto client = std::make_unique<SimpleNtpClient>();
    assertTrue((bool)client);

    Particle.connect();
    waitFor(Particle.connected, 10 * 60 * 10000);
    assertTrue(Particle.connected());

    const auto getNtpTime = [&client](uint64_t& ntpTime) -> int {
        int r = SYSTEM_ERROR_UNKNOWN;
        for (int i = 0; i < 10; i++) {
            r = client->ntpDate(&ntpTime);
            if (!r) {
                break;
            }
        }
        return r;
    };

    uint64_t ntpTime = 0;
    assertEqual(0, getNtpTime(ntpTime));

    const auto start = hal_timer_micros(nullptr);
    const auto startNtpTime = ntpTime;
    uint64_t prev = start;

    const auto duration = std::chrono::microseconds(30min).count();

    while (prev - start < duration) {
        uint64_t now = hal_timer_micros(nullptr);
        assertMoreOrEqual(now, prev);
        assertMoreOrEqual(now, start);
        prev = now;
    }

    assertEqual(0, getNtpTime(ntpTime));
    assertMoreOrEqual(ntpTime - startNtpTime, duration * 9 / 10);
}
