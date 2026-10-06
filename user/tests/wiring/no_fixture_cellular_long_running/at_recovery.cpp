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
#include "test_suite.h"

#if HAL_PLATFORM_CELLULAR

/* unresponsive AT interface
 *
 * Some Quectel firmware leaves preprocess_mode stuck at 1 after a PPP dial, so ATCoP swallows every
 * AT command that follows. Device OS probes AT right after the dial and recovers with +++ on the
 * data channel, then dials again. After DIAL_ATTEMPTS of that it power cycles the modem, which
 * clears it too.
 *
 * All of that happens inside enterDataMode(), before the interface goes ready, so from up here the
 * recovery should be invisible. Every connection has to come up with a working AT interface. This
 * cycles the connection to produce data mode transitions and fails if one does not.
 *
 * A power cycle still passes. It reaches the same place, just in ~80s rather than a few seconds.
 */

// Serial1LogHandler at_recovery_logHandler(115200, LOG_LEVEL_ALL, {
//     // { "comm", LOG_LEVEL_NONE }
// });

namespace {

const system_tick_t AT_PROBE_TIMEOUT = 3000;
const unsigned AT_PROBE_ATTEMPTS = 3;
const system_tick_t CONNECT_TIMEOUT = 5 * 60 * 1000;
const system_tick_t DISCONNECT_TIMEOUT = 60 * 1000;
// Reduce strain on network during testing by limiting the cycle period
const system_tick_t MIN_CYCLE_PERIOD = 30 * 1000;
// Around 6 cycles at that floor. The stall hits most connections, so that is plenty of exposure.
const system_tick_t PROVOKE_BUDGET = 3 * 60 * 1000;

// The msom platform covers both cellular and Wi-Fi devices, so a Wi-Fi one can be selected here.
// Not retained, nothing in this suite resets the device between tests.
bool skipTests = false;

unsigned cycles = 0;
unsigned atFailedAtCycle = 0;
int lastProbeResult = 0;

// RESP_OK on success, WAIT (-1) on timeout, RESP_ERROR (-3) if the modem answered ERROR.
// A wedged AT interface times out: commands are swallowed with no response and no error.
bool atResponds() {
    for (unsigned i = 0; i < AT_PROBE_ATTEMPTS; ++i) {
        lastProbeResult = Cellular.command(AT_PROBE_TIMEOUT, "AT\r\n");
        if (lastProbeResult == RESP_OK) {
            return true;
        }
        Log.warn("AT probe %u/%u returned %d", i + 1, AT_PROBE_ATTEMPTS, lastProbeResult);
        delay(500);
    }
    return false;
}

} // namespace

test(AT_RECOVERY_00_init) {
    if (TestSuite::instance()->network() != NETWORK_INTERFACE_ALL &&
            TestSuite::instance()->network() != NETWORK_INTERFACE_CELLULAR) {
        skipTests = true;
        skip();
        return;
    }
    skipTests = false;

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, CONNECT_TIMEOUT));
}

test(AT_RECOVERY_01_data_mode_cycling_provokes_unresponsive_at) {
    if (skipTests) {
        skip();
        return;
    }

    const system_tick_t start = millis();
    while (millis() - start < PROVOKE_BUDGET) {
        const system_tick_t cycleStart = millis();
        Cellular.connect();
        assertTrue(waitFor(Cellular.ready, CONNECT_TIMEOUT));
        ++cycles;
        // Cross-check against the ATD*99***1# count in the AT log. If they do not match, the cycle
        // is not producing a data mode transition and the test is not exercising anything.
        Log.info("cycle %u connected", cycles);

        if (!atResponds()) {
            Log.error("AT unresponsive after %u transitions, the dial recovery did not cover it",
                    cycles);
            atFailedAtCycle = cycles;
            break;
        }

        Cellular.disconnect();
        waitForNot(Cellular.ready, DISCONNECT_TIMEOUT);

        const system_tick_t elapsed = millis() - cycleStart;
        if (elapsed < MIN_CYCLE_PERIOD) {
            delay(MIN_CYCLE_PERIOD - elapsed);
        }
    }
    Log.info("Ran %u data mode transitions", cycles);
    assertMore(cycles, 0u);
}

test(AT_RECOVERY_02_device_os_recovers_the_modem) {
    if (skipTests) {
        skip();
        return;
    }

    assertEqual(0, pushMailboxMsg(String::format(
            "{\"cycles\": %u, \"atFailedAtCycle\": %u, \"lastProbe\": %d}",
            cycles, atFailedAtCycle, lastProbeResult), 30000));
    assertEqual(0u, atFailedAtCycle);
}

#endif // HAL_PLATFORM_CELLULAR
