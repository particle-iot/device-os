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

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// Dot not enter listening mode based on the flag
test(LISTENING_00_DISABLE_LISTENING_MODE) {
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    Network.listen();
    delay(1000); // Time for system thread to enter listening mode, if any
    assertFalse(Network.listening());
}

// Enter listening mode based on the flag
test(LISTENING_01_ENABLE_LISTENING_MODE) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    Network.listen();
    delay(1000); // Time for system thread to enter listening mode
    assertTrue(Network.listening());
    Network.listen(false);
    delay(1000); // Time for system thread to exit listening mode
    assertFalse(Network.listening());
}

// If the flag is enabled while device is in listening mode,
// device-os should exit listening mode
test(LISTENING_02_DISABLE_FLAG_WHILE_IN_LISTENING_MODE) {
    Network.listen();
    delay(1000); // Time for system thread to enter listening mode
    assertTrue(Network.listening());
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    delay(1500); // Time for system thread to process the flag
    assertFalse(Network.listening());
}

test(LISTENING_03_ENABLE_BLE_PROV_MODE_WHEN_FLAG_SET) {
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    BLE.provisioningMode(true);
    HAL_Delay_Milliseconds(100);
    assertTrue(BLE.getProvisioningStatus());
    assertTrue(BLE.advertising());
    BLE.provisioningMode(false);
    HAL_Delay_Milliseconds(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
}

test(LISTENING_04_ENABLE_BLE_PROV_MODE_WHEN_FLAG_CLEARED) {
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    assertEqual(BLE.provisioningMode(true), (int)SYSTEM_ERROR_NOT_ALLOWED);
    HAL_Delay_Milliseconds(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
    BLE.provisioningMode(false);
    HAL_Delay_Milliseconds(100);
    assertFalse(BLE.getProvisioningStatus());
    assertFalse(BLE.advertising());
}

test(LISTENING_05_ENABLE_BLE_PROV_AFTER_LISTENING_MODE) {
    Network.listen();
    HAL_Delay_Milliseconds(1000); // Time for system thread to enter listening mode
    assertTrue(Network.listening());
    System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    HAL_Delay_Milliseconds(1500); // Time for system thread to process the flag
    assertFalse(Network.listening());
    BLE.provisioningMode(true);
    HAL_Delay_Milliseconds(100);
    assertTrue(BLE.getProvisioningStatus());
    BLE.provisioningMode(false);
    HAL_Delay_Milliseconds(100);
    assertFalse(BLE.getProvisioningStatus());
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
}

#endif