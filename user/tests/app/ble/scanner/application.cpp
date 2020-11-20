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

#define SCAN_RESULT_COUNT       30
#define BLE_ADV_DATA_MAX        31

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

BleScanResult results[SCAN_RESULT_COUNT];


void setup() {

}

void loop() {
    int count = BLE.scan(results, SCAN_RESULT_COUNT);

    if (count > 0) {
        Log.info("%d devices are found:", count);
        for (int i = 0; i < count; i++) {
            Log.info(" -------- MAC: %s | RSSI: %dBm --------", results[i].address().toString().c_str(), results[i].rssi());

            String name = results[i].advertisingData().deviceName();
            if (name.length() > 0) {
                Log.info("Local name: %s", name.c_str());
            }
            name = results[i].scanResponse().deviceName();
            if (name.length() > 0) {
                Log.info("Local name: %s", name.c_str());
            }

            const BleAdvertisingData& advData = results[i].advertisingData();
            if (advData.length() > 0) {
                Serial1.print("Advertising data: ");
                for (size_t j = 0; j < advData.length(); j++) {
                    Serial1.printf("0x%02x,", advData[j]);
                }
                Serial1.println("\r\n");
            }

            const BleAdvertisingData& srData = results[i].scanResponse();
            if (srData.length() > 0) {
                Serial1.print("Scan response data: ");
                for (size_t j = 0; j < srData.length(); j++) {
                    Serial1.printf("0x%02x,", srData[j]);
                }
                Serial1.println("\r\n");
            }
        }
    }

    delay(3000);
}
