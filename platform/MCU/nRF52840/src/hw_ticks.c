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
#include "nrf_nvic.h"
#include <limits.h>

/**
 * The current millisecond counter value.
 */
static volatile uint64_t system_millis = 0;

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
    uint8_t is = 0;
    sd_nvic_critical_region_enter(&is);

    ++system_millis;
    system_millis_clock = DWT->CYCCNT;

    sd_nvic_critical_region_exit(is);
}

/**
 * Fetches the current millisecond tick counter.
 * @return
 */
system_tick_t GetSystem1MsTick()
{
    return GetSystem1MsTick64();
}

uint64_t GetSystem1MsTick64()
{
    uint64_t millis = 0;

    uint8_t is = 0;
    sd_nvic_critical_region_enter(&is);

    millis = system_millis + (DWT->CYCCNT - system_millis_clock) / SYSTEM_US_TICKS / 1000;

    sd_nvic_critical_region_exit(is);

    return millis;
}

/**
 * Fetches the current microsecond tick counter.
 * @return
 */
system_tick_t GetSystem1UsTick()
{
    system_tick_t base_millis;
    system_tick_t base_clock;

    uint8_t is = 0;
    sd_nvic_critical_region_enter(&is);

    base_millis = system_millis;
    base_clock = system_millis_clock;

    system_tick_t elapsed_since_millis = ((DWT->CYCCNT-base_clock) / SYSTEM_US_TICKS);

    sd_nvic_critical_region_exit(is);
    return (base_millis * 1000) + elapsed_since_millis;
}

/**
 * Testing method that simulates advancing the time forward.
 */
void __advance_system1MsTick(uint64_t millis, system_tick_t micros_from_rollover)
{
    uint8_t is = 0;
    sd_nvic_critical_region_enter(&is);

    DWT->CYCCNT = UINT_MAX - (micros_from_rollover * SYSTEM_US_TICKS);
    system_millis_clock = DWT->CYCCNT;
    system_millis = millis;

    sd_nvic_critical_region_exit(is);
}

void SysTick_Disable() {
    SysTick->CTRL = SysTick->CTRL & ~SysTick_CTRL_ENABLE_Msk;
}
