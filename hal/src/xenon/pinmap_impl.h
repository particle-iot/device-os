/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef PINMAP_IMPL_H
#define	PINMAP_IMPL_H


#ifdef	__cplusplus
extern "C" {
#endif

#include "pinmap_hal.h"

typedef struct NRF5x_Pin_Info {
    uint8_t     gpio_port;      // port0: 0; port: 1;
    uint8_t     gpio_pin;       // range: 0~31;
    PinMode     pin_mode;       // GPIO pin mode
    PinFunction pin_func;       
    uint8_t     adc_channel;
    uint8_t     pwm_instance;   // 4 instances on nRF52, range: 0~3
    uint8_t     pwm_channel;    // 4 channels in each instance, range: 0~3
    uint8_t     exti_channel;   // 16 channels
} NRF5x_Pin_Info;

#define PIN_INVALID         ((uint8_t)(0xFF))

#define NRF_PORT_NONE       ((uint8_t)(0xFF))
#define NRF_PORT_0          ((uint8_t)(0))
#define NRF_PORT_1          ((uint8_t)(1))

#define CHANNEL_NONE        ((uint8_t)(0xFF))
#define ADC_CHANNEL_NONE    CHANNEL_NONE
#define DAC_CHANNEL_NONE    CHANNEL_NONE
#define PWM_INSTANCE_NONE   ((uint8_t)(0xFF))
#define PWM_CHANNEL_NONE    CHANNEL_NONE
#define EXTI_CHANNEL_NONE   CHANNEL_NONE

extern NRF5x_Pin_Info* HAL_Pin_Map(void);

#ifdef	__cplusplus
}
#endif

#endif
