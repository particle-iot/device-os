/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "rtc_hal.h"
#include "exrtc_hal.h"
#include "sleep_hal.h"

#define HAL_EXRTC_MFG_MAGIC (0xC36AE15D)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Just mimicking rtc_hal to simplify rtc_hal <-> exrtc_hal coupling

int hal_exrtc_init(void);
int hal_exrtc_get_time_internal(struct timeval* tv);
int hal_exrtc_set_time_internal(const struct timeval* tv);
int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context);
int hal_exrtc_cancel_alarm(void);

bool hal_exrtc_is_default(void);

hal_exrtc_binding_t* hal_exrtc_default_binding();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
