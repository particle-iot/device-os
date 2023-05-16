/******************************************************************************
 * @file    hal_dynalib_spi.h
 * @authors Matthew McGowan
 * @date    04 March 2015
 ******************************************************************************/
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

#ifndef HAL_DYNALIB_SPI_H
#define	HAL_DYNALIB_SPI_H

#include "dynalib.h"
#include "hal_platform.h"

#ifdef DYNALIB_EXPORT
#include "spi_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_spi)

DYNALIB_FN(0, hal_spi, hal_spi_begin, void(hal_spi_interface_t, uint16_t))
DYNALIB_FN(1, hal_spi, hal_spi_end, void(hal_spi_interface_t))
DYNALIB_FN(2, hal_spi, hal_spi_set_bit_order, void(hal_spi_interface_t, uint8_t))
DYNALIB_FN(3, hal_spi, hal_spi_set_data_mode, void(hal_spi_interface_t, uint8_t))
DYNALIB_FN(4, hal_spi, hal_spi_set_clock_divider, void(hal_spi_interface_t, uint8_t))
DYNALIB_FN(5, hal_spi, hal_spi_transfer, uint16_t(hal_spi_interface_t, uint16_t))
DYNALIB_FN(6, hal_spi, hal_spi_is_enabled_deprecated, bool(void))
DYNALIB_FN(7, hal_spi, hal_spi_init, void(hal_spi_interface_t))
DYNALIB_FN(8, hal_spi, hal_spi_is_enabled, bool(hal_spi_interface_t))
DYNALIB_FN(9, hal_spi, hal_spi_info, void(hal_spi_interface_t, hal_spi_info_t*, void*))
DYNALIB_FN(10, hal_spi, hal_spi_transfer_dma, void(hal_spi_interface_t, const void*, void*, uint32_t, hal_spi_dma_user_callback))
DYNALIB_FN(11, hal_spi, hal_spi_begin_ext, void(hal_spi_interface_t, hal_spi_mode_t, uint16_t, const hal_spi_config_t*))
DYNALIB_FN(12, hal_spi, hal_spi_set_callback_on_selected, void(hal_spi_interface_t, hal_spi_select_user_callback, void*))
DYNALIB_FN(13, hal_spi, hal_spi_transfer_dma_cancel, void(hal_spi_interface_t))
DYNALIB_FN(14, hal_spi, hal_spi_transfer_dma_status, int32_t(hal_spi_interface_t, hal_spi_transfer_status_t*))
DYNALIB_FN(15, hal_spi, hal_spi_set_settings, int32_t(hal_spi_interface_t, uint8_t, uint8_t, uint8_t, uint8_t, void*))
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
DYNALIB_FN(16, hal_spi, hal_spi_acquire, int32_t(hal_spi_interface_t, const hal_spi_acquire_config_t*))
DYNALIB_FN(17, hal_spi, hal_spi_release, int32_t(hal_spi_interface_t, void*))
#define BASE_IDX 18
#else
#define BASE_IDX 16
#endif
DYNALIB_FN(BASE_IDX + 0, hal_spi, hal_spi_sleep, int(hal_spi_interface_t, bool, void*))
DYNALIB_END(hal_spi)

#undef BASE_IDX

#endif	/* HAL_DYNALIB_SPI_H */

