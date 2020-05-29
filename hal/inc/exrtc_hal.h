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

#ifndef EXRTC_HAL_H
#define EXRTC_HAL_H

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_exrtc_alarm_handler_t)(void* context);

int hal_exrtc_set_unixtime(time_t unixtime, void* reserved);
time_t hal_exrtc_get_unixtime(void* reserved);
int hal_exrtc_set_unix_alarm(time_t unixtime, hal_exrtc_alarm_handler_t handler, void* context, void* reserved);
int hal_exrtc_cancel_unixalarm(void* reserved);
bool hal_exrtc_time_is_valid(void* reserved);

int hal_exrtc_enable_watchdog(time_t ms, void* reserved);
int hal_exrtc_disable_watchdog(void* reserved);
int hal_exrtc_feed_watchdog(void* reserved);

int hal_exrtc_sleep_timer(time_t ms, void* reserved);

int hal_exrtc_calibrate_xt(int adjValue, void* reserved);

#ifdef __cplusplus
}
#endif

#endif // HAL_PLATFORM_EXTERNAL_RTC

#endif // EXRTC_HAL_H
