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

#define EEPROM_SIZE ((uint8_t)0x64) /* 100 bytes (Max 255/0xFF bytes) as per eeprom_hal.c */

test(EEPROM_ReadWriteSucceedsForAllAddressWithInRange) {
    // when
    for(int i=0;i<EEPROM_SIZE;i++)
    {
        EEPROM.write(i, i);
    }
    // then
    for(int i=0;i<EEPROM_SIZE;i++)
    {
        assertEqual(EEPROM.read(i), i);
    }
}

test(EEPROM_ReadWriteFailsForAnyAddressOutOfRange) {
    // when
    for(int i=EEPROM_SIZE;i<(2*EEPROM_SIZE);i++)
    {
        EEPROM.write(i, i);
    }
    // then
    for(int i=EEPROM_SIZE;i<(2*EEPROM_SIZE);i++)
    {
        assertNotEqual(EEPROM.read(i), i);
    }
}

