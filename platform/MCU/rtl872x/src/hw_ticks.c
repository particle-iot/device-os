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

#include "hw_ticks.h"
#include "timer_hal.h"

void System1MsTick(void)
{
    // Unsupported
}

system_tick_t GetSystem1MsTick()
{
    return (system_tick_t)GetSystem1MsTick64();
}

uint64_t GetSystem1MsTick64()
{
    return hal_timer_millis(NULL);
}

/**
 * Fetches the current microsecond tick counter.
 * @return
 */
system_tick_t GetSystem1UsTick()
{
    return (system_tick_t)hal_timer_micros(NULL);
}

/**
 * Testing method that simulates advancing the time forward.
 */
void __advance_system1MsTick(uint64_t millis, system_tick_t micros_from_rollover)
{
    // Unsupported
}
