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
#include "test_suite.h"

// Serial1LogHandler log1Handler(115200, LOG_LEVEL_ALL);

#if HAL_PLATFORM_CELLULAR

/* registration backoff, behaviour
 *
 * Proves the client drives the modem the way the policy says. The schedule arithmetic itself is a
 * unit test, test/unit_tests/cellular/cellular_registration_backoff.cpp.
 *
 * We shorten the schedule at run time and force registration to fail by locking the modem to a band
 * the SIM cannot use. A short window on its own does not work, the modem answers the first +CEREG?
 * with "registered" 81 ms in.
 */

namespace {

// Long enough for registerNet() to run its whole AT sequence and get a real search
const uint32_t TEST_ACTIVE_WINDOW = 20 * 1000;

// Band 26 only, old Sprint 800 MHz that none of the carriers we roam on run
// A Particle SIM roams T-Mobile, Verizon, AT&T and Alaska Wireless, band 13 is the Verizon trap
// If a run ever registers anyway, suspect refarmed spectrum first
const unsigned TEST_BAND = 26;

// The mask is 128 bits, bit n-1 per band, lowercase hex with no leading zeros
String bandMaskString() {
    return String::format("%lx", (unsigned long)(1UL << (TEST_BAND - 1)));
}
// One free stage, the same shape as production's six
const uint32_t TEST_FIRST_STAGES = 1;
const uint32_t TEST_INTERVALS[] = { 45 * 1000, 60 * 1000, 75 * 1000 };
const uint32_t TEST_HEARTBEAT = 10 * 1000;

// The cooldown is the interval minus the window it contains
system_tick_t expectedCooldown(unsigned index) {
    return TEST_INTERVALS[index] - TEST_ACTIVE_WINDOW;
}

// The first reading is always a little short, we poll and entering a cooldown costs an AT round trip
const system_tick_t COOLDOWN_SLACK = 12 * 1000;

// Failed window, modem power cycle, next failed window, measured at ~60 s
// The 20 s window is only noticed at the next 15 s registration check
const system_tick_t CYCLE_TIMEOUT = 3 * 60 * 1000;
const system_tick_t CONNECT_TIMEOUT = 5 * 60 * 1000;

// Re-derived rather than held in a static, which would not survive the reset the band lock needs
bool shouldSkip() {
    return TestSuite::instance()->network() != NETWORK_INTERFACE_ALL &&
            TestSuite::instance()->network() != NETWORK_INTERFACE_CELLULAR;
}

bool inCooldownNow = false;
unsigned lastStage = 0;

cellular_backoff_state_t backoffState() {
    cellular_backoff_state_t state = {};
    state.size = sizeof(state);
    cellular_registration_backoff_state(&state, nullptr);
    return state;
}

int setSchedule() {
    cellular_backoff_schedule_t s = {};
    s.size = sizeof(s);
    s.active_window_ms = TEST_ACTIVE_WINDOW;
    s.first_stages = TEST_FIRST_STAGES;
    for (unsigned i = 0; i < CELLULAR_BACKOFF_INTERVAL_COUNT; i++) {
        s.intervals_ms[i] = TEST_INTERVALS[i];
    }
    s.heartbeat_ms = TEST_HEARTBEAT;
    return cellular_registration_backoff_set_schedule(&s, nullptr);
}

// Polls, there is no notification for entering a cooldown
bool waitForCooldown(cellular_backoff_state_t* out, system_tick_t timeout) {
    const system_tick_t start = millis();
    while (millis() - start < timeout) {
        const auto s = backoffState();
        if (s.in_cooldown) {
            *out = s;
            inCooldownNow = true;
            return true;
        }
        delay(200);
    }
    return false;
}

bool waitForCooldownEnd(system_tick_t timeout) {
    const system_tick_t start = millis();
    while (millis() - start < timeout) {
        if (!backoffState().in_cooldown) {
            inCooldownNow = false;
            return true;
        }
        delay(200);
    }
    return false;
}

int cfunCallback(int type, const char* buf, int len, int* value) {
    if (type == TYPE_PLUS && value && buf) {
        // strstr, the callback is handed the line with and without a leading \r\n
        const char* p = strstr(buf, "+CFUN:");
        int v = -1;
        if (p && sscanf(p, "+CFUN: %d", &v) == 1) {
            *value = v;
        }
    }
    return WAIT;
}

char g_lteBand[32 + 1] = {};

int bandCallback(int type, const char* buf, int len, int* value) {
    if (type == TYPE_PLUS && buf) {
        const char* p = strstr(buf, "+QCFG: \"band\"");
        if (p) {
            char gsm[4 + 1] = {};
            char lte[32 + 1] = {};
            // <GSM>,<CAT-M1>,<CAT-NB>[,<NTN>], same shape the client parses
            if (sscanf(p, "+QCFG: \"band\",0x%4[^,],0x%32[^,],", gsm, lte) == 2) {
                strlcpy(g_lteBand, lte, sizeof(g_lteBand));
                if (value) {
                    *value = 1;
                }
            }
        }
    }
    return WAIT;
}

// The mask the modem is actually enforcing, empty if the read failed
String currentLteBandMask() {
    g_lteBand[0] = '\0';
    int found = 0;
    if (Cellular.command(bandCallback, &found, 10000, "AT+QCFG=\"band\"\r\n") != RESP_OK) {
        return String();
    }
    return String(g_lteBand);
}

// CFUN? reads, it does not set, so this is harmless to the cooldown
int modemFunctionality() {
    int value = -1;
    const int r = Cellular.command(cfunCallback, &value, 10000, "AT+CFUN?\r\n");
    if (r != RESP_OK) {
        return r;
    }
    return value;
}

} // namespace

