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
 * gpio_peripheral : ???
 * gpio_pin : ???
 * adc_channel : ???
 * timer_peripheral : ???
 * timer_ch : ???
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 * timer_ccr (0 by default, store the CCR value for TIM interrupt use)
 * user_property (0 by default, user variable storage)
 */
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 },
  { NULL, NONE, NONE, NULL, NONE, PIN_MODE_NONE, 0, 0 }
};


