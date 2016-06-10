/**
 ******************************************************************************
 * @file    adc.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   ADC test application
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
#include <stdlib.h>

/*
 * ADC Test requires two 100k ohm resistors hooked up to the unit under test as follows:
 * only on Core (PLATFORM_ID = 0)
 *
 *           R1 100k           R2 100k
 * (3V3*) ---/\/\/\--- (A5) ---/\/\/\--- (GND)
 *
 * On Photon, connect DAC Output to ADC input as follows:
 *
 *           WIRE
 * (DAC) --==========-- (A5)
 *
 *           WIRE
 * (DAC2/A3) --==========-- (A1)
 *
 */

test(ADC_NoAnalogReadWhenPinSelectedIsOutOfRange) {
    pin_t pin = 23;//pin under test (not a valid user pin)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertFalse(ADCValue!=0);
    //To Do : Add test for remaining pins if required
}

#if (PLATFORM_ID == 0)
test(ADC_AnalogReadOnPinWithVoltageDividerResultsInCorrectValue) {
    pin_t pin = A5;//pin under test (Voltage divider with equal resistor values)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertTrue((ADCValue>2000)&&(ADCValue<2100));//ADCValue should be around 2048
    //To Do : Add test for remaining pins if required
}
#endif


#if (PLATFORM_ID >= 3)
test(ADC_AnalogReadOnPinWithDACOutputResultsInCorrectValue) {
    pin_t pin = A5;//pin under test
    // when
    analogWriteResolution(DAC1, 12);
    assertEqual(analogWriteResolution(DAC1), 12);
    analogWrite(DAC1, 2048);
    int32_t ADCValue = analogRead(pin);
    // then
    assertTrue((ADCValue>2000)&&(ADCValue<2100));//ADCValue should be around 2048
    //To Do : Add test for remaining pins if required
}
#endif

#if (PLATFORM_ID >= 3)
test(ADC_AnalogReadOnPinWithDACShmoo) {
    pin_t pin1 = A5;
    pin_t pin2 = A1;
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);

    analogWriteResolution(DAC, 12);
    analogWriteResolution(DAC2, 12);

    assertEqual(analogWriteResolution(DAC), 12);
    assertEqual(analogWriteResolution(DAC2), 12);

    // In Github issues #671, following code will cause DAC output to stuck a constant value
    for(int i = 0; i < 4095; i++) {
        pinMode(DAC, OUTPUT);
        analogWrite(DAC, i);
    }
    for(int i = 0; i < 4095; i++) {
        pinMode(DAC2, OUTPUT);
        analogWrite(DAC2, i);
    }
    delay(100);

    // Shmoo test
    // Set DAC/DAC2 output from 200 to 3896 (ignore non-linearity near 0V or VDD)
    // Read back analog and compare for significant difference
    int errorCount = 0;
    for(int i = 200; i < 3896; i++){
        analogWrite(DAC, i);
        analogWrite(DAC2, 4095-i);
    	if (abs(analogRead(pin1)-i) > 200) {
    		errorCount ++;
    	}
        else if (abs(analogRead(pin2) - 4095 + i) > 200) {
    		errorCount ++;
    	}
    }
    assertTrue(errorCount == 0);
}

test(ADC_AnalogReadOnPinWithDACMixedWithDigitalWrite) {
    pin_t pin1 = A5;
    pin_t pin2 = A1;
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);

    analogWriteResolution(DAC, 12);
    analogWriteResolution(DAC2, 12);

    assertEqual(analogWriteResolution(DAC), 12);
    assertEqual(analogWriteResolution(DAC2), 12);

    // Tests for issue #833 where digitalWrite after analogWrite on DAC pin
    // causes pin to latch at last analogWrite value

    // Shmoo test
    // Set DAC/DAC2 output from 200 to 3896 (ignore non-linearity near 0V or VDD)
    // Read back analog and compare for significant difference
    int errorCount = 0;
    for(int i = 200; i < 3896; i++){
        analogWrite(DAC, i);
        analogWrite(DAC2, 4095-i);
        if (abs(analogRead(pin1)-i) > 200) {
            errorCount ++;
        }
        else if (abs(analogRead(pin2) - 4095 + i) > 200) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);

    digitalWrite(DAC, LOW);
    digitalWrite(DAC2, HIGH);
    assertFalse(abs(analogRead(pin1) - 0) > 128);
    assertFalse(abs(analogRead(pin2) - 4095) > 128);

    // Tests that digitalWrite on DAC doesn't influence analog value on DAC2
    // and vice versa
    errorCount = 0;
    for(int i = 200; i < 3896; i++){
        analogWrite(DAC, i);
        digitalWrite(DAC2, !digitalRead(DAC2));
        if (abs(analogRead(pin1)-i) > 200) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);

    errorCount = 0;
    for(int i = 200; i < 3896; i++){
        analogWrite(DAC2, 4095-i);
        digitalWrite(DAC, !digitalRead(DAC));
        if (abs(analogRead(pin2) - 4095 + i) > 200) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);
}

