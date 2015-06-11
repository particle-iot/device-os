/**
 ******************************************************************************
 * @file    pinmap_hal.c
 * @authors Satish Nair, Brett Walach, Matthew McGowan
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
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include <stddef.h>

#define NONE CHANNEL_NONE

/* Private typedef -----------------------------------------------------------*/

STM32_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_peripheral (GPIOA, GPIOB or GPIOC)
 * gpio_pin (0-15)
 * gpio_pin_source (GPIO_PinSource0 - GPIO_PinSource15)
 * adc_channel (ADC_Channel_xx or NONE. Note we don't define the peripheral because our chip only has one)
 * dac_channel (DAC_Channel_xx or NONE)
 * timer_peripheral (TIM1 - TIM5, or NONE)
 * timer_ch (1-3, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 * timer_ccr (0 by default, store the CCR value for TIM interrupt use)
 * user_property (0 by default, user variable storage)
 */
  { GPIOB, GPIO_Pin_7, GPIO_PinSource7, NONE, NONE, TIM4, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_6, GPIO_PinSource6, NONE, NONE, TIM4, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_5, GPIO_PinSource5, NONE, NONE, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_4, GPIO_PinSource4, NONE, NONE, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_3, GPIO_PinSource3, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_15, GPIO_PinSource15, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_14, GPIO_PinSource14, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_13, GPIO_PinSource13, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_8, GPIO_PinSource8, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_9, GPIO_PinSource9, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_5, GPIO_PinSource5, ADC_Channel_15, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_3, GPIO_PinSource3, ADC_Channel_13, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_2, GPIO_PinSource2, ADC_Channel_12, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_5, GPIO_PinSource5, ADC_Channel_5, DAC_Channel_2, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_6, GPIO_PinSource6, ADC_Channel_6, NONE, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_7, GPIO_PinSource7, ADC_Channel_7, NONE, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_4, GPIO_PinSource4, ADC_Channel_4, DAC_Channel_1, NULL, NONE, PIN_MODE_NONE, 0, 0 }, // need to define DAC
  { GPIOA, GPIO_Pin_0, GPIO_PinSource0, ADC_Channel_0, NONE, TIM5, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_10, GPIO_PinSource10, ADC_Channel_3, NONE, TIM1, TIM_Channel_3, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_9, GPIO_PinSource9, ADC_Channel_2, NONE, TIM1, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_7, GPIO_PinSource7, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }
};


STM32_Pin_Info* HAL_Pin_Map() {
    return __PIN_MAP;
} 