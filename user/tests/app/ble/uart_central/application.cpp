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

#define UART_TX_BUF_SIZE        20
#define SCAN_RESULT_COUNT       5

BleScanResult results[SCAN_RESULT_COUNT];

BleCharacteristic peerTxCharacteristic;
BleCharacteristic peerRxCharacteristic;
BlePeerDevice peer;

uint8_t txBuf[UART_TX_BUF_SIZE];
size_t txLen = 0;

void onDataReceived(const uint8_t* data, size_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin(115200);
    peerTxCharacteristic.onDataReceived(onDataReceived);
}

void loop() {
    if (peer.connected()) {
        while (Serial.available() && txLen < UART_TX_BUF_SIZE) {
            txBuf[txLen++] = Serial.read();
        }
        if (txLen > 0) {
            peerRxCharacteristic.setValue(txBuf, txLen);
            txLen = 0;
        }
    }
    else {
        size_t count = BLE.scan(results, SCAN_RESULT_COUNT);
        if (count > 0) {
            for (uint8_t i = 0; i < count; i++) {
                String peerDeviceName = results[i].advertisingData.deviceName();
                if (peerDeviceName == "Xenon-123ABC") {
                    peer = BLE.connect(results[i].address);
                    if (peer.connected()) {
                        peerTxCharacteristic = peer.characteristic("tx");
                        peerRxCharacteristic = peer.characteristic("rx");
                    }
                    break;
                }
            }
        }
    }
}

