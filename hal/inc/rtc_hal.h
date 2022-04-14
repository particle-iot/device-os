/*
 * Copyright (c) 2013 Particle Industries, Inc.  All rights reserved.
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

#ifndef RTC_HAL_H
#define RTC_HAL_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "time_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_rtc_alarm_handler)(void* context);

typedef enum hal_rtc_alarm_flags {
    HAL_RTC_ALARM_FLAG_IN = 0x01 // In provided amount of time, instead of an absolute timestamp
} hal_rtc_alarm_flags;

void hal_rtc_init(void);
int hal_rtc_get_time(struct timeval* tv, void* reserved);
int hal_rtc_set_time(const struct timeval* tv, void* reserved);
bool hal_rtc_time_is_valid(void* reserved);
// XXX: only one alarm and its handler can be registered at a time
int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved);
void hal_rtc_cancel_alarm(void);

void hal_rtc_internal_enter_sleep();
void hal_rtc_internal_exit_sleep();

// These functions are deprecated and are only used for backwards compatibility
// due to time_t size change
time32_t hal_rtc_get_unixtime_deprecated(void);
void hal_rtc_set_unixtime_deprecated(time32_t value);
//

#ifdef __cplusplus
}
#endif

#include "rtc_hal_compat.h"

#endif  /* RTC_HAL_H */
