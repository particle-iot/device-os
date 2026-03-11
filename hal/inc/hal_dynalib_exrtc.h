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

#pragma once

#include "hal_platform.h"

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "exrtc_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

DYNALIB_BEGIN(hal_exrtc)

DYNALIB_FN(0, hal_exrtc, hal_exrtc_bind, int(hal_exrtc_instance_t, const hal_exrtc_binding_t*, void*))
DYNALIB_FN(1, hal_exrtc, hal_exrtc_get_device, int(hal_exrtc_instance_t, hal_exrtc_device_t*, void*))
DYNALIB_FN(2, hal_exrtc, hal_exrtc_unbind, int(hal_exrtc_instance_t, void*))
DYNALIB_FN(3, hal_exrtc, hal_exrtc_get_status, int(hal_exrtc_instance_t, hal_exrtc_status_t*, void*, void*))
DYNALIB_FN(4, hal_exrtc, hal_exrtc_set_config, int(hal_exrtc_instance_t, const hal_exrtc_config_t*, const hal_exrtc_vendor_config_t*, void*))
DYNALIB_FN(5, hal_exrtc, hal_exrtc_get_config, int(hal_exrtc_instance_t, hal_exrtc_config_t*, hal_exrtc_vendor_config_t*, void*))
DYNALIB_FN(6, hal_exrtc, hal_exrtc_event_handler_add, void*(hal_exrtc_instance_t, hal_exrtc_event_handler_t, void*, void*))
DYNALIB_FN(7, hal_exrtc, hal_exrtc_command, int(hal_exrtc_instance_t, hal_exrtc_command_t, void*, void*, void*))

DYNALIB_END(hal_exrtc)

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL