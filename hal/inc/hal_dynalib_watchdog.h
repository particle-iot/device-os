/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#ifndef HAL_DYNALIB_WATCHDOG_H
#define	HAL_DYNALIB_WATCHDOG_H

#include "dynalib.h"
#include "hal_platform.h"

#ifdef DYNALIB_EXPORT
#include "watchdog_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

#if HAL_PLATFORM_HW_WATCHDOG

DYNALIB_BEGIN(hal_watchdog)

DYNALIB_FN(0, hal_watchdog, hal_watchdog_set_config, int(hal_watchdog_instance_t, const hal_watchdog_config_t*, void*))
DYNALIB_FN(1, hal_watchdog, hal_watchdog_on_expired_callback, int(hal_watchdog_instance_t, hal_watchdog_on_expired_callback_t, void*, void*))
DYNALIB_FN(2, hal_watchdog, hal_watchdog_start, int(hal_watchdog_instance_t, void*))
DYNALIB_FN(3, hal_watchdog, hal_watchdog_stop, int(hal_watchdog_instance_t, void*))
DYNALIB_FN(4, hal_watchdog, hal_watchdog_refresh, int(hal_watchdog_instance_t, void*))
DYNALIB_FN(5, hal_watchdog, hal_watchdog_get_info, int(hal_watchdog_instance_t, hal_watchdog_info_t*, void*))

DYNALIB_END(hal_watchdog)

#endif // HAL_PLATFORM_HW_WATCHDOG


#endif	/* HAL_DYNALIB_WATCHDOG_H */

