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

void onDataReceived(const uint8_t* data, size_t len);

BleUuid uartSvcUuid("14d1d294-8317-4bea-bbfb-9363a6517afc");
BleUuid txUuid("a2ef4f89-11ad-4887-9e32-6eeb57594ced");
BleUuid rxUuid("ad43b620-f076-4fb3-9bdc-785b585f4719");

BleAttribute txAttr("tx", PROPERTY::NOTIFY, txUuid, uartSvcUuid);
BleAttribute rxAttr("rx", PROPERTY::WRITE_WO_RSP, rxUuid, uartSvcUuid, onDataReceived);

uint8_t txBuf[UART_TX_BUF_SIZE];
size_t txLen = 0;


void onDataReceived(const uint8_t* data, size_t len) {
    for (uint8_t i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
}

void setup() {
    Serial.begin();

    BLE.on();

    BLE.advertise();
}

void loop() {
    while (Serial.available() && txLen < UART_TX_BUF_SIZE) {
        txBuf[txLen++] = Serial.read();
    }

    if (txLen > 0) {
        txAttr.setValue(txBuf, txLen);
        txLen = 0;
    }
}
