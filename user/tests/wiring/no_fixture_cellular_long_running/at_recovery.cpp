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

#if Wiring_Cellular

/* unresponsive AT interface
 *
 * Some Quectel firmware stops servicing the AT channel (DLCI 1) while continuing to service the
 * muxer control channel and the PPP data channel. Data keeps flowing, so connectionState() reports
 * CONNECTED and nothing notices. Device OS detects it with a periodic AT probe and recovers by
 * power cycling the modem.
 *
 * The fault follows a PPP data mode transition (ATD*99***1# -> CONNECT), at roughly 1 in 8
 * transitions in testing, and has never been seen at any other time. This cycles the connection to
 * generate transitions.
 *
 * Pass/fail is keyed on Device OS tearing the connection down and bringing it back with a working
 * AT interface, not on this test's own AT probe. The probe only decides when to stop cycling and
 * wait: if Device OS does not then act, it is treated as a false alarm and cycling resumes. That way
 * a probe that is wrong in either direction costs time rather than a spurious failure.
 *
 * Reproducing the fault is probabilistic, so a run that never sees it reports that and passes. A
 * run that does see it must observe the recovery, or fail.
 */

// Serial1LogHandler at_recovery_logHandler(115200, LOG_LEVEL_ALL, {
//     // { "comm", LOG_LEVEL_NONE }
// });

namespace {

const system_tick_t AT_PROBE_TIMEOUT = 3000;
const unsigned AT_PROBE_ATTEMPTS = 3;
const system_tick_t CONNECT_TIMEOUT = 5 * 60 * 1000;
const system_tick_t DISCONNECT_TIMEOUT = 60 * 1000;
// Device OS declares the fault after ~105s and then power cycles the modem. Measured stand down to
// modem off: ~132s.
const system_tick_t RECOVERY_TIMEOUT = 4 * 60 * 1000;
// Reduce strain on network during testing by limiting the cycle period
const system_tick_t MIN_CYCLE_PERIOD = 30 * 1000;
const system_tick_t PROVOKE_BUDGET = 15 * 60 * 1000;
// The fault can recur on the very first data mode transition after a recovery, 7 of 45 bench
// occurrences were on transition #1 after a power cycle. That is the same fault again, not a failed
// recovery, so allow Device OS a few rounds to land a connection with a working AT interface.
const unsigned MAX_RECOVERY_ROUNDS = 3;

unsigned cycles = 0;
unsigned falseAlarms = 0;
int lastProbeResult = 0;
bool wedgeConfirmed = false;
system_tick_t standDownAt = 0;
system_tick_t connectivityLostAt = 0;
system_tick_t recoveredAt = 0;

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

// Waits for Device OS to act on the fault. Returns false if it does not, which means our probe was
// wrong rather than the modem being wedged.
//
// Watches connectivity rather than any power or network_status signal. Two of those were tried and
// neither works on this path: the network_status power events are gated on a DISABLED transition
// that recovery never makes (and handleIfPowerState only logs), while Cellular.isOff() needs
// IF_POWER_STATE_DOWN *and* !isInterfacePhyReady(), and the phy flag stays set. Connectivity does
// drop, verifiably: IP_CONFIGURED -> IFACE_LINK_UP as soon as recovery starts, back ~15s later.
//
// Since we do not touch the modem while standing down, connectivity dropping can only be Device OS
// tearing it down. Note this asserts the user-visible recovery rather than proving a power cycle
// specifically.
bool waitForRecovery() {
    if (!waitForNot(Cellular.ready, RECOVERY_TIMEOUT)) {
        return false;
    }
    connectivityLostAt = millis();
    return true;
}

} // namespace

test(AT_RECOVERY_00_init) {
    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, CONNECT_TIMEOUT));
}

test(AT_RECOVERY_01_data_mode_cycling_provokes_unresponsive_at) {
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
            // Suspected fault. Stop touching the modem from here: calling disconnect() would take
            // the recovery path itself and mask the probe under test.
            Log.warn("AT unresponsive after %u transitions, standing down", cycles);
            standDownAt = millis();
            if (waitForRecovery()) {
                wedgeConfirmed = true;
                break;
            }
            ++falseAlarms;
            Log.warn("No recovery within %lu ms, treating as a false alarm",
                    (unsigned long)RECOVERY_TIMEOUT);
        }

        Cellular.disconnect();
        waitForNot(Cellular.ready, DISCONNECT_TIMEOUT);

        const system_tick_t elapsed = millis() - cycleStart;
        if (elapsed < MIN_CYCLE_PERIOD) {
            delay(MIN_CYCLE_PERIOD - elapsed);
        }
    }
    Log.info("Ran %u data mode transitions, %u false alarms, wedge %s",
            cycles, falseAlarms, wedgeConfirmed ? "confirmed" : "not seen");
    assertMore(cycles, 0u);
}

test(AT_RECOVERY_02_device_os_recovers_the_modem) {
    if (!wedgeConfirmed) {
        assertEqual(0, pushMailboxMsg(String::format(
                "{\"reproduced\": false, \"cycles\": %u, \"falseAlarms\": %u, \"lastProbe\": %d}",
                cycles, falseAlarms, lastProbeResult), 30000));
        return;
    }

    // What is under test is that Device OS keeps detecting and recovering, not that the modem stops
    // wedging. If the fault recurs immediately, that is a fresh occurrence and must be recovered too.
    bool recovered = false;
    unsigned recurrences = 0;
    for (unsigned round = 0; round < MAX_RECOVERY_ROUNDS; ++round) {
        if (waitFor(Cellular.ready, CONNECT_TIMEOUT) && atResponds()) {
            recovered = true;
            break;
        }
        ++recurrences;
        Log.warn("AT still unresponsive after recovery %u, waiting for the next", round + 1);
        if (!waitForRecovery()) {
            break;
        }
    }
    recoveredAt = millis();

    assertEqual(0, pushMailboxMsg(String::format(
            "{\"reproduced\": true, \"cycles\": %u, \"falseAlarms\": %u, \"recurrences\": %u, "
            "\"detectMs\": %lu, \"recoveryMs\": %lu}",
            cycles, falseAlarms, recurrences,
            (unsigned long)(connectivityLostAt - standDownAt),
            (unsigned long)(recoveredAt - standDownAt)), 30000));
    assertTrue(recovered);
}

#endif // Wiring_Cellular
