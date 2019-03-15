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

uint8_t uuids[2][16] = {
    {0xd0,0x2c,0x4d,0xe4,0x00,0x4e,0x98,0xb2,0x87,0x41,0x9c,0xcf,0xa1,0xf1,0x24,0x91},
    {0x8d,0xac,0xef,0x0d,0x1a,0x7b,0x38,0x83,0x66,0x4b,0x1d,0x40,0x64,0xeb,0x70,0x01}
};

BLEUUID uartSvcUuid(0x5AA5);
BLEUUID txUuid(uuids[0]);
BLEUUID rxUuid(uuids[1]);

void onDataReceived(uint8_t* data, size_t len);

BLEAttribute txAttr("tx", NOTIFY, txUuid, uartSvcUuid);
BLEAttribute rxAttr("rx", WRITE_WO_RSP, rxUuid, uartSvcUuid, onDataReceived);

void onDataReceived(uint8_t* data, size_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin();

    BLE.advertise();
}

void loop() {
    if (BLE.connected()) {
        while (Serial.available()) {
            // Read data from Serial into txBuf
            uint8_t txBuf[20];

            txAttr.setValue(txBuf, sizeof(txBuf));
        }
    }
}
