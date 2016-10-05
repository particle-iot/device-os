/**
 ******************************************************************************
 * @file    pwm_hal.c
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
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

/* Includes ------------------------------------------------------------------*/
#include "pwm_hal.h"
#include "pinmap_impl.h"
#define NAMESPACE_WPI_PINMODE
#include "wiringPi.h"

#define WPI_DEFAULT_PWM_DIVISOR 37
#define WPI_DEFAULT_PWM_FREQUENCY 500
#define WPI_DEFAULT_MAX_PWM_DIVISOR 4095
#define WPI_DEFAULT_MIN_PWM_DIVISOR 2

uint16_t wpiPWMFrequency = WPI_DEFAULT_PWM_FREQUENCY;

inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin)
{
    return pin<TOTAL_PINS;
}

void ensurePinModePWM(uint16_t pin) {
    RPi_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (PIN_MAP[pin].pin_mode != AF_OUTPUT_PWM) {
        pinModePi(pin, WPI_PWM_OUTPUT);
        PIN_MAP[pin].pin_mode = AF_OUTPUT_PWM;
    }
}

void ensurePWMFrequency(uint16_t frequency) {
    if (wpiPWMFrequency != frequency && frequency > 0) {
        uint32_t divisor = ((uint32_t)WPI_DEFAULT_PWM_FREQUENCY * WPI_DEFAULT_PWM_DIVISOR) / frequency;
        divisor = divisor > WPI_DEFAULT_MAX_PWM_DIVISOR ? WPI_DEFAULT_MAX_PWM_DIVISOR : divisor;
        divisor = divisor < WPI_DEFAULT_MIN_PWM_DIVISOR ? WPI_DEFAULT_MIN_PWM_DIVISOR : divisor;
        pwmSetClock((int)divisor);
        wpiPWMFrequency = frequency;
    }
}

void HAL_PWM_Write(uint16_t pin, uint8_t value)
{
    HAL_PWM_Write_With_Frequency(pin, value, WPI_DEFAULT_PWM_FREQUENCY);
}

void HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency)
{
    if (!is_valid_pin(pin)) {
        return;
    }
    ensurePinModePWM(pin);
    ensurePWMFrequency(pwm_frequency);
    int valuePi = (int)value * 4; // range of pwmWrite is 0-1023
    pwmWrite(pin, valuePi);
}

uint16_t HAL_PWM_Get_Frequency(uint16_t pin)
{
    return wpiPWMFrequency;
}

uint16_t HAL_PWM_Get_AnalogValue(uint16_t pin)
{
    return 0;
}