test(ADC_AnalogReadOnPinWithDACShmoo8bit) {
    pin_t pin1 = A5;
    pin_t pin2 = A1;
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);

    analogWriteResolution(DAC, 8);
    analogWriteResolution(DAC2, 8);

    assertEqual(analogWriteResolution(DAC), 8);
    assertEqual(analogWriteResolution(DAC2), 8);

    // In Github issues #671, following code will cause DAC output to stuck a constant value
    for(int i = 0; i < 255; i++) {
        pinMode(DAC, OUTPUT);
        analogWrite(DAC, i);
    }
    for(int i = 0; i < 255; i++) {
        pinMode(DAC2, OUTPUT);
        analogWrite(DAC2, i);
    }
    delay(100);

    // Shmoo test
    // Set DAC/DAC2 output from 20 to 230 (ignore non-linearity near 0V or VDD)
    // Read back analog and compare for significant difference
    int errorCount = 0;
    for(int i = 20; i < 230; i++){
        analogWrite(DAC, i);
        analogWrite(DAC2, 255-i);
        if (abs(analogRead(pin1)*255/4095-i) > 15) {
            errorCount ++;
        }
        else if (abs(analogRead(pin2)*255/4095 - 255 + i) > 15) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);
}

test(ADC_AnalogReadOnPinWithDACMixedWithDigitalWrite8bit) {
    pin_t pin1 = A5;
    pin_t pin2 = A1;
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);

    analogWriteResolution(DAC, 8);
    analogWriteResolution(DAC2, 8);

    assertEqual(analogWriteResolution(DAC), 8);
    assertEqual(analogWriteResolution(DAC2), 8);

    // Tests for issue #833 where digitalWrite after analogWrite on DAC pin
    // causes pin to latch at last analogWrite value

    // Shmoo test
    // Set DAC/DAC2 output from 20 to 230 (ignore non-linearity near 0V or VDD)
    // Read back analog and compare for significant difference
    int errorCount = 0;
    for(int i = 20; i < 230; i++){
        analogWrite(DAC, i);
        analogWrite(DAC2, 255-i);
        if (abs(analogRead(pin1)*255/4095-i) > 15) {
            errorCount ++;
        }
        else if (abs(analogRead(pin2)*255/4095 - 255 + i) > 15) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);

    digitalWrite(DAC, LOW);
    digitalWrite(DAC2, HIGH);
    assertFalse(abs(analogRead(pin1)*255/4095 - 0) > 10);
    assertFalse(abs(analogRead(pin2)*255/4095 - 255) > 10);

    // Tests that digitalWrite on DAC doesn't influence analog value on DAC2
    // and vice versa
    errorCount = 0;
    for(int i = 20; i < 230; i++){
        analogWrite(DAC, i);
        digitalWrite(DAC2, !digitalRead(DAC2));
        if (abs(analogRead(pin1)*255/4095-i) > 15) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);

    errorCount = 0;
    for(int i = 20; i < 230; i++){
        analogWrite(DAC2, 255-i);
        digitalWrite(DAC, !digitalRead(DAC));
        if (abs(analogRead(pin2)*255/4095 - 255 + i) > 15) {
            errorCount ++;
        }
    }
    assertTrue(errorCount == 0);
}

test(DAC_AnalogWriteWorksMixedWithDigitalRead) {
    pin_t pin = DAC;

    // when
    pinMode(pin, INPUT_PULLUP);
    // then
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);

    // 2 analogReads
    analogWrite(pin, 1000);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_OUTPUT);
    analogWrite(pin, 2000);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_OUTPUT);
    // 2 digitalReads
    digitalRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);
    digitalRead(pin);
    assertEqual(HAL_Get_Pin_Mode(pin), INPUT_PULLUP);
    // 2 analogReads again
    analogWrite(pin, 1000);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_OUTPUT);
    analogWrite(pin, 500);
    assertEqual(HAL_Get_Pin_Mode(pin), AN_OUTPUT);
}


#endif
