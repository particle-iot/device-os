/**
 ******************************************************************************
 * @file    tone.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   TONE test application
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
#include "tone_hal.h"
#include "unit-test/unit-test.h"

test(TONE_NoGenerateWhenPinSelectedIsNotTimerChannel) {
    uint8_t pin = D5;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertNotEqual(HAL_Tone_Is_Stopped(pin), false);
    //To Do : Add test for remaining pins if required
}

test(TONE_NoGenerateWhenPinSelectedIsOutOfRange) {
    pin_t pin = 25;//pin under test (not a valid user pin)
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertNotEqual(HAL_Tone_Is_Stopped(pin), false);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinResultsInCorrectFrequency) {
    pin_t pin = A0;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Get_Frequency(pin), frequency);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinResultsInCorrectDuration) {
    pin_t pin = A1;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    // when
    delay(110);//delay for > 100 ms
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), true);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinStopsWhenStopped) {
    pin_t pin = A1;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    // when
    noTone(pin);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), true);
    //To Do : Add test for remaining pins if required
}

