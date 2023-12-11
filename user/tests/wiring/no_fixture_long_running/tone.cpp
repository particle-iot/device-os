/**
 ******************************************************************************
 * @file    tone.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   TONE test application
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
#include "tone_hal.h"
#include "unit-test/unit-test.h"

#if PLATFORM_ID == PLATFORM_P2
static const hal_pin_t pin = D1;//pin under test
#elif PLATFORM_ID == PLATFORM_TRACKERM
static const hal_pin_t pin = A2;//pin under test
#elif PLATFORM_ID == PLATFORM_ESOMX
static const hal_pin_t pin = A3;//pin under test
#else
static const hal_pin_t pin = A1;//pin under test
#endif

test(TONE_01_NoGenerateWhenPinSelectedIsNotTimerChannel) {
#if HAL_PLATFORM_NRF52840
# if PLATFORM_ID == PLATFORM_TRACKER
    hal_pin_t pin = BTN;
# elif PLATFORM_ID == PLATFORM_ESOMX
    hal_pin_t pin = A0;
# else
    hal_pin_t pin = D0;
# endif
#elif HAL_PLATFORM_RTL872X
#if PLATFORM_ID == PLATFORM_MSOM
    hal_pin_t pin = D0;
#else
    hal_pin_t pin = D5;
#endif
#else
#error "Unsupported platform"
#endif
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertNotEqual(HAL_Tone_Is_Stopped(pin), false);
    //To Do : Add test for remaining pins if required
}

test(TONE_02_GeneratedOnPinResultsInCorrectFrequency) {
    uint32_t frequency = 500;
    uint32_t duration = 100;
    ATOMIC_BLOCK() {
        // when
        tone(pin, frequency, duration);
        // then
        assertEqual(HAL_Tone_Get_Frequency(pin), frequency);
        //To Do : Add test for remaining pins if required
    }
}

test(TONE_03_GeneratedOnPinResultsInCorrectDuration) {
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

test(TONE_04_GeneratedOnPinStopsWhenStopped) {
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

test(TONE_05_ZeroDurationInfiniteToneDuration) {
    uint32_t frequency = 500;
    uint32_t duration = 0;
    tone(pin, frequency, duration);
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    delay(1000);
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    noTone(pin);
    assertEqual(HAL_Tone_Is_Stopped(pin), true);
}
