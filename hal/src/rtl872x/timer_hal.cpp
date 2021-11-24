/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <cstdint>
#include "timer_hal.h"
#include "service_debug.h"
#include "hw_ticks.h"
extern "C" {
#include "rtl8721d.h"
}

// Particle-specific
int hal_timer_init(const hal_timer_init_config_t* conf) {
    return 0;
}

int hal_timer_deinit(void* reserved) {
    return 0;
}

uint64_t hal_timer_micros(void* reserved) {
    // FIXME: wraps in 36 hours
    return SYSTIMER_TickGet() * 31;
}

uint64_t hal_timer_millis(void* reserved) {
    return hal_timer_micros(nullptr) / 1000ULL;
}

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return (system_tick_t)hal_timer_micros(nullptr);
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return (system_tick_t)hal_timer_millis(nullptr);
}