test(REG_BACKOFF_00_init) {
    if (shouldSkip()) {
        skip();
        return;
    }

    // Start from the compiled in schedule whatever an earlier suite left behind
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_backoff_set_schedule(nullptr, nullptr));

    // Clear first so a half applied set from an interrupted earlier run cannot be inherited
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, CONNECT_TIMEOUT));

    // The runner reads this and flashes it back as an env var asset
    // Env vars are read at boot, so it has to be on the device before the reset below
    const String mask = bandMaskString();
    Log.info("Requesting band lock, LTE mask %s (band %u only)", mask.c_str(), TEST_BAND);
    pushMailboxMsg(String::format("REG_BACKOFF_BANDS=%s", mask.c_str()), 20000 /* wait */);

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);

    expectSystemReset();
    System.reset();
}

test(REG_BACKOFF_01_the_band_lock_is_in_force) {
    if (shouldSkip()) {
        skip();
        return;
    }

    // A walk that fails because the lock never landed looks exactly like a broken backoff
    const String mask = bandMaskString();
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"), mask);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, CONNECT_TIMEOUT));

    // The env var only says what we asked for, this is what the modem is enforcing
    const String applied = currentLteBandMask();
    Log.info("LTE band mask the modem is enforcing: %s, requested %s", applied.c_str(), mask.c_str());
    assertTrue(applied.length() > 0);
    assertEqual(applied, mask);
}

