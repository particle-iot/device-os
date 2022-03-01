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
void hal_pwm_write(uint16_t pin, uint8_t value)
{
}

void hal_pwm_write_with_frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency)
{
}

uint16_t hal_pwm_get_frequency(uint16_t pin)
{
    return 0;
}

uint16_t hal_pwm_get_analog_value(uint16_t pin)
{
    return 0;
}

void hal_pwm_write_ext(uint16_t pin, uint32_t value)
{
}

void hal_pwm_write_with_frequency_ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency)
{
}

uint32_t hal_pwm_get_frequency_ext(uint16_t pin)
{
    return 0;
}

uint32_t hal_pwm_get_analog_value_ext(uint16_t pin)
{
    return 0;
}

uint32_t hal_pwm_get_max_frequency(uint16_t pin)
{
    return 0;
}

void hal_pwm_update_duty_cycle(uint16_t pin, uint16_t value)
{
}

void hal_pwm_update_duty_cycle_ext(uint16_t pin, uint32_t value)
{
}

uint8_t hal_pwm_get_resolution(uint16_t pin)
{
    return 0;
}

void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution)
{
}

int hal_pwm_sleep(bool sleep, void* reserved)
{
    return 0;
}
