/**
 ******************************************************************************
 * @file    eeprom.cpp
 * @authors Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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


#include "testapi.h"

test(api_eeprom_read_write) {

    uint8_t value = 0;
    API_COMPILE(value=EEPROM.read(10));
    API_COMPILE(EEPROM.write(10, value));
    (void)value++; // avoid compiler warning about assigned but unused `length`
}


class MyClass
{
    int a;
    int b;
public:
};

test(api_eeprom_get_put) {

    MyClass cls;
    API_COMPILE(EEPROM.get(10, cls));
    API_COMPILE(EEPROM.put(10, cls));
}

test(api_eeprom_begin_end_length) {

    EEPtr e(0);
    uint16_t length;
    API_COMPILE(e = EEPROM.begin());
    API_COMPILE(e = EEPROM.end());
    API_COMPILE(length = EEPROM.length());
    (void)length++;  // avoid compiler warning about assigned but unused `length`
}

