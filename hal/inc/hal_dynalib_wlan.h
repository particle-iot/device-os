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

#pragma once

#include "dynalib.h"
#include "hal_platform.h"

#if HAL_PLATFORM_WIFI

#if HAL_PLATFORM_WIFI_COMPAT
#include "hal_dynalib_wlan_compat.h"
#else

#ifdef DYNALIB_EXPORT
#include "wlan_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_wlan)
DYNALIB_FN(0, hal_wlan, wlan_select_antenna, int(WLanSelectAntenna_TypeDef))
DYNALIB_FN(1, hal_wlan, wlan_scan, int(wlan_scan_result_t, void*))
DYNALIB_FN(2, hal_wlan, wlan_set_country_code, int(wlan_country_code_t, void*))
DYNALIB_FN(3, hal_wlan, wlan_get_country_code, int(void*))
DYNALIB_END(hal_wlan)

#endif // HAL_PLATFORM_WIFI_COMPAT

#endif // HAL_PLATFORM_WIFI