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

const uint8_t pwm_pins[] = {

#if (PLATFORM_ID == 0) // Core
        A0, A1, A4, A5, A6, A7, D0, D1
#elif (PLATFORM_ID == 6) // Photon
		D0, D1, D2, D3, A4, A5, WKP, RX, TX
#elif (PLATFORM_ID == 8) // P1
        D0, D1, D2, D3, A4, A5, WKP, RX, TX, P1S0, P1S1, P1S6
#elif (PLATFORM_ID == 10) // Electron
        D0, D1, D2, D3, A4, A5, WKP, RX, TX, B0, B1, B2, B3, C4, C5
#endif
};

static pin_t pin = pwm_pins[0];

#if (PLATFORM_ID == 8) // P1
test(PWM_00_P1S6SetupForP1) {
    // disable POWERSAVE_CLOCK on P1S6
    System.disableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
}
#endif

test(PWM_01_NoAnalogWriteWhenPinModeIsNotSetToOutput) {
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    analogWrite(pin, 50);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 50);
    //To Do : Add test for remaining pins if required
}

test(PWM_02_NoAnalogWriteWhenPinSelectedIsNotTimerChannel) {
    pin_t pin = D5;//pin under test
    // when
    pinMode(pin, OUTPUT);//D5 is not a Timer channel
    analogWrite(pin, 100);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertNotEqual(HAL_PWM_Get_Frequency_Ext(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_03_NoAnalogWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = 51; // pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT); // will simply return
    analogWrite(pin, 100); // will simply return
    // when pinAvailable is checked with a bad pin,
    // then it returns 0
    assertEqual(pinAvailable(pin), 0);
}

template <typename F> void for_all_pwm_pins(F callback)
{
	for (uint8_t i = 0; i<arraySize(pwm_pins); i++)
	{
		callback(pwm_pins[i]);
	}
}

test(PWM_04_AnalogWriteOnPinResultsInCorrectFrequency) {
    for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 150);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertEqual(HAL_PWM_Get_Frequency_Ext(pin), TIM_PWM_FREQ);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9);
    // then
    assertEqual(HAL_PWM_Get_Frequency_Ext(pin), TIM_PWM_FREQ);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900);
    // then
    assertEqual(HAL_PWM_Get_Frequency_Ext(pin), TIM_PWM_FREQ);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900);
    // then
    assertEqual(HAL_PWM_Get_Frequency_Ext(pin), TIM_PWM_FREQ);

    pinMode(pin, INPUT);
    });
}

test(PWM_05_AnalogWriteOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
	pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
	analogWrite(pin, 200);
	// then
	assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 200);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 9);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 1900);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 15900);

	pinMode(pin, INPUT);
	});
}

test(PWM_06_AnalogWriteWithFrequencyOnPinResultsInCorrectFrequency) {
	for_all_pwm_pins([](uint16_t pin) {

	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 150, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)HAL_PWM_Get_Frequency_Ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)HAL_PWM_Get_Frequency_Ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)HAL_PWM_Get_Frequency_Ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)HAL_PWM_Get_Frequency_Ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

	pinMode(pin, INPUT);
	});
}

test(PWM_07_AnalogWriteWithFrequencyOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 200, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 200);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 9);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 1900);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue_Ext(pin), 15900);

    pinMode(pin, INPUT);
	});
}

test(PWM_08_LowDCAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

    // when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
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

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    avgPulseHigh = 0;
    for(int i=0;i<100;i++) {
        analogWrite(pin, 2); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 100;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 80
    assertMoreOrEqual(avgPulseHigh, 120);
    assertLessOrEqual(avgPulseHigh, 280);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    avgPulseHigh = 0;
    for(int i=0;i<100;i++) {
        analogWrite(pin, 409); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 100;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 50
    assertMoreOrEqual(avgPulseHigh, 150);
    assertLessOrEqual(avgPulseHigh, 250);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    avgPulseHigh = 0;
    for(int i=0;i<100;i++) {
        analogWrite(pin, 3277); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
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

test(PWM_09_LowFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    uint32_t avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 25, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    // then
    // avgPulseHigh should equal 10000 +/- 50
    assertMoreOrEqual(avgPulseHigh, 9050);
    assertLessOrEqual(avgPulseHigh, 10050);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 2, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    // then
    // avgPulseHigh should equal 10000 +/- 4000
    assertMoreOrEqual(avgPulseHigh, 6000);
    assertLessOrEqual(avgPulseHigh, 14000);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 409, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    // then
    // avgPulseHigh should equal 10000 +/- 50
    assertMoreOrEqual(avgPulseHigh, 9050);
    assertLessOrEqual(avgPulseHigh, 10050);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    avgPulseHigh = 0;
    for(int i=0;i<5;i++) {
        analogWrite(pin, 3277, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 5;
    // then
    // avgPulseHigh should equal 10000 +/- 50
    assertMoreOrEqual(avgPulseHigh, 9050);
    assertLessOrEqual(avgPulseHigh, 10050);

    analogWrite(pin, 0, 10);
    pinMode(pin, INPUT);
	});
}

test(PWM_10_HighFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    uint32_t avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 25, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    // then
    // avgPulseHigh should equal 10 +/- 5
    assertMoreOrEqual(avgPulseHigh, 5);
    assertLessOrEqual(avgPulseHigh, 15);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 2, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    // then
    // avgPulseHigh should equal 10 +/- 5
    assertMoreOrEqual(avgPulseHigh, 5);
    assertLessOrEqual(avgPulseHigh, 15);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 409, 1000); // 10% Duty Cycle at 1kHz = 100us HIGH, 900us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    // then
    // avgPulseHigh should equal 100 +/- 20
    assertMoreOrEqual(avgPulseHigh, 80);
    assertLessOrEqual(avgPulseHigh, 120);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    avgPulseHigh = 0;
    for(int i=0;i<1000;i++) {
        analogWrite(pin, 3277, 1000); // 10% Duty Cycle at 1kHz = 100us HIGH, 900us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 1000;
    // then
    // avgPulseHigh should equal 100 +/- 20
    assertMoreOrEqual(avgPulseHigh, 80);
    assertLessOrEqual(avgPulseHigh, 120);

    analogWrite(pin, 0, 1000);
    pinMode(pin, INPUT);
	});
}

#if (PLATFORM_ID == 8) // P1
test(PWM_11_P1S6CleanupForP1) {
    // enable POWERSAVE_CLOCK on P1S6
    System.enableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
}
#endif
