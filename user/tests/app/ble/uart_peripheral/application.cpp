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

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const char* serviceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* txUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BleCharacteristic txCharacteristic("tx", PROPERTY::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", PROPERTY::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived);

uint8_t txBuf[UART_TX_BUF_SIZE];
size_t txLen = 0;


void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    LOG(TRACE, "Received data from: %02X:%02X:%02X:%02X:%02X:%02X:",
            peer.address[0], peer.address[1], peer.address[2], peer.address[3], peer.address[4], peer.address[5]);

    BleCharacteristic* characteristic = static_cast<BleCharacteristic*>(context);
    BleUuid uuid = characteristic->UUID();
    Serial1.print("Characteristic UUID: ");
    for (int i = 0; i < 16; i++) {
        Serial1.printf("0x%02X,", uuid.full()[i]);
    }
    Serial1.println("");

    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin(115200);

    //LOG(TRACE, "Application started.");

    BLE.addCharacteristic(txCharacteristic);
    BLE.addCharacteristic(rxCharacteristic);

    BLE.advertise();
}

void loop() {
    if (BLE.connected()) {
        while (Serial.available() && txLen < UART_TX_BUF_SIZE) {
            txBuf[txLen++] = Serial.read();
        }

        if (txLen > 0) {
            txCharacteristic.setValue(txBuf, txLen);
            txLen = 0;
        }
    }
}
