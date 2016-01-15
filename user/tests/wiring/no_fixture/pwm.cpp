/**
 ******************************************************************************
 * @file    pwm.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   PWM test application
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
#include "pwm_hal.h"
#include "unit-test/unit-test.h"

#if defined(STM32F2XX)
static pin_t pin = D0;//pin under test
#else
static pin_t pin = A0;//pin under test
#endif

test(PWM_NoAnalogWriteWhenPinModeIsNotSetToOutput) {
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    analogWrite(pin, 50);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(pin), 50);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsNotTimerChannel) {
    pin_t pin = D5;//pin under test
    // when
    pinMode(pin, OUTPUT);//D5 is not a Timer channel
    analogWrite(pin, 100);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertNotEqual(HAL_PWM_Get_Frequency(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = 51;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);//21 is not a user pin
    analogWrite(pin, 100);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(pin), 100);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectFrequency) {
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 150);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertEqual(HAL_PWM_Get_Frequency(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectAnalogValue) {
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 200);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue(pin), 200);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteWithFrequencyOnPinResultsInCorrectFrequency) {
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 150, 10000);
    // then
    assertEqual(HAL_PWM_Get_Frequency(pin), 10000);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteWithFrequencyOnPinResultsInCorrectAnalogValue) {
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 200, 10000);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue(pin), 200);
    //To Do : Add test for remaining pins if required
}


test(PWM_LowDCAnalogWriteOnPinResultsInCorrectPulseWidth) {
    pin_t pin = D0; // pin under test
    // when
    pinMode(pin, OUTPUT);

    uint32_t avgPulseHigh = 0;
    for(int i=0;i<100;i++) {
        analogWrite(pin, 25); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 100;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 50
    assertMoreOrEqual(avgPulseHigh, 150);
    assertLessOrEqual(avgPulseHigh, 250);
}

test(PWM_LowFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
    pin_t pin = D0; // pin under test
    // when
    pinMode(pin, OUTPUT);

    uint32_t avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 25, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    analogWrite(pin, 0, 10);

    // then
    // avgPulseHigh should equal 10000 +/- 50
    assertMoreOrEqual(avgPulseHigh, 9050);
    assertLessOrEqual(avgPulseHigh, 10050);
}

test(PWM_HighFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
    pin_t pin = D0; // pin under test
    // when
    pinMode(pin, OUTPUT);

    uint32_t avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 25, 10000); // 10% Duty Cycle at 10Hz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    analogWrite(pin, 0, 10);

    // then
    // avgPulseHigh should equal 10 +/- 5
    assertMoreOrEqual(avgPulseHigh, 5);
    assertLessOrEqual(avgPulseHigh, 15);
}

