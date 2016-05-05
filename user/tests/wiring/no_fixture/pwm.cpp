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


uint8_t pwm_pins[] = {

#if defined(STM32F2XX)
		D0, D1, D2, D3, A4, A5, WKP, RX, TX
#else
		A0, A1, A4, A5, A6, A7, D0, D1
#endif
};

static pin_t pin = pwm_pins[0];

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

template <typename F> void for_all_pwm_pins(F callback)
{
	for (uint8_t i = 0; i<arraySize(pwm_pins); i++)
	{
		callback(pwm_pins[i]);
	}
}

test(PWM_AnalogWriteOnPinResultsInCorrectFrequency) {
    for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 150);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertEqual(HAL_PWM_Get_Frequency(pin), TIM_PWM_FREQ);
    pinMode(pin, INPUT);
    });
}

test(PWM_AnalogWriteOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
	pinMode(pin, OUTPUT);
	analogWrite(pin, 200);
	// then
	assertEqual(HAL_PWM_Get_AnalogValue(pin), 200);
	pinMode(pin, INPUT);
	});
}

test(PWM_AnalogWriteWithFrequencyOnPinResultsInCorrectFrequency) {
	for_all_pwm_pins([](uint16_t pin) {

	// when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 150, 10000);
    // then
    assertEqual(HAL_PWM_Get_Frequency(pin), 10000);
	pinMode(pin, INPUT);
	});
}

test(PWM_AnalogWriteWithFrequencyOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 200, 10000);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue(pin), 200);
    pinMode(pin, INPUT);
	});
}


test(PWM_LowDCAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

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
    pinMode(pin, INPUT);
	});
}

test(PWM_LowFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    uint32_t avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 25, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    analogWrite(pin, 0, 10);
    pinMode(pin, INPUT);
    // then
    // avgPulseHigh should equal 10000 +/- 50
    assertMoreOrEqual(avgPulseHigh, 9050);
    assertLessOrEqual(avgPulseHigh, 10050);
	});
}

test(PWM_HighFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

	// when
    pinMode(pin, OUTPUT);

    uint32_t avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 25, 10000); // 10% Duty Cycle at 10Hz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    analogWrite(pin, 0, 10);
    pinMode(pin, INPUT);
    // then
    // avgPulseHigh should equal 10 +/- 5
    assertMoreOrEqual(avgPulseHigh, 5);
    assertLessOrEqual(avgPulseHigh, 15);
	});
}

