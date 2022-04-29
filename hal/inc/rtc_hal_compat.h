/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#ifndef RTC_HAL_COMPAT_H
#define RTC_HAL_COMPAT_H

#include "time_compat.h"

// Deprecated *dynalib* APIs for backwards compatibility
inline void __attribute__((deprecated("Will be removed in 5.x! use hal_rtc_init() instead"), always_inline))
HAL_RTC_Configuration(void) {
    hal_rtc_init();
}

// There is no replacement
inline time32_t __attribute__((deprecated("Will be removed in 5.x!"), always_inline)) HAL_RTC_Get_UnixTime(void) {
    return hal_rtc_get_unixtime_deprecated();
}
// There is no replacement
inline void __attribute__((deprecated("Will be removed in 5.x!"), always_inline)) HAL_RTC_Set_UnixTime(time32_t value) {
    hal_rtc_set_unixtime_deprecated(value);
}

inline void __attribute__((deprecated("Will be removed in 5.x! use hal_rtc_cancel_alarm() instead"), always_inline))
HAL_RTC_Cancel_UnixAlarm(void) {
    hal_rtc_cancel_alarm();
}

inline uint8_t __attribute__((deprecated("Will be removed in 5.x! use hal_rtc_time_is_valid() instead"), always_inline))
HAL_RTC_Time_Is_Valid(void* reserved) {
    return (uint8_t)hal_rtc_time_is_valid(reserved);
}

#endif  /* RTC_HAL_COMPAT_H */
