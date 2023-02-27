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

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

#define UART_TX_BUF_SIZE        20
#define SCAN_RESULT_COUNT       20

BleScanResult results[SCAN_RESULT_COUNT];

BleCharacteristic peerTxCharacteristic;
BleCharacteristic peerRxCharacteristic;
BlePeerDevice peer;

uint8_t txBuf[UART_TX_BUF_SIZE];
size_t txLen = 0;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin(115200);
    peerTxCharacteristic.onDataReceived(onDataReceived, &peerTxCharacteristic);
}

void loop() {
    if (BLE.connected()) {
        while (Serial.available() && txLen < UART_TX_BUF_SIZE) {
            txBuf[txLen++] = Serial.read();
            Serial.write(txBuf[txLen - 1]);
        }
        if (txLen > 0) {
            peerRxCharacteristic.setValue(txBuf, txLen);
            txLen = 0;
        }
    }
    else {
        delay(3000);
        ssize_t count = BLE.scan(results, SCAN_RESULT_COUNT);
        if (count > 0) {
            for (uint8_t i = 0; i < count; i++) {
                BleUuid foundServiceUUID;
                size_t svcCount = results[i].advertisingData().serviceUUID(&foundServiceUUID, 1);
                if (svcCount > 0 && foundServiceUUID == "6E400001-B5A3-F393-E0A9-E50E24DCCA9E") {
                    peer = BLE.connect(results[i].address());
                    if (peer.connected()) {
                        peer.getCharacteristicByDescription(peerTxCharacteristic, "tx");
                        peer.getCharacteristicByUUID(peerRxCharacteristic, "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
                    }
                    break;
                }
            }
        }
    }
}

