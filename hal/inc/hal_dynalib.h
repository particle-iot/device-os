/**
 ******************************************************************************
 * @file    services-dynalib.h
 * @author  Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "dynalib.h"

#include "hal_platform.h"

#include "time_compat.h"

#ifdef DYNALIB_EXPORT
#include "rng_hal.h"
#include "eeprom_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "rtc_hal.h"
#include "interrupts_hal.h"

#if PLATFORM_ID == PLATFORM_TRACKER
#include "exrtc_hal.h"
#endif // PLATFORM_ID == PLATFORM_TRACKER

#endif

/**
 * Define the dynamic exports for the current platform.
 *
 * Note that once used in released binary, the only permissable changes to the dynamic
 * library definition is to append new functions to the end of the list.  This is
 * so the dynamic library maintains backwards compatibility and can continue to
 * be used by older clients.
 *
 * Each platform may have a distinct dynamic library, via conditional compilation
 * but the rule above still holds.
 */

DYNALIB_BEGIN(hal)

#if PLATFORM_ID > PLATFORM_GCC
DYNALIB_FN(0, hal, HAL_RNG_Configuration, void(void))
DYNALIB_FN(1, hal, HAL_RNG_GetRandomNumber, uint32_t(void))
#define BASE_IDX 2 // Base index for all subsequent functions
#else
#define BASE_IDX 0
#endif // PLATFORM_ID > PLATFORM_GCC

DYNALIB_FN(BASE_IDX + 0, hal, HAL_Delay_Milliseconds, void(uint32_t))
DYNALIB_FN(BASE_IDX + 1, hal, HAL_Delay_Microseconds, void(uint32_t))
DYNALIB_FN(BASE_IDX + 2, hal, HAL_Timer_Get_Micro_Seconds, system_tick_t(void))
DYNALIB_FN(BASE_IDX + 3, hal, HAL_Timer_Get_Milli_Seconds, system_tick_t(void))

DYNALIB_FN(BASE_IDX + 4, hal, hal_rtc_init, void(void))

// These functions are deprecated
DYNALIB_FN(BASE_IDX + 5, hal, hal_rtc_get_unixtime_deprecated, time32_t(void))
DYNALIB_FN(BASE_IDX + 6, hal, hal_rtc_set_unixtime_deprecated, void(time32_t))

DYNALIB_FN(BASE_IDX + 7, hal, hal_rtc_set_alarm, int(const struct timeval*, uint32_t, hal_rtc_alarm_handler, void*, void*))

DYNALIB_FN(BASE_IDX + 8, hal, HAL_EEPROM_Init, void(void))
DYNALIB_FN(BASE_IDX + 9, hal, HAL_EEPROM_Read, uint8_t(uint32_t))
DYNALIB_FN(BASE_IDX + 10, hal, HAL_EEPROM_Write, void(uint32_t, uint8_t))
DYNALIB_FN(BASE_IDX + 11, hal, HAL_EEPROM_Length, size_t(void))

DYNALIB_FN(BASE_IDX + 12, hal, HAL_disable_irq, int(void))
DYNALIB_FN(BASE_IDX + 13, hal, HAL_enable_irq, void(int))
DYNALIB_FN(BASE_IDX + 14, hal, hal_rtc_cancel_alarm, void(void))

DYNALIB_FN(BASE_IDX + 15, hal, HAL_EEPROM_Get, void(uint32_t, void *, size_t))
DYNALIB_FN(BASE_IDX + 16, hal, HAL_EEPROM_Put, void(uint32_t, const void *, size_t))
DYNALIB_FN(BASE_IDX + 17, hal, HAL_EEPROM_Clear, void(void))
DYNALIB_FN(BASE_IDX + 18, hal, HAL_EEPROM_Has_Pending_Erase, bool(void))
DYNALIB_FN(BASE_IDX + 19, hal, HAL_EEPROM_Perform_Pending_Erase, void(void))
DYNALIB_FN(BASE_IDX + 20, hal, hal_rtc_time_is_valid, bool(void*))

DYNALIB_FN(BASE_IDX + 21, hal, hal_timer_millis, uint64_t(void*))
DYNALIB_FN(BASE_IDX + 22, hal, hal_timer_micros, uint64_t(void*))

DYNALIB_FN(BASE_IDX + 23, hal, hal_rtc_get_time, int(struct timeval*, void*))
DYNALIB_FN(BASE_IDX + 24, hal, hal_rtc_set_time, int(const struct timeval*, void*))

#if PLATFORM_ID == PLATFORM_TRACKER
DYNALIB_FN(BASE_IDX + 25, hal, hal_exrtc_enable_watchdog, int(system_tick_t, void*))
DYNALIB_FN(BASE_IDX + 26, hal, hal_exrtc_feed_watchdog, int(void*))
DYNALIB_FN(BASE_IDX + 27, hal, hal_exrtc_disable_watchdog, int(void*))
DYNALIB_FN(BASE_IDX + 28, hal, hal_exrtc_get_watchdog_limits, void(system_tick_t*, system_tick_t*, void*))
#define BASE_IDX2 (BASE_IDX + 29)
#else
#define BASE_IDX2 (BASE_IDX + 25)
#endif // PLATFORM_ID == PLATFORM_TRACKER

DYNALIB_END(hal)

#undef BASE_IDX
#undef BASE_IDX2
