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

test(GPIO_01_PinModeSetResultsInCorrectMode) {
    PinMode mode[] = {
            OUTPUT,
            INPUT,
            INPUT_PULLUP,
            INPUT_PULLDOWN,
            OUTPUT_OPEN_DRAIN,
#if !HAL_PLATFORM_NRF52840
            AF_OUTPUT_PUSHPULL,
            AN_INPUT
#if (PLATFORM_ID == 6)
            ,
            AN_OUTPUT
#endif
#endif // !HAL_PLATFORM_NRF52840
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

test(GPIO_02_NoDigitalWriteWhenPinModeIsNotSetToOutput) {
    pin_t pin = D0;//pin under test
    // when
    // pin set to INPUT_PULLUP mode, to keep pin from floating and test failing
    pinMode(pin, INPUT_PULLUP);
    digitalWrite(pin, LOW);
    // then
    assertNotEqual((PinState)HAL_GPIO_Read(pin), LOW);
    //To Do : Add test for remaining pins if required
}

test(GPIO_03_NoDigitalWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = TOTAL_PINS+1;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)digitalRead(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_04_DigitalWriteOnPinResultsInCorrectDigitalRead) {
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

test(GPIO_05_pulseIn_Measures1000usHIGHWithin5Percent) {
#if !HAL_PLATFORM_NRF52840
    pin_t pin = D1; // pin under test
#else
    pin_t pin = D4; // pin under test
#endif

    uint32_t avgPulseHigh = 0;
    // when
    SINGLE_THREADED_BLOCK() {
        pinMode(pin, OUTPUT);
        analogWrite(pin, 128); // 50% Duty Cycle at 500Hz = 1000us HIGH, 1000us LOW.
        for(int i=0;i<100;i++) {
            avgPulseHigh += pulseIn(pin, HIGH);
        }
        avgPulseHigh /= 100;
        analogWrite(pin, 0);
    }
    // then
    // avgPulseHigh should equal 1000 +/- 5%
    assertMoreOrEqual(avgPulseHigh, 950);
    assertLessOrEqual(avgPulseHigh, 1050);
}

test(GPIO_06_pulseIn_Measures1000usLOWWithin5Percent) {
#if !HAL_PLATFORM_NRF52840
    pin_t pin = D1; // pin under test
#else
    pin_t pin = D4; // pin under test
#endif

    uint32_t avgPulseLow = 0;
    // when
    SINGLE_THREADED_BLOCK() {
        pinMode(pin, OUTPUT);
        analogWrite(pin, 128); // 50% Duty Cycle at 500Hz = 1000us HIGH, 1000us LOW.
        for(int i=0;i<100;i++) {
            avgPulseLow += pulseIn(pin, LOW);
        }
        avgPulseLow /= 100;
        analogWrite(pin, 0);
    }
    // then
    // avgPulseHigh should equal 1000 +/- 5%
    assertMoreOrEqual(avgPulseLow, 950);
    assertLessOrEqual(avgPulseLow, 1050);
}

test(GPIO_07_pulseIn_TimesOutAfter3Seconds) {
    pin_t pin = D1; // pin under test
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    uint32_t startTime = millis();
    // then
    assertEqual(pulseIn(pin, LOW), 0);
    // timeout should equal 3000 +/- 5%
    assertMoreOrEqual(millis()-startTime, 2850);
    assertLessOrEqual(millis()-startTime, 3150);
    // when
    startTime = millis();
    // then
    assertEqual(pulseIn(pin, HIGH), 0);
    // timeout should equal 3000 +/- 5%
    assertMoreOrEqual(millis()-startTime, 2850);
    assertLessOrEqual(millis()-startTime, 3150);
}

#if !HAL_PLATFORM_NRF52840

test(GPIO_08_AnalogReadWorksMixedWithDigitalRead) {
    pin_t pin = A0;

    // when
    pinMode(pin, INPUT_PULLUP);
    // then
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);

    // 2 analogReads
    analogRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_INPUT);
    analogRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_INPUT);
    // 2 digitalReads
    digitalRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);
    digitalRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);
    // 2 analogReads again
    analogRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_INPUT);
    analogRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_INPUT);
}

#endif //  !HAL_PLATFORM_NRF52840
