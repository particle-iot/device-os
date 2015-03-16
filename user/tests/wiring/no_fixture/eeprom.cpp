/**
 ******************************************************************************
 * @file    eeprom.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   EEPROM test application
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

test(EEPROM_ReadWriteSucceedsForAllAddressWithInRange) {
    int EEPROM_SIZE = EEPROM.length();

    uint8_t data_seed = Time.second(); //simple data seed initialized with current second

    // when
    for(uint16_t address=0, data=data_seed; address < EEPROM_SIZE; address++, data--)
    {
        EEPROM.write(address, data);
        if(data == 0) data=0xFF;
    }
    // then
    for(uint16_t address=0, data=data_seed; address < EEPROM_SIZE; address++, data--)
    {
        assertEqual(EEPROM.read(address), data);
        if(data == 0) data=0xFF;
    }
}

test(EEPROM_ReadWriteFailsForAnyAddressOutOfRange) {
    int EEPROM_SIZE = EEPROM.length();

    // when
    for(uint16_t address=EEPROM_SIZE, data=0; address < EEPROM_SIZE+10; address++, data++)
    {
        EEPROM.write(address, data);
    }
    // then
    for(uint16_t address=EEPROM_SIZE, data=0; address < EEPROM_SIZE+10; address++, data++)
    {
        assertNotEqual(EEPROM.read(address), data);
    }
}

