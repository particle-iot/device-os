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

#ifndef _BUTTON_HAL_IMPL_H
#define _BUTTON_HAL_IMPL_H

#include "nrfx_types.h"
#include "nrf52840.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
typedef struct {
    uint16_t              pin;
    nrf_gpio_pin_dir_t    mode;
    nrf_gpio_pin_pull_t   pupd;
    volatile uint8_t      active;
    volatile uint16_t     debounce_time;
    uint16_t              event_in;
    uint16_t              event_channel;
    uint16_t              int_mask;
    uint16_t              interrupt_mode;
    uint16_t              nvic_irqn;
    uint16_t              nvic_irq_prio;
    uint8_t               padding[12];
} button_config_t;
#else
typedef struct {
    uint16_t              pin;
    uint8_t               interrupt_mode;
    volatile uint8_t      active;
    volatile uint16_t     debounce_time;
    uint8_t               padding[26];
} button_config_t;
#endif /* MODULE_FUNCTION == MOD_FUNC_BOOTLOADER */

#ifdef __cplusplus
}
#endif

#endif
