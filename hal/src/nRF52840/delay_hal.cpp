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
#include <nrf52840.h>
#include "hw_config.h"
#include "watchdog_hal.h"
#include <limits.h>
#include <FreeRTOS.h>
#include <task.h>

void HAL_Delay_Milliseconds(uint32_t nTime)
{
    vTaskDelay(nTime);
}

void HAL_Delay_Microseconds(uint32_t uSec)
{
    volatile uint32_t DWT_START = DWT->CYCCNT;
    // keep DWT_TOTAL from overflowing (max 59.652323s w/72MHz SystemCoreClock)
    if (uSec > (UINT_MAX / SYSTEM_US_TICKS))
    {
        uSec = (UINT_MAX / SYSTEM_US_TICKS);
    }

    volatile uint32_t DWT_TOTAL = (SYSTEM_US_TICKS * uSec);

    while((DWT->CYCCNT - DWT_START) < DWT_TOTAL)
    {
        HAL_Notify_WDT();
    }
}
