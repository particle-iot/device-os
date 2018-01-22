/**
 ******************************************************************************
 * @file    wifitester.cpp
 * @authors  David Middlecamp, Matthew McGowan
 * @version V1.0.0
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
#include "spark_wiring_wifitester.h"

SYSTEM_MODE(MANUAL);

#define USE_SERIAL1 1

#ifndef USE_SERIAL1
#define USE_SERIAL1 0
#endif

WiFiTester tester;

void setup()
{
    Serial.begin(9600);
#if USE_SERIAL1
    Serial1.begin(9600);
#endif
    tester.setup(USE_SERIAL1);
}

void loop()
{
    int c = -1;
	if (tester.serialAvailable()) {
            c = tester.serialRead();
            char s[2];
            s[0] = c;
            s[1] = 0;
            tester.serialPrint(s);
    }
    tester.loop(c);
}

