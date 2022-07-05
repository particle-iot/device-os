/**
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


#pragma once

#include "platform_config.h"
#include "system_tick_hal.h"

#define SYSTEM_US_TICKS         (SystemCoreClock / 1000000) //cycles per microsecond
#define SYSTEM_TICK_COUNTER     GetDwtCounter()

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t SystemCoreClock;

/**
 * Increment the millisecond tick counter.
 */
void System1MsTick(void);

/**
 * Increment the microsecond tick counter.
 */
void System1UsTick(void);

/**
 * Fetch the current milliseconds count.
 * @return the number of milliseconds since the device was powered on or woken up from
 * sleep. Automatically wraps around when above UINT_MAX;
 */
system_tick_t GetSystem1MsTick();

/**
 * Fetches the milliseconds counter. This function is similar to GetSystem1MsTick() but
 * returns a 64-bit value.
 */
uint64_t GetSystem1MsTick64();

/**
 * Fetch the current microseconds count.
 * @return the number of microseconds since the device was powered on or woken up from
 * sleep. Automatically wraps around when above UINT_MAX;
 */
system_tick_t GetSystem1UsTick();

uint32_t GetDwtCounter();

#ifdef __cplusplus
}
#endif
