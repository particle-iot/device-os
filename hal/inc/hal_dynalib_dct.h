/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
#include "dct_hal.h"
#include "device_code.h"
#endif

DYNALIB_BEGIN(hal_dct)

#if HAL_PLATFORM_DCT_NO_DEPRECATED
DYNALIB_FN(0, hal_dct, dct_read_app_data_copy, int(uint32_t, void*, size_t))
DYNALIB_FN(1, hal_dct, dct_write_app_data, int(const void*, uint32_t, uint32_t))
DYNALIB_FN(2, hal_dct, fetch_or_generate_setup_ssid, bool(device_code_t*))
#else
DYNALIB_FN(0, hal_dct, dct_read_app_data, const void*(uint32_t))
DYNALIB_FN(1, hal_dct, dct_read_app_data_copy, int(uint32_t, void*, size_t))
DYNALIB_FN(2, hal_dct, dct_read_app_data_lock, const void*(uint32_t))
DYNALIB_FN(3, hal_dct, dct_read_app_data_unlock, int(uint32_t))
DYNALIB_FN(4, hal_dct, dct_write_app_data, int(const void*, uint32_t, uint32_t))
DYNALIB_FN(5, hal_dct, fetch_or_generate_setup_ssid, bool(device_code_t*))
#endif // HAL_PLATFORM_DCT_NO_DEPRECATED
DYNALIB_END(hal_dct)
