/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);
STARTUP(System.enableFeature(FEATURE_DISABLE_LISTENING_MODE));

const char* serviceUuid = "6E400021-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxUuid = "6E400022-B5A3-F393-E0A9-E50E24DCCA9E";
const char* txUuid = "6E400023-B5A3-F393-E0A9-E50E24DCCA9E";
const char* versionUuid = "6E400024-B5A3-F393-E0A9-E50E24DCCA9E";

// 16-bit UUIDs can also be used
// BleUuid serviceUuid(0x1234);
// BleUuid txUuid(0x5678);
// BleUuid rxUuid(0x9123);
// BleUuid versionUuid(0x8123);

void ble_prov_mode_handler(system_event_t evt, int param) {
    if (param == ble_prov_mode_connected) {
        Log.info("BLE Event detected: ble_prov_mode_connected");
    }
    if (param == ble_prov_mode_disconnected) {
        Log.info("BLE Event detected: ble_prov_mode_disconnected");
    }
    if (param == ble_prov_mode_handshake_failed) {
        Log.info("BLE Event detected: ble_prov_mode_handshake_failed");
    }
    if (param == ble_prov_mode_handshake_done) {
        Log.info("BLE Event detected: ble_prov_mode_handshake_done");
    }
}

void nw_creds_handler(system_event_t evt, int param) {
    if (param == network_credentials_added) {
        Log.info("BLE Event detected: network_crendetials_added");
    }
}

void setup() {
    // ---------System Events---------
    System.on(ble_prov_mode, ble_prov_mode_handler);
    System.on(network_credentials, nw_creds_handler);

    // ---------Control request filter---------
    // System.setControlRequestFilter(SystemControlRequestAclAction::ACCEPT, {
    //     {CTRL_REQUEST_DFU_MODE, SystemControlRequestAclAction::DENY},
    //     {CTRL_REQUEST_RESET, SystemControlRequestAclAction::DENY}
    // });
    System.setControlRequestFilter(SystemControlRequestAclAction::ACCEPT);

    // ---------Provisioning Service and Characteristic UUIDs---------
    // Provisioning UUIDs must be set before initialising BLE for the first time
    BLE.setProvisioningSvcUuid(serviceUuid);
    BLE.setProvisioningTxUuid(txUuid);
    BLE.setProvisioningRxUuid(rxUuid);
    BLE.setProvisioningVerUuid(versionUuid);

    // ---------Custom mobile secret---------
    // Uncomment the following three lines with user defined custom mobile secret as needed
    // char arr[HAL_DEVICE_SECRET_SIZE] = {};
    // memcpy(arr, "0123456789abcde", HAL_DEVICE_SECRET_SIZE);
    // hal_set_device_secret(arr, sizeof(arr), nullptr);
    // To clear device secret and use default, run this API -> hal_clear_device_secret(nullptr);

    // ---------Setup device name---------
    BLE.setDeviceName("aabbccdd", 8);

    // ---------Set company ID---------
    BLE.setProvisioningCompanyId(0x1234);

    // ---------BLE provisioning mode---------
    BLE.provisioningMode(true);
    LOG(TRACE, "BLE prov mode status: %d", BLE.getProvisioningStatus());
    // To exit provisioning mode, run this API -> BLE.provisioningMode(false);

}

void loop() {
}