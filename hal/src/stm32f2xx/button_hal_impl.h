/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#ifndef BUTTON_HAL_IMPL_H
#define BUTTON_HAL_IMPL_H

#include "stm32f2xx.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    __IO uint8_t active;
    GPIO_TypeDef* port;
    uint16_t pin;
    union {
        uint16_t clk;
        uint16_t hal_pin;
    };
    union {
        GPIOMode_TypeDef mode;
        uint16_t interrupt_mode;
    };
    GPIOPuPd_TypeDef pupd;
    __IO uint16_t debounce_time;
    uint16_t exti_line;
    uint16_t exti_port_source;
    uint16_t exti_pin_source;
    uint16_t exti_irqn;
    uint16_t exti_irq_prio;
    EXTITrigger_TypeDef exti_trigger;
} button_config_t;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BUTTON_HAL_IMPL_H
