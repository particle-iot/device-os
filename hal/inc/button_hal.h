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
    BUTTON1 = 0,
    BUTTON1_MIRROR = 1
} Button_TypeDef;

typedef enum {
    BUTTON_MODE_GPIO = 0,
    BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void BUTTON_Uninit();
void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState);
uint8_t BUTTON_GetState(Button_TypeDef Button);
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button);
void BUTTON_ResetDebouncedState(Button_TypeDef Button);

void BUTTON_Init_Ext();
uint8_t BUTTON_Is_Pressed(Button_TypeDef button);
uint16_t BUTTON_Pressed_Time(Button_TypeDef button);

void BUTTON_Irq_Handler(uint16_t irqn);
void BUTTON_Check_Irq(uint16_t button, uint16_t irqn);
void BUTTON_Check_State(uint16_t button, uint8_t pressed);
int BUTTON_Debounce();

void BUTTON_Timer_Handler(void);

extern button_config_t HAL_Buttons[];

#ifdef __cplusplus
}
#endif

#endif