/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pinmap_hal.h"

typedef struct hal_pin_info_t {
    uint8_t             gpio_port;
    uint8_t             gpio_pin;
    PinMode             pin_mode;
    PinFunction         pin_func;
    uint8_t             adc_channel;
    uint8_t             pwm_instance;
    uint8_t             pwm_channel;
#if HAL_PLATFORM_IO_EXTENSION
    hal_pin_type_t      type;
#endif // HAL_PLATFORM_IO_EXTENSION
    uint32_t     user_data;
} hal_pin_info_t;

#define RTL_PORT_NONE       ((uint8_t)(0xFF))
#define RTL_PORT_A          ((uint8_t)(0))
#define RTL_PORT_B          ((uint8_t)(1))

#define CHANNEL_NONE        ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE    CHANNEL_NONE
#define DAC_CHANNEL_NONE    CHANNEL_NONE
#define PWM_INSTANCE_NONE   ((uint8_t)(0xFF))
#define PWM_CHANNEL_NONE    CHANNEL_NONE
#define EXTI_CHANNEL_NONE   CHANNEL_NONE

#define DEFAULT_PWM_FREQ    500 // 500Hz
#define TIM_PWM_FREQ        DEFAULT_PWM_FREQ

#include "pinmap_defines.h"

uint32_t hal_pin_to_rtl_pin(hal_pin_t pin);

#ifdef __cplusplus
}
#endif
