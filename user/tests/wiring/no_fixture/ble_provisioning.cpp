/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#if HAL_PLATFORM_BLE

const system_tick_t LISTENING_MODE_STATE_CHANGE_TIMEOUT = 30000;

bool waitListening(bool state, system_tick_t timeout = LISTENING_MODE_STATE_CHANGE_TIMEOUT) {
    if (state) {
        return waitFor(Network.listening, timeout);
    } else {
        return waitForNot(Network.listening, timeout);
    }
}

const system_tick_t WAIT_TIMEOUT = 15 * 60 * 1000;
// Do not enter listening mode based on the flag
test(LISTENING_00_DISABLE_LISTENING_MODE) {
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    Network.listen();
    SCOPE_GUARD({
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
        Network.listen(false);
    });
    // We should not enter listening mode, give LISTENING_MODE_STATE_CHANGE_TIMEOUT
    // worst case scenario for this to NOT happen
    assertFalse(waitListening(true));
    // Check that we are still not listening mode
    assertTrue(waitListening(false));
}

// Enter listening mode based on the flag
test(LISTENING_01_ENABLE_LISTENING_MODE) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    Network.listen(false);
    assertTrue(waitListening(false));
}

// If the flag is enabled while device is in listening mode,
// device-os should exit listening mode
test(LISTENING_02_DISABLE_FLAG_WHILE_IN_LISTENING_MODE) {
    Network.listen();
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
    });
    assertTrue(waitListening(true));
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    while (Particle.process());
    SCOPE_GUARD({
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    });
    assertTrue(waitListening(false));
}

test(LISTENING_03_ENABLE_BLE_PROV_MODE_WHEN_FLAG_SET) {
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    BLE.provisioningMode(true);
    SCOPE_GUARD({
        BLE.provisioningMode(false);
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    });
    delay(100);
    assertTrue(BLE.getProvisioningStatus());
    assertTrue(BLE.advertising());
    BLE.provisioningMode(false);
    delay(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
}

test(LISTENING_04_ENABLE_BLE_PROV_MODE_WHEN_FLAG_CLEARED) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    assertEqual(BLE.provisioningMode(true), (int)SYSTEM_ERROR_NOT_ALLOWED);
    delay(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
    BLE.provisioningMode(false);
    delay(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
}

test(LISTENING_05_ENABLE_BLE_PROV_AFTER_LISTENING_MODE) {
    // 15 min gives the device time to go through a 10 min timeout & power cycle
    SCOPE_GUARD({
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    });
    Network.listen();
    assertTrue(waitListening(true));
    SCOPE_GUARD({
        Network.listen(false);
        assertTrue(waitListening(false));
        // Make sure we restore cloud connection after exiting this test because we entered listening mode
        Particle.connect();
        waitFor(Particle.connected, WAIT_TIMEOUT);
        assertTrue(Particle.connected());
    });
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    while (Particle.process());
    assertTrue(waitListening(false));
    BLE.provisioningMode(true);
    delay(100);
    assertTrue(BLE.getProvisioningStatus());
    BLE.provisioningMode(false);
    delay(100);
    assertFalse(BLE.getProvisioningStatus());
}

#endif