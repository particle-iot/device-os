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
#ifndef __PWM_HAL_H
#define __PWM_HAL_H

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


// Deprecated *dynalib* APIs for backwards compatibility
void inline __attribute__((always_inline, deprecated("Use hal_pwm_write() instead"))) HAL_PWM_Write(uint16_t pin, uint8_t value) { hal_pwm_write(pin, value); }
void inline __attribute__((always_inline, deprecated("Use hal_pwm_write_ext() instead"))) HAL_PWM_Write_Ext(uint16_t pin, uint32_t value) { hal_pwm_write_ext(pin, value); }
void inline __attribute__((always_inline, deprecated("Use hal_pwm_write_with_frequency() instead"))) HAL_PWM_Write_With_Frequency(uint16_t pin, uint8_t value, uint16_t frequency) { hal_pwm_write_with_frequency(pin, value, frequency); }
void inline __attribute__((always_inline, deprecated("Use hal_pwm_write_with_frequency_ext() instead"))) HAL_PWM_Write_With_Frequency_Ext(uint16_t pin, uint32_t value, uint32_t frequency) { hal_pwm_write_with_frequency_ext(pin, value, frequency); }
uint16_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_frequency() instead"))) HAL_PWM_Get_Frequency(uint16_t pin) { return hal_pwm_get_frequency(pin); }
uint32_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_frequency_ext() instead"))) HAL_PWM_Get_Frequency_Ext(uint16_t pin) { return hal_pwm_get_frequency_ext(pin); }
uint16_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_analog_value() instead"))) HAL_PWM_Get_AnalogValue(uint16_t pin) { return hal_pwm_get_analog_value(pin); }
uint32_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_analog_value_ext() instead"))) HAL_PWM_Get_AnalogValue_Ext(uint16_t pin) { return hal_pwm_get_analog_value_ext(pin); }
uint32_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_max_frequency() instead"))) HAL_PWM_Get_Max_Frequency(uint16_t pin) { return hal_pwm_get_max_frequency(pin); }
uint8_t inline __attribute__((always_inline, deprecated("Use hal_pwm_get_resolution() instead"))) HAL_PWM_Get_Resolution(uint16_t pin) { return hal_pwm_get_resolution(pin); }
void inline __attribute__((always_inline, deprecated("Use hal_pwm_set_resolution() instead"))) HAL_PWM_Set_Resolution(uint16_t pin, uint8_t resolution) { hal_pwm_set_resolution(pin, resolution); }

#ifdef __cplusplus
}
#endif

#endif  /* __PWM_HAL_H */
