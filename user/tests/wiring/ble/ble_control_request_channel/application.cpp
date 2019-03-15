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

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

const uint8_t BLE_CTRL_REQ_SVC_UUID[]= { 0xfc, 0x36, 0x6f, 0x54, 0x30, 0x80, 0xf4, 0x94, 0xa8, 0x48, 0x4e, 0x5c, 0x01, 0x00, 0xa9, 0x6f };

BleUuid bleCtrlReqSvcUuid(BLE_CTRL_REQ_SVC_UUID);
BleUuid verCharUuid(BLE_CTRL_REQ_SVC_UUID, 0x0002);
BleUuid txCharUuid(BLE_CTRL_REQ_SVC_UUID, 0x0003);
BleUuid rxCharUuid(BLE_CTRL_REQ_SVC_UUID, 0x0004);

// Generate a UUID: https://www.uuidgenerator.net
BleUuid mysvcUuid("4a7e62de-dcac-4dee-bfe3-c9bd54e69642");
BleUuid myCharUuid("56ea805a-725a-4538-bda2-5f6a11f3bfae");

void onDataReceived(const uint8_t* data, size_t len);

BleAttribute verAttr("Version", PROPERTY::READ, verCharUuid, bleCtrlReqSvcUuid);
BleAttribute txAttr("Control TX", PROPERTY::NOTIFY, txCharUuid, bleCtrlReqSvcUuid);
BleAttribute rxAttr("Control RX", PROPERTY::WRITE | PROPERTY::WRITE_WO_RSP, rxCharUuid, bleCtrlReqSvcUuid, onDataReceived);

BleAttribute myAttr("Custom", PROPERTY::READ, myCharUuid, mysvcUuid);

size_t counter = 0;

void onDataReceived(const uint8_t* data, size_t len) {
    LOG(TRACE, "Control RX received data:");
    for (uint8_t i = 0; i < len; i++) {
        LOG(TRACE, "0x%02x", data[i]);
    }
}

void setup() {
    BLE.on();

    verAttr.setValue("1.2.3");

    BLE.advertise();
}

void loop() {
    if (BLE.connected()) {
        txAttr.setValue(counter);
    }

    delay(5000);
    counter++;
}
