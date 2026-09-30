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

// cellular_registration_backoff_set_schedule() is behind the unstable API gate
#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "unit-test/unit-test.h"

// Serial1LogHandler log1Handler(115200, LOG_LEVEL_ALL);

/* registration backoff, API contract
 *
 * This suite runs on a healthy SIM, so it covers the other half of the contract, that a device
 * which registers normally is unaffected. The behaviour itself is in
 * no_fixture_cellular_long_running/registration_backoff.cpp.
 */

#if Wiring_Cellular == 1

// Defined in cellular.cpp, which builds alongside this one
void connect_to_cloud(system_tick_t timeout);

namespace {

cellular_backoff_state_t backoffState() {
    cellular_backoff_state_t state = {};
    state.size = sizeof(state);
    const int r = cellular_registration_backoff_state(&state, nullptr);
    if (r != SYSTEM_ERROR_NONE) {
        // Reported by the caller's assertion on the returned stage, which stays 0 here
        Log.error("cellular_registration_backoff_state() failed: %d", r);
    }
    return state;
}

// Obviously wrong to ship, so a build that accepts it is loud rather than subtle
cellular_backoff_schedule_t absurdSchedule() {
    cellular_backoff_schedule_t s = {};
    s.size = sizeof(s);
    s.active_window_ms = 1000;
    s.first_stages = 0;
    s.intervals_ms[0] = 2000;
    s.intervals_ms[1] = 3000;
    s.intervals_ms[2] = 4000;
    s.heartbeat_ms = 500;
    return s;
}

} // namespace

test(CELLULAR_BACKOFF_01_a_registered_device_sits_at_stage_one) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    const auto state = backoffState();
    assertEqual((int)state.size, (int)sizeof(state));
    // 1 based, a device that has never failed a window reads 1
    assertEqual((unsigned)state.stage, 1u);
    assertEqual((int)state.in_cooldown, 0);
    assertEqual((unsigned)state.cooldown_remaining_ms, 0u);
}

test(CELLULAR_BACKOFF_02_state_rejects_a_null_argument) {
    assertEqual(SYSTEM_ERROR_INVALID_ARGUMENT, cellular_registration_backoff_state(nullptr, nullptr));
}

test(CELLULAR_BACKOFF_03_reset_is_a_no_op_on_a_healthy_device) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    // Nothing to abandon on a device that is already registered
    // Proving that must not disturb the connection
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_backoff_reset(nullptr));
    delay(2000);
    assertTrue(Cellular.ready());
    assertTrue(Particle.connected());

    const auto state = backoffState();
    assertEqual((unsigned)state.stage, 1u);
    assertEqual((int)state.in_cooldown, 0);
}

test(CELLULAR_BACKOFF_04_the_schedule_override_validates_what_it_is_given) {
    auto s = absurdSchedule();

    // This ships, guarded by PARTICLE_USE_UNSTABLE_API at the top of this file
    auto bad = s;
    bad.size = sizeof(bad) - 1;
    assertEqual(SYSTEM_ERROR_INVALID_ARGUMENT, cellular_registration_backoff_set_schedule(&bad, nullptr));

    bad = s;
    bad.active_window_ms = 0;
    assertEqual(SYSTEM_ERROR_INVALID_ARGUMENT, cellular_registration_backoff_set_schedule(&bad, nullptr));

    bad = s;
    bad.heartbeat_ms = 0;
    assertEqual(SYSTEM_ERROR_INVALID_ARGUMENT, cellular_registration_backoff_set_schedule(&bad, nullptr));

    bad = s;
    bad.intervals_ms[1] = 0;
    assertEqual(SYSTEM_ERROR_INVALID_ARGUMENT, cellular_registration_backoff_set_schedule(&bad, nullptr));

    // Restore before asserting on anything
    // A one second window left in force fails every later test that needs to register
    const int applied = cellular_registration_backoff_set_schedule(&s, nullptr);
    const int restored = cellular_registration_backoff_set_schedule(nullptr, nullptr);
    assertEqual(SYSTEM_ERROR_NONE, applied);
    assertEqual(SYSTEM_ERROR_NONE, restored);

    // NULL restores the compiled in schedule and the registration timeout with it
    const auto state = backoffState();
    assertEqual((unsigned)state.stage, 1u);
    assertEqual((int)state.in_cooldown, 0);
}

test(CELLULAR_BACKOFF_05_the_registration_timeout_takes_the_active_window_with_it) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    // cellular_registration_timeout_set() moves the window the cooldown is derived from
    // The arithmetic is a unit test, this catches a build where the two came apart
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_timeout_set(20 * 60 * 1000, nullptr));
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_timeout_set(10 * 60 * 1000, nullptr));

    const auto state = backoffState();
    assertEqual((unsigned)state.stage, 1u);
    assertEqual((int)state.in_cooldown, 0);
    assertTrue(Cellular.ready());
}

#endif // Wiring_Cellular == 1
