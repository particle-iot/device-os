/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "system_tick_hal.h"
#include "am18x5.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int system_external_rtc_set_config(const particle::hal_am18x5_config_t* conf, void* reserved);
int system_external_rtc_get_config(particle::hal_am18x5_config_t* conf, void* reserved);
bool system_external_rtc_is_present(void* reserved);
int system_external_rtc_sleep(const particle::hal_am18x5_sleep_config_t* conf, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL