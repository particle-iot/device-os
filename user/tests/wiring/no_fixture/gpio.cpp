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

// FIXME: breaks USB comms as USB interrupts are processed in a thread
#if HAL_PLATFORM_RTL872X
#undef SINGLE_THREADED_BLOCK
#define SINGLE_THREADED_BLOCK()
#endif // HAL_PLATFORM_RTL872X

test(GPIO_01_PinModeSetResultsInCorrectMode) {
    PinMode mode[] = {
        OUTPUT,
        INPUT,
        INPUT_PULLUP,
        INPUT_PULLDOWN,
        OUTPUT_OPEN_DRAIN,
        OUTPUT_OPEN_DRAIN_PULLUP,
#if !HAL_PLATFORM_NRF52840 && !HAL_PLATFORM_RTL872X
        // AF_OUTPUT_PUSHPULL,
        // AN_INPUT,
        // AN_OUTPUT
#error "Unsupported platform"
#endif
    };
    int n = sizeof(mode) / sizeof(mode[0]);
    hal_pin_t pin = A0;//pin under test
    for (int i = 0; i < n; i++)
    {
        // when
        pinMode(pin, mode[i]);
        // then
        assertEqual(hal_gpio_get_mode(pin), mode[i]);
    }
    //To Do : Add test for remaining pins if required
}

test(GPIO_02_NoDigitalWriteWhenPinModeIsNotSetToOutput) {
    hal_pin_t pin = A0;//pin under test
    // when
    // pin set to INPUT_PULLUP mode, to keep pin from floating and test failing
    pinMode(pin, INPUT_PULLUP);
    digitalWrite(pin, LOW);
    // then
    assertNotEqual((PinState)hal_gpio_read(pin), LOW);
    //To Do : Add test for remaining pins if required
}

test(GPIO_03_NoDigitalWriteWhenPinSelectedIsOutOfRange) {
    hal_pin_t pin = TOTAL_PINS+1;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)digitalRead(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_04_DigitalWriteOnPinResultsInCorrectDigitalRead) {
    hal_pin_t pin = A0;//pin under test
    PinMode mode[] = {
        OUTPUT,
        OUTPUT_OPEN_DRAIN_PULLUP,
    };
    int n = sizeof(mode) / sizeof(mode[0]);
    for (int i = 0; i < n; i++) {
        // when
        pinMode(pin, mode[i]);
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
}

#if !HAL_PLATFORM_RTL872X
test(GPIO_05_pulseIn_Measures1000usHIGHWithin5Percent) {
#if HAL_PLATFORM_NRF52840
     // pin under test
#if PLATFORM_ID == PLATFORM_ESOMX
    pin_t pin = A3;
#else 
    pin_t pin = D4;
#endif
#else
#error "Unsupported platform"
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
#if HAL_PLATFORM_NRF52840
     // pin under test
#if PLATFORM_ID == PLATFORM_ESOMX
    pin_t pin = A3;
#else 
    pin_t pin = D4;
#endif
#else
#error "Unsupported platform"
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
#endif // !HAL_PLATFORM_RTL872X

test(GPIO_07_pulseIn_TimesOutAfter3Seconds) {
    hal_pin_t pin = D1; // pin under test
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

test(GPIO_08_DigitalReadWorksMixedWithAnalogRead) {
    pin_t pin = A0;

    pinMode(pin, INPUT_PULLUP);
    assertEqual(hal_gpio_get_mode(pin), INPUT_PULLUP);

    // 2 digitalReads
    PinState digVal = (PinState)digitalRead(pin);
    assertEqual(digVal, HIGH);
    digVal = (PinState)digitalRead(pin);
    assertEqual(digVal, HIGH);

    // 2 analogReads
    int32_t anlg_val = analogRead(pin);
    assertMoreOrEqual(anlg_val, 4096/2);
    anlg_val = analogRead(pin);
    assertMoreOrEqual(anlg_val, 4096/2);

    // 2 digitalReads
    digVal = (PinState)digitalRead(pin);
    assertEqual(digVal, HIGH);
    digVal = (PinState)digitalRead(pin);
    assertEqual(digVal, HIGH);
}
