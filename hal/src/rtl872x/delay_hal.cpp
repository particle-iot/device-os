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

#define NO_STATIC_ASSERT
#include "delay_hal.h"
extern "C" {
#include "rtl8721d.h"
} // extern "C"
#include "hw_config.h"
#include "watchdog_hal.h"
#include <limits.h>
#include <FreeRTOS.h>
#include <task.h>
#include "timer_hal.h"

void HAL_Delay_Milliseconds(uint32_t nTime)
{
    vTaskDelay(nTime);
}

void HAL_Delay_Microseconds(uint32_t uSec)
{
    const system_tick_t start_micros = HAL_Timer_Get_Micro_Seconds();

    while (HAL_Timer_Get_Micro_Seconds() - start_micros < uSec) {
    }
}
