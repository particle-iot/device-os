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


/*
 *************************************************
 * Keep antenna disconnected for all tests below *
 *************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

namespace {

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;
static retained uint32_t phase = 0;

} // anonymous

#if Wiring_Cellular

void disconnect_from_cloud(system_tick_t timeout) {
    Particle.disconnect();
    waitFor(Particle.disconnected, timeout);

    Cellular.disconnect();
    waitForNot(Cellular.ready, timeout);

    // Avoids some sort of race condition in AUTOMATIC mode
    delay(1000);
}
void connect_to_cloud(system_tick_t timeout) {
    Particle.connect();
    waitFor(Particle.connected, timeout);
}
// Global variable to indicate a connection attempt
int g_state_conn_attempt = 0;
void nwstatus_callback_handler(system_event_t ev, int param) {
    if (param == network_status_connecting){
        g_state_conn_attempt = 1;
    }
}
int network_is_connecting() {
    return g_state_conn_attempt;
}

/*
 *  The following illustrates the problem statement:
 *  > Keep antenna disconnected for this test
 *  1. With antenna disconnected, run `Particle.connect()`
 *  2. Run `Cellular.off()`
 *  3. After 60 sec, run `Particle.connect()` again, and verify that you see cellular AT traffic to turn on the modem or connect to the network. 
 */
test(CELLULAR_NO_ANTENNA_01_conn_after_off) {
    /* This test should only be run with threading enabled */
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }

    if (phase == 0xbeef0002) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    } else {
        // Connect to Particle cloud
        Particle.connect();
        delay(30000);
        // Power off the cell radio
        Cellular.off();
        delay(60000);
        // Callback handler for network_status
        System.on(network_status, nwstatus_callback_handler);
        // clear g_state_conn_attempt just-in-case
        g_state_conn_attempt = 0;
        // Check that Particle.connect() attempts to work after the delay
        Particle.connect();
        // Wait sometime for Particle.connect() to try
        waitFor(network_is_connecting, 30000);
        // Verify that a connection attempt has been made
        assertEqual(g_state_conn_attempt, 1);
    }
}

// Test for ch73242
test(CELLULAR_NO_ANTENNA_02_device_will_poweroff_quickly_when_modem_cannot_connect) {
    /* This test should only be run with threading enabled */
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }

    if (phase == 0xbeef0002) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    } else {
        const system_tick_t waitMs[9] = {2000, 4000, 5000, 7500, 10000, 12500, 15000, 25000};
        // Callback handler for network_status
        System.on(network_status, nwstatus_callback_handler);
        // clear g_state_conn_attempt just-in-case
        g_state_conn_attempt = 0;
        for (int x = 0; x < 9; x++) {
            Particle.connect();
            // Log.info("delaying: %lu", waitMs[x]);
            delay(waitMs[x]);
            assertTrue(Particle.disconnected());
            // cellular_cancel(true, false, NULL); // Workaround: call before Cellular.off() / System.sleep(config);
            Cellular.off();
            waitFor(Cellular.isOff, 60000);
            assertTrue(Cellular.isOff());
        }
        // Check that Particle.connect() attempts to work after the delay
        Particle.connect();
        // Wait sometime for Particle.connect() to try
        waitFor(network_is_connecting, 30000);
        // Verify that a connection attempt has been made
        assertEqual(g_state_conn_attempt, 1);
    }
}

// Test for ch73242
test(CELLULAR_NO_ANTENNA_03a_device_will_sleep_quickly_when_modem_cannot_connect) {
    /* This test should only be run with threading enabled */
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }

    if (magick != 0xdeadbeef) {
        magick = 0xdeadbeef;
        phase = 0xbeef0001;
    }
    if (phase == 0xbeef0001) {
        Serial.println("    >> Device attempts to connect to the Cloud for 25s.");
        Particle.connect();
        delay(25000);
        assertTrue(Particle.disconnected());
        Serial.println("    >> Device is still trying to connect...");
        Serial.println("    >> Device enters hibernate mode.");
        Serial.println("    >> Please reconnect serial and type 't' immediately after you press the reset button.");
        Serial.println("    >> Press any key now - if device does not sleep within 60s, consider it a failure.");
        while (Serial.available() <= 0);
        while (Serial.available() > 0) {
            (void)Serial.read();
        }

        phase = 0xbeef0002;

        // cellular_cancel(true, false, NULL); // Workaround: call before Cellular.off() / System.sleep(config);
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::HIBERNATE);
        SystemSleepResult result = System.sleep(config);
        assertEqual(result.error(), SYSTEM_ERROR_NONE);
    } else if (phase == 0xbeef0002) {
        Serial.println("    >> Device is reset from hibernate mode.");
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    }
}

test(CELLULAR_NO_ANTENNA_03b_device_will_sleep_quickly_when_modem_cannot_connect) {
    /* This test should only be run with threading enabled */
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }

    if (phase == 0xbeef0002) {
        phase = 0xbeef0001;
        assertEqual(System.resetReason(), (int)RESET_REASON_PIN_RESET);
    } else {
        Serial.println("    >> Please run Test 3a first!");
        fail();
    }
}

#endif // Wiring_Cellular
