/**
 ******************************************************************************
 * @file    constructor.cpp
 * @authors Satish Nair
 * @date    23 January 2015
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
#include "unit-test/unit-test.h"

#define TEST_VALUE (int)12345
static int initVar = 0;

class ConstructorTestClass {

public:
    ConstructorTestClass(int testVar) { initVar = testVar; }
    static bool isConstructorInvoked(void) { return (initVar == TEST_VALUE); }

};

ConstructorTestClass ConstructorTest(TEST_VALUE);

test(CONSTRUCTOR_Invoked_During_System_Initialization) {
    assertEqual(ConstructorTest.isConstructorInvoked(), true);
}
