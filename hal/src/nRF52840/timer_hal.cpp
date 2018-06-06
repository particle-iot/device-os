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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "hw_config.h"
#include "core_hal.h"
#include "timer_hal.h"
#include "syshealth_hal.h"

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return GetSystem1UsTick();
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return GetSystem1MsTick();
}

uint64_t hal_timer_millis(void* reserved)
{
    return GetSystem1MsTick64();
}
