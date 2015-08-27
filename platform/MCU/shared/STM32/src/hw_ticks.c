/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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


#include "hw_ticks.h"
#include <limits.h>

/**
 * The current millisecond counter value.
 */
static volatile system_tick_t system_millis = 0;

/**
 * The system clock value at last time system_millis was updated. This is updated
 * after system_millis is incremented.
 */
static volatile system_tick_t system_millis_clock = 0;

/**
 * Increment the millisecond tick counter.
 */
void System1MsTick(void)
{
    system_millis++;
    system_millis_clock = DWT->CYCCNT;
}

/**
 * Fetches the current millisecond tick counter.
 * @return
 */
system_tick_t GetSystem1MsTick()
{
    return system_millis;
}

/**
 * Fetches the current microsecond tick counter.
 * @return
 */
system_tick_t GetSystem1UsTick()
{
    system_tick_t base_millis;
    system_tick_t base_clock;

    // these values need to be fetched consistently - if system_millis changes after fetching
    // (due to the millisecond counter being updated), try again.
    do {
        base_millis = system_millis;
        base_clock = system_millis_clock;
    }
    while (base_millis!=system_millis);

    system_tick_t elapsed_since_millis = ((DWT->CYCCNT-base_clock) / SYSTEM_US_TICKS);
    return (base_millis * 1000) + elapsed_since_millis;
}

/**
 * Testing method that simulates advancing the time forward.
 */
void __advance_system1MsTick(system_tick_t millis, system_tick_t micros_from_rollover)
{
    DWT->CYCCNT = UINT_MAX-(micros_from_rollover*SYSTEM_US_TICKS);   // 10 seconds before rollover
    system_millis = millis;
}

void SysTick_Disable() {
    SysTick->CTRL = SysTick->CTRL & ~SysTick_CTRL_ENABLE_Msk;
}


