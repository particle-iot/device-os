/**
 ******************************************************************************
 * @file    gpio.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   GPIO test application
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

test(GPIO_PinModeSetResultsInCorrectMode) {
    PinMode mode[] = {
            OUTPUT,
            INPUT,
            INPUT_PULLUP,
            INPUT_PULLDOWN,
            AF_OUTPUT_PUSHPULL,
            AF_OUTPUT_DRAIN,
            AN_INPUT
#if (PLATFORM_ID == 6)
            ,
            AN_OUTPUT
#endif
    };
    int n = sizeof(mode) / sizeof(mode[0]);
    pin_t pin = A0;//pin under test
    for(int i=0;i<n;i++)
    {
        // when
        pinMode(pin, mode[i]);
        // then
        assertEqual(HAL_Get_Pin_Mode(pin), mode[i]);
    }
    //To Do : Add test for remaining pins if required
}

test(GPIO_NoDigitalWriteWhenPinModeIsNotSetToOutput) {
    pin_t pin = D0;//pin under test
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)HAL_GPIO_Read(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_NoDigitalWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = 21;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)HAL_GPIO_Read(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_DigitalWriteOnPinResultsInCorrectDigitalRead) {
    pin_t pin = D0;//pin under test
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    PinState pinState = (PinState)digitalRead(pin);
    // then
    assertEqual(pinState, HIGH);
    delay(100);
    // when
    digitalWrite(pin, LOW);
    pinState = (PinState)digitalRead(pin);
    // then
    assertEqual(pinState, LOW);
    //To Do : Add test for remaining pins if required
}
