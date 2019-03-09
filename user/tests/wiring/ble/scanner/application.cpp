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

#define SCAN_RESULT_COUNT       10

BLEScanResult results[SCAN_RESULT_COUNT];


void setup() {
    Serial.begin();
}

void loop() {
    int count = BLE.scan(results, SCAN_RESULT_COUNT);

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            Serial.println("Advertising data:");
            for (size_t j = 0; j < results[i].advDataLen(); j++) {
                Serial.printf("0x%02x, ", results[i].advData()[j]);
            }
            Serial.println("");

            Serial.println("Scan response data:");
            for (size_t j = 0; j < results[i].srDataLen(); j++) {
                Serial.printf("0x%02x, ", results[i].srData()[j]);
            }
            Serial.println("");
        }
    }

    delay(3000);
}
