/**
 ******************************************************************************
 * @file    hal_dynalib_i2c.h
 * @authors Matthew McGowan
 * @date    04 March 2015
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

#ifndef HAL_DYNALIB_I2C_H
#define	HAL_DYNALIB_I2C_H

#include "dynalib.h"

#include "platforms.h"

#ifdef DYNALIB_EXPORT
#include "i2c_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_i2c)

// first edition of these functions that were released on the Photon/P1
// They are not needed on other platforms.
#if PLATFORM_ID==PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID==PLATFORM_P1
DYNALIB_FN(0, hal_i2c, hal_i2c_set_speed_deprecated, void(uint32_t))
DYNALIB_FN(1, hal_i2c, hal_i2c_enable_dma_mode_deprecated, void(bool))
DYNALIB_FN(2, hal_i2c, hal_i2c_stretch_clock_deprecated, void(bool))
DYNALIB_FN(3, hal_i2c, hal_i2c_begin_deprecated, void(hal_i2c_mode_t, uint8_t))
DYNALIB_FN(4, hal_i2c, hal_i2c_end_deprecated, void(void))
DYNALIB_FN(5, hal_i2c, hal_i2c_request_deprecated, uint32_t(uint8_t, uint8_t, uint8_t))
DYNALIB_FN(6, hal_i2c, hal_i2c_begin_transmission_deprecated, void(uint8_t))
DYNALIB_FN(7, hal_i2c, hal_i2c_end_transmission_deprecated, uint8_t(uint8_t))
DYNALIB_FN(8, hal_i2c, hal_i2c_write_deprecated, uint32_t(uint8_t))
DYNALIB_FN(9, hal_i2c, hal_i2c_available_deprecated, int32_t(void))
DYNALIB_FN(10, hal_i2c, hal_i2c_read_deprecated, int32_t(void))
DYNALIB_FN(11, hal_i2c, hal_i2c_peek_deprecated, int32_t(void))
DYNALIB_FN(12, hal_i2c, hal_i2c_flush_deprecated, void(void))
DYNALIB_FN(13, hal_i2c, hal_i2c_is_enabled_deprecated, bool(void))
DYNALIB_FN(14, hal_i2c, hal_i2c_set_callback_on_received_deprecated, void(void(*)(int)))
DYNALIB_FN(15, hal_i2c, hal_i2c_set_callback_on_requested_deprecated, void(void(*)(void)))
#define BASE_IDX 16 // Base index for all subsequent functions
#else
#define BASE_IDX 0
#endif

DYNALIB_FN(BASE_IDX + 0, hal_i2c, hal_i2c_set_speed, void(hal_i2c_interface_t, uint32_t, void*))
DYNALIB_FN(BASE_IDX + 1, hal_i2c, hal_i2c_enable_dma_mode, void(hal_i2c_interface_t, bool, void*))
DYNALIB_FN(BASE_IDX + 2, hal_i2c, hal_i2c_stretch_clock, void(hal_i2c_interface_t, bool, void*))
DYNALIB_FN(BASE_IDX + 3, hal_i2c, hal_i2c_begin, void(hal_i2c_interface_t, hal_i2c_mode_t, uint8_t, void*))
DYNALIB_FN(BASE_IDX + 4, hal_i2c, hal_i2c_end, void(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 5, hal_i2c, hal_i2c_request, uint32_t(hal_i2c_interface_t, uint8_t, uint8_t, uint8_t, void*))
DYNALIB_FN(BASE_IDX + 6, hal_i2c, hal_i2c_begin_transmission, void(hal_i2c_interface_t, uint8_t, const hal_i2c_transmission_config_t*))
DYNALIB_FN(BASE_IDX + 7, hal_i2c, hal_i2c_end_transmission, uint8_t(hal_i2c_interface_t, uint8_t, void*))
DYNALIB_FN(BASE_IDX + 8, hal_i2c, hal_i2c_write, uint32_t(hal_i2c_interface_t, uint8_t, void*))
DYNALIB_FN(BASE_IDX + 9, hal_i2c, hal_i2c_available, int32_t(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 10, hal_i2c, hal_i2c_read, int32_t(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 11, hal_i2c, hal_i2c_peek, int32_t(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 12, hal_i2c, hal_i2c_flush, void(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 13, hal_i2c, hal_i2c_is_enabled, bool(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 14, hal_i2c, hal_i2c_set_callback_on_received, void(hal_i2c_interface_t, void(*)(int), void*))
DYNALIB_FN(BASE_IDX + 15, hal_i2c, hal_i2c_set_callback_on_requested, void(hal_i2c_interface_t, void(*)(void), void*))
DYNALIB_FN(BASE_IDX + 16, hal_i2c, hal_i2c_init, int(hal_i2c_interface_t, const hal_i2c_config_t*))
DYNALIB_FN(BASE_IDX + 17, hal_i2c, hal_i2c_reset, uint8_t(hal_i2c_interface_t, uint32_t, void*))
DYNALIB_FN(BASE_IDX + 18, hal_i2c, hal_i2c_lock, int32_t(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 19, hal_i2c, hal_i2c_unlock, int32_t(hal_i2c_interface_t, void*))
DYNALIB_FN(BASE_IDX + 20, hal_i2c, hal_i2c_request_ex, int32_t(hal_i2c_interface_t, const hal_i2c_transmission_config_t*, void*))
DYNALIB_FN(BASE_IDX + 21, hal_i2c, hal_i2c_sleep, int(hal_i2c_interface_t i2c, bool sleep, void* reserved))

DYNALIB_END(hal_i2c)

#undef BASE_IDX

#endif	/* HAL_DYNALIB_I2C_H */

