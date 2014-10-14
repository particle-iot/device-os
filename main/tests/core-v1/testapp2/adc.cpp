/**
 ******************************************************************************
 * @file    adc.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   ADC test application
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

test(ADC_NoAnalogReadWhenPinSelectedIsOutOfRange) {
    pin_t pin = 23;//pin under test (not a valid user pin)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertFalse(ADCValue!=0);
    //To Do : Add test for remaining pins if required
}

test(ADC_AnalogReadOnPinWithVoltageDividerResultsInCorrectValue) {
    pin_t pin = A5;//pin under test (Voltage divider with equal resistor values)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertTrue((ADCValue>2000)&&(ADCValue<2100));//ADCValue should be around 2048
    //To Do : Add test for remaining pins if required
}
