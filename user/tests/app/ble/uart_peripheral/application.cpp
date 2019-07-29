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

#define UART_TX_BUF_SIZE        20

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const char* serviceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* txUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BleCharacteristic txCharacteristic("tx",
                                   BleCharacteristicProperty::NOTIFY,
                                   txUuid,
                                   serviceUuid);

BleCharacteristic rxCharacteristic("rx",
                                   BleCharacteristicProperty::WRITE_WO_RSP,
                                   rxUuid,
                                   serviceUuid,
                                   onDataReceived, &rxCharacteristic);

uint8_t txBuf[UART_TX_BUF_SIZE];
size_t txLen = 0;


void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    BleAddress address = peer.address();
    LOG(TRACE, "Received data from: %s", address.toString().c_str());

    BleCharacteristic* characteristic = static_cast<BleCharacteristic*>(context);
    Serial1.printf("Characteristic UUID: %s\r\n", characteristic->UUID().toString().c_str());

    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin(115200);

    LOG(TRACE, "Application started.");

    BLE.addCharacteristic(txCharacteristic);
    BLE.addCharacteristic(rxCharacteristic);

    BleAdvertisingData data;
    data.appendServiceUUID(serviceUuid);
    BLE.advertise(&data);
}

void loop() {
    if (BLE.connected()) {
        while (Serial.available() && txLen < UART_TX_BUF_SIZE) {
            txBuf[txLen++] = Serial.read();
            Serial.write(txBuf[txLen - 1]);
        }

        if (txLen > 0) {
            txCharacteristic.setValue(txBuf, txLen);
            txLen = 0;
        }
    }
}
