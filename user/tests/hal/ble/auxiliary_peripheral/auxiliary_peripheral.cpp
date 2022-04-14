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

#include "application.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);


uint8_t svc1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x0f};
uint8_t char1UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x0f};
uint8_t svc2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x00,0x00,0x0e,0x10};
uint8_t char2UUID[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0a,0x0b,0x01,0x00,0x0e,0x10};
uint16_t svc3Uuid  = 0x1234;
uint16_t char3Uuid = 0x5678;

BleCharacteristic Characteristic1("char1", PROPERTY::READ|PROPERTY::WRITE, char1UUID, svc1UUID);
BleCharacteristic Characteristic2("char2", PROPERTY::NOTIFY, char2UUID, svc2UUID);
BleCharacteristic Characteristic3("char3", PROPERTY::WRITE_WO_RSP, char3Uuid, svc3Uuid);

/* This function is called once at start up ----------------------------------*/
void setup()
{
    BLE.addCharacteristic(Characteristic1);

    uint8_t data[20] = {0x11};
    Characteristic1.setValue(data, 5);

    BLE.addCharacteristic(Characteristic2);
    BLE.addCharacteristic(Characteristic3);

    BleAdvertisingData advData;
    advData.appendLocalName("Argon BLE Sample");
    BLE.advertise(&advData);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    static uint16_t cnt = 0;

    if (BLE.connected()) {
        Characteristic2.setValue(cnt);
        cnt++;
    } else {
        cnt = 0;
    }

    delay(3000);
}
