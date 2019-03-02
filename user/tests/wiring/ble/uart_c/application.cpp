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

BLEScanResult results[10];

BLEAttribute* peerTxAttr = nullptr;
BLEAttribute* peerRxAttr = nullptr;
BLEConnection* myConn;

void onDataReceived(uint8_t* data, uint16_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin();

    BLE.on();
}

void loop() {
    if (BLE.connected(myConn)) {
        while (Serial.available()) {
            // Read data from Serial into txBuf
            uint8_t txBuf[20];

            peerRxAttr->update(txBuf, sizeof(txBuf));
        }
    }
    else {
        uint8_t count = BLE.scan(results, 10);

        if (count > 0) {
            for (uint8_t i = 0; i < count; i++) {
                bool found = results[i].find(BLE_SIG_AD_TYPE_FLAGS);
                if (found) {
                    myConn = BLE.connect(results[i].address);
                    if (BLE.connected(myConn)) {
                        myConn->fetch("tx", &peerTxAttr);
                        myConn->fetch("rx", &peerRxAttr);
                        peerTxAttr->onDataReceived(onDataReceived);
                    }
                }
            }
        }
    }
}
