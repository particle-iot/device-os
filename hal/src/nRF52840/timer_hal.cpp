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

#include <stdint.h>
#include "timer_hal.h"

// NOTE: hal_timer_init(), hal_timer_millis() and hal_timer_micros() are
// implemented in OpenThread platform code (alarm.c). We are re-using alarm implementation
// necessary for OpenThread internal functionality for our internal needs.
// It already provides a proper 64-bit stable monotonic microsecond counter, so we use that
// to derive 64-bit milliseconds out of it, as well as use it in rtc HAL.

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return (system_tick_t)hal_timer_micros(nullptr);
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return (system_tick_t)hal_timer_millis(nullptr);
}
