/**
 ******************************************************************************
 * @file    pinmap_hal.c
 * @authors Satish Nair, Brett Walach, Matthew McGowan
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

/* Private typedef -----------------------------------------------------------*/

STM32_Pin_Info PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_peripheral (GPIOA, GPIOB or GPIOC)
 * gpio_pin (0-15)
 * adc_channel (0-15 or NONE. Note we don't define the peripheral because our chip only has one)
 * timer_peripheral (TIM1 - TIM5, or NONE)
 * timer_ch (1-3, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 * timer_ccr (0 by default, store the CCR value for TIM interrupt use)
 * user_property (0 by default, user variable storage)
 */
  { GPIOB, GPIO_Pin_7, NONE, TIM4, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_6, NONE, TIM4, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_5, NONE, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_4, NONE, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOB, GPIO_Pin_3, NONE, TIM2, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_15, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_14, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_13, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_8, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_9, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_5, ADC_Channel_15, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_3, ADC_Channel_13, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_2, ADC_Channel_12, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_5, ADC_Channel_5, TIM2, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_6, ADC_Channel_6, TIM3, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_7, ADC_Channel_7, TIM3, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_4, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }, // need to define DAC
  { GPIOA, GPIO_Pin_0, NONE, TIM5, TIM_Channel_1, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_10, ADC_Channel_3, TIM1, TIM_Channel_3, PIN_MODE_NONE, 0, 0 },
  { GPIOA, GPIO_Pin_9, ADC_Channel_2, TIM1, TIM_Channel_2, PIN_MODE_NONE, 0, 0 },
  { GPIOC, GPIO_Pin_7, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }
};


