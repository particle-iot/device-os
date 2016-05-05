/**
 ******************************************************************************
 * @file    bridge.cpp
 * @authors mat
 * @date    27 January 2015
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

#include "application.h"

SYSTEM_MODE(MANUAL);

static uint8_t magic_code[] = { 0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0 };


void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
}

void loop()
{
    for (;;) {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c=='@')
                Serial1.write(magic_code, sizeof(magic_code));
            else
                Serial1.write(c);
        }

        if (Serial1.available()) {
            char c = Serial1.read();
            Serial.write(c);
        }
    }
}