test(REG_BACKOFF_02_failed_windows_walk_the_schedule) {
    if (shouldSkip()) {
        skip();
        return;
    }

    // Down first, so stage 1 is a real attempt and not whatever the last test left the radio on
    Cellular.off();
    assertTrue(waitFor(Cellular.isOff, CONNECT_TIMEOUT));

    assertEqual(SYSTEM_ERROR_NONE, setSchedule());
    Cellular.connect();

    // Stage 1 is free, so the first cooldown is the one stage 2 earns, two failed windows in
    // It reports stage 3, stage() is the window that runs next
    cellular_backoff_state_t first = {};
    assertTrue(waitForCooldown(&first, CYCLE_TIMEOUT * 2));
    assertEqual((unsigned)first.stage, TEST_FIRST_STAGES + 2);
    Log.info("First cooldown at stage %u, %lu ms remaining",
            (unsigned)first.stage, (unsigned long)first.cooldown_remaining_ms);

    // The radio is off, not just idle
    const int cfun = modemFunctionality();
    Log.info("AT+CFUN? during cooldown: %d", cfun);
    assertEqual(cfun, 0);

    // Failing stage 2 earns intervals[0], stage 3 intervals[1], stage 4 intervals[2]
    // Each is observed one stage later than the one that earned it
    for (unsigned i = 0; i < CELLULAR_BACKOFF_INTERVAL_COUNT; i++) {
        cellular_backoff_state_t s = {};
        if (i == 0) {
            s = first;
        } else {
            assertTrue(waitForCooldown(&s, expectedCooldown(i - 1) + CYCLE_TIMEOUT));
        }
        const system_tick_t expected = expectedCooldown(i);
        Log.info("stage %u: cooldown %lu ms remaining, expected %lu",
                (unsigned)s.stage, (unsigned long)s.cooldown_remaining_ms, (unsigned long)expected);
        assertEqual((unsigned)s.stage, TEST_FIRST_STAGES + 2 + i);
        assertMoreOrEqual((system_tick_t)s.cooldown_remaining_ms, expected - COOLDOWN_SLACK);
        assertLessOrEqual((system_tick_t)s.cooldown_remaining_ms, expected);
        // Nothing should be up while the radio is off
        assertFalse(Cellular.ready());
        lastStage = s.stage;

        if (i + 1 < CELLULAR_BACKOFF_INTERVAL_COUNT) {
            assertTrue(waitForCooldownEnd(expected + COOLDOWN_SLACK));
        }
    }

    // Left sitting in the last cooldown on purpose, REG_BACKOFF_03 needs one to break
    assertTrue(inCooldownNow);
}

test(REG_BACKOFF_03_reset_breaks_a_cooldown) {
    if (shouldSkip()) {
        skip();
        return;
    }
    if (!inCooldownNow) {
        // REG_BACKOFF_02 did not get far enough to leave one, its failure is the report
        skip();
        return;
    }

    const auto before = backoffState();
    assertEqual((int)before.in_cooldown, 1);
    assertEqual((unsigned)before.stage, lastStage);
    assertMore((unsigned)before.cooldown_remaining_ms, 0u);

    // The escape hatch, for an application that knows the SIM was just reactivated
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_backoff_reset(nullptr));

    // Takes effect immediately, not at the end of the cooldown that was running
    const system_tick_t start = millis();
    while (millis() - start < 30000) {
        const auto s = backoffState();
        if (!s.in_cooldown && s.stage == 1) {
            inCooldownNow = false;
            break;
        }
        delay(200);
    }
    const auto after = backoffState();
    Log.info("After reset: stage %u, in cooldown %d", (unsigned)after.stage, (int)after.in_cooldown);
    assertEqual((unsigned)after.stage, 1u);
    assertEqual((int)after.in_cooldown, 0);
    assertEqual((unsigned)after.cooldown_remaining_ms, 0u);
    inCooldownNow = false;
}

test(REG_BACKOFF_97_restore_bands) {
    if (shouldSkip()) {
        skip();
        return;
    }

    // Unconditional, a device left pinned to band 26 fails every later suite on the rig
    // The staged copies go too, clearEnv() alone leaves them for the next boot
    System.clearEnv(false /* reset */);
    unlink("/sys/env_app");
    unlink("/sys/env_app.staged");
    unlink("/sys/env_snapshot");
    unlink("/sys/env_snapshot.staged");

    expectSystemReset();
    System.reset();
}

test(REG_BACKOFF_98_the_device_recovers_on_the_shipping_schedule) {
    if (shouldSkip()) {
        skip();
        return;
    }

    // Also the proof that REG_BACKOFF_97 actually restored the rig
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));

    // NULL restores the compiled in schedule, and the ten minute registration timeout with it
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_backoff_set_schedule(nullptr, nullptr));
    assertEqual(SYSTEM_ERROR_NONE, cellular_registration_backoff_reset(nullptr));

    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, CONNECT_TIMEOUT));

    // Registering clears the stage and the retained copy with it, so the next boot starts clean
    const auto s = backoffState();
    assertEqual((unsigned)s.stage, 1u);
    assertEqual((int)s.in_cooldown, 0);

    Particle.connect();
    assertTrue(waitFor(Particle.connected, CONNECT_TIMEOUT));
}

#endif // HAL_PLATFORM_CELLULAR
