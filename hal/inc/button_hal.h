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

#ifndef _BUTTON_HAL_H
#define _BUTTON_HAL_H

#include <stdint.h>
#include "button_hal_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTONn                             1

typedef enum {
    HAL_BUTTON1 = 0,
    HAL_BUTTON1_MIRROR = 1
} hal_button_t;

typedef enum {
    HAL_BUTTON_MODE_GPIO = 0,
    HAL_BUTTON_MODE_EXTI = 1
} hal_button_mode_t;

void hal_button_init(hal_button_t button, hal_button_mode_t mode);
void hal_button_uninit();
void hal_button_exti_config(hal_button_t button, FunctionalState state);
uint8_t hal_button_get_state(hal_button_t button);
uint16_t hal_button_get_debounce_time(hal_button_t button);
void hal_button_reset_debounced_state(hal_button_t button);

void hal_button_init_ext();
uint8_t hal_button_is_pressed(hal_button_t button);
uint16_t hal_button_get_pressed_time(hal_button_t button);

void hal_button_irq_handler(uint16_t irqn);
void hal_button_check_irq(uint16_t button, uint16_t irqn);
int hal_button_debounce();

void hal_button_timer_handler(void);

extern hal_button_config_t HAL_buttons[];

#ifdef __cplusplus
}
#endif

#endif