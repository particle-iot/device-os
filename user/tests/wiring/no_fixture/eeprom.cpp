/**
 ******************************************************************************
 * @file    eeprom.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   EEPROM test application
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"

struct EEPROMCustomObject{
  float fValue;
  byte bValue;
  char sValue[10];
};

test(EEPROM_Capacity) {
#if (PLATFORM_ID == 0) // Core
  uint16_t expectedCapacity = 128;
#else // Photon/P1/Electron
  uint16_t expectedCapacity = 2048;
#endif
  assertEqual(EEPROM.length(), expectedCapacity);
}

test(EEPROM_ReadWriteSucceedsForAllAddressWithInRange) {
    int EEPROM_SIZE = EEPROM.length();
    uint16_t address = 0;
    uint8_t data = 0;
    uint8_t data_seed = (uint8_t)(EEPROM.read(0)+1);//make sure we write new data every time

    // when
    for(address=0, data=data_seed; address < EEPROM_SIZE; address++, data++)
    {
        EEPROM.write(address, data);
    }
    // then
    for(address=0, data=data_seed; address < EEPROM_SIZE; address++, data++)
    {
        uint8_t data_read = EEPROM.read(address);
        if (data_read!=data) {
            assertEqual(EEPROM.read(address), data);
        }
    }

    // Avoid leaving the EEPROM 100% full which leads to poor performance
    // in other programs using EEPROM on this device in the future
    EEPROM.clear();
}

test(EEPROM_ReadWriteFailsForAnyAddressOutOfRange) {
    int EEPROM_SIZE = EEPROM.length();
    uint16_t address = 0;
    uint8_t data = 0;

    // when
    for(address=EEPROM_SIZE, data=0; address < EEPROM_SIZE+10; address++, data++)
    {
        EEPROM.write(address, data);
    }
    // then
    for(address=EEPROM_SIZE, data=0; address < EEPROM_SIZE+10; address++, data++)
    {
        assertNotEqual(EEPROM.read(address), data);
    }
}

test(EEPROM_PutGetSucceedsForCustomDataType) {
    // when
    EEPROMCustomObject putCustomData = { 123.456f, 100, "Success" };
    EEPROM.put(0, putCustomData);
    // then
    EEPROMCustomObject getCustomData;
    EEPROM.get(0, getCustomData);
    assertTrue(!memcmp(&putCustomData, &getCustomData, sizeof(EEPROMCustomObject)));
    assertEqual(putCustomData.fValue, getCustomData.fValue);
    assertEqual(putCustomData.bValue, getCustomData.bValue);
    assertEqual(strcmp(putCustomData.sValue, getCustomData.sValue), 0);
}

