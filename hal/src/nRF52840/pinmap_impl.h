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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pinmap_hal.h"

typedef struct hal_pin_info_t {
    uint8_t      gpio_port; // port0: 0; port: 1;
    uint8_t      gpio_pin;  // range: 0~31;
    PinMode      pin_mode;  // GPIO pin mode
    PinFunction  pin_func;
    uint8_t      adc_channel;
    uint8_t      pwm_instance;   // 4 instances on nRF52, range: 0~3
    uint8_t      pwm_channel;    // 4 channels in each instance, range: 0~3
    uint8_t      pwm_resolution; // default 8bit, max 15bit
    uint8_t      exti_channel;   // 16 channels
#if HAL_PLATFORM_IO_EXTENSION
    hal_pin_type_t type;
#endif // HAL_PLATFORM_IO_EXTENSION
    uint32_t     user_data;
} hal_pin_info_t;

// For compatibility
typedef hal_pin_info_t NRF5x_Pin_Info;

extern const uint8_t NRF_PIN_LOOKUP_TABLE[48];

#define NRF_PORT_NONE ((uint8_t)(0xFF))
#define NRF_PORT_0 ((uint8_t)(0))
#define NRF_PORT_1 ((uint8_t)(1))

#define CHANNEL_NONE ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE CHANNEL_NONE
#define DAC_CHANNEL_NONE CHANNEL_NONE
#define PWM_INSTANCE_NONE ((uint8_t)(0xFF))
#define PWM_CHANNEL_NONE CHANNEL_NONE
#define EXTI_CHANNEL_NONE CHANNEL_NONE

#define DEFAULT_PWM_FREQ 500 // 500Hz
#define TIM_PWM_FREQ DEFAULT_PWM_FREQ

#include "pinmap_defines.h"

inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin) {
    hal_pin_info_t* PIN_MAP = hal_pin_map();
    return (pin < TOTAL_PINS && PIN_MAP[pin].gpio_port != NRF_PORT_NONE && PIN_MAP[pin].gpio_pin != PIN_INVALID);
}

#ifdef __cplusplus
}
#endif
