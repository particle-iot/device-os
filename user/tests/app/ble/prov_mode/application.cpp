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

// ***********
// IN PROGRESS
// ***********

#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

STARTUP(System.enableFeature(FEATURE_DISABLE_LISTENING_MODE));

const char* advServiceUuid = "6E460120-B5A3-F393-E0A9-E50E24DCCA9E";
const char* serviceUuid = "6E400021-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxUuid = "6E400022-B5A3-F393-E0A9-E50E24DCCA9E";
const char* txUuid = "6E400023-B5A3-F393-E0A9-E50E24DCCA9E";

void setup() {
    Serial.begin(115200);

    LOG(TRACE, "Application started.");

    // Must run before initialising BLE for the first time
    // Even better to call in STARTUP()
    BLE.setProvisioningUuids(serviceUuid, txUuid, rxUuid);
    BLE.setProvisioningAdvServiceUuid(advServiceUuid);
    BLE.provisioningMode(true);
    LOG(TRACE, "BLE prov mode status: %d", BLE.provisioningStatus());

}

void loop() {
}
