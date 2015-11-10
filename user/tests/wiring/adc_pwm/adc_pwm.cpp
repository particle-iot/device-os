/**
 ******************************************************************************
 * @file    adc_pwm.cpp
 * @authors Peter Qu
 * @version V1.0.0
 * @date    9-Nov-2015
 * @brief   ADC PWM test application
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

/*
 * PWM Test requires connecting D0 to A0
 *
 *           WIRE
 * (D0) --==========-- (A0)
 *
 */

/*
 * @brief repeatably call AnalogWrite at period of microseconds.
 * Analog write is hardcoded to 50% duty cycle on pin D0.
 * Measure the duty cycle on pin A0.
 */
bool pwmRepeatablyAnalogWrite(int period){
	pinMode(D0, OUTPUT);
	pinMode(A0, INPUT);

	//Set to fastest ADC sample time
	#if (PLATFORM_ID == 0)
	setADCSampleTime(ADC_SampleTime_1Cycles5);
	#endif

	#if (PLATFORM_ID >= 3)
	setADCSampleTime(ADC_SampleTime_3Cycles);
	#endif

	unsigned long sum=0;
	for (int i = 0; i<1000; i++){
		// Set PWM to 50% duty cycle
		analogWrite(D0, 128);
		int delay=random(0, period);

		// Read analog at a random time in this cycle
		delayMicroseconds(delay);
		sum+= analogRead(A0);
		delayMicroseconds(period-delay);
	}

	// Measured duty cycle should be close to 50%
	float dutyCycle = float(sum) / float(1000.0*4096.0);
	return (dutyCycle > 0.4 && dutyCycle < 0.6);
}

test(ADC_PWMRepeatablyWriteAt100Hz) {
    assertTrue(pwmRepeatablyAnalogWrite(10000));
}

test(ADC_PWMRepeatablyWriteAt1KHz) {
    assertTrue(pwmRepeatablyAnalogWrite(1000));
}

test(ADC_PWMRepeatablyWriteAt10KHz) {
    assertTrue(pwmRepeatablyAnalogWrite(100));
}

test(ADC_PWMRepeatablyWriteAt100KHz) {
    assertTrue(pwmRepeatablyAnalogWrite(10));
}


