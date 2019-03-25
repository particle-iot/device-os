/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "application.h"

#define SCAN_RESULT_COUNT       5
#define BLE_ADV_DATA_MAX        31

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

BleScannedDevice results[SCAN_RESULT_COUNT];


void setup() {

}

void loop() {
    int count = BLE.scan(results, SCAN_RESULT_COUNT);

    if (count > 0) {
        LOG(TRACE, "%d devices are found:", count);
        for (int i = 0; i < count; i++) {
            LOG(TRACE, "devices %d: %d - %02X:%02X:%02X:%02X:%02X:%02X", i, results[i].rssi,
                    results[i].address.addr[0], results[i].address.addr[1], results[i].address.addr[2],
                    results[i].address.addr[3], results[i].address.addr[4], results[i].address.addr[5]);
            if (results[i].advData.len > 0) {
                LOG(TRACE, "Advertising data:");
                for (size_t j = 0; j < results[i].advData.len; j++) {
                    Serial1.printf("0x%02x, ", results[i].advData.data[j]);
                }
                Serial1.println("\r\n");
            }
            if (results[i].srData.len > 0) {
                LOG(TRACE, "Scan response data:");
                for (size_t j = 0; j < results[i].srData.len; j++) {
                    Serial1.printf("0x%02x, ", results[i].srData.data[j]);
                }
                Serial1.println("\r\n");
            }
        }
    }

    delay(3000);
}
