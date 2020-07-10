/******************************************************************************
 * @file    pwm_hal.h
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************/
/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PWM_HAL_H
#define PWM_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* This perhaps should be moved in a different place. */
typedef enum hal_pwm_user_property_t {
	PWM_INIT = 1,
	/* add other flags here*/
} hal_pwm_user_property_t;

#ifdef __cplusplus
extern "C" {
#endif

void hal_pwm_write(uint16_t pin, uint8_t value);
void hal_pwm_write_ext(uint16_t pin, uint32_t value);
void hal_pwm_write_with_frequency(uint16_t pin, uint8_t value, uint16_t pwm_frequency);
void hal_pwm_write_with_frequency_ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency);
uint16_t hal_pwm_get_frequency(uint16_t pin);
uint32_t hal_pwm_get_frequency_ext(uint16_t pin);
uint16_t hal_pwm_get_analog_value(uint16_t pin);
uint32_t hal_pwm_get_analog_value_ext(uint16_t pin);
uint32_t hal_pwm_get_max_frequency(uint16_t pin);
void hal_pwm_update_duty_cycle(uint16_t pin, uint16_t value);
void hal_pwm_update_duty_cycle_ext(uint16_t pin, uint32_t value);
uint8_t hal_pwm_get_resolution(uint16_t pin);
void hal_pwm_set_resolution(uint16_t pin, uint8_t resolution);
void hal_pwm_reset_pin(uint16_t pin);
int hal_pwm_sleep(bool sleep, void* reserved);


#include "pwm_hal_compat.h"

#ifdef __cplusplus
}
#endif

#endif  /* PWM_HAL_H */
