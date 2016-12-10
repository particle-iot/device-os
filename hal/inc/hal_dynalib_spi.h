/**
 ******************************************************************************
 * @file    hal_dynalib_spi.h
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

#ifndef HAL_DYNALIB_SPI_H
#define	HAL_DYNALIB_SPI_H

#include "dynalib.h"

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

DYNALIB_FN(0, hal_spi, HAL_SPI_Begin, void(HAL_SPI_Interface, uint16_t))
DYNALIB_FN(1, hal_spi, HAL_SPI_End, void(HAL_SPI_Interface))
DYNALIB_FN(2, hal_spi, HAL_SPI_Set_Bit_Order, void(HAL_SPI_Interface, uint8_t))
DYNALIB_FN(3, hal_spi, HAL_SPI_Set_Data_Mode, void(HAL_SPI_Interface, uint8_t))
DYNALIB_FN(4, hal_spi, HAL_SPI_Set_Clock_Divider, void(HAL_SPI_Interface, uint8_t))
DYNALIB_FN(5, hal_spi, HAL_SPI_Send_Receive_Data, uint16_t(HAL_SPI_Interface, uint16_t))
DYNALIB_FN(6, hal_spi, HAL_SPI_Is_Enabled_Old, bool(void))
DYNALIB_FN(7, hal_spi, HAL_SPI_Init, void(HAL_SPI_Interface))
DYNALIB_FN(8, hal_spi, HAL_SPI_Is_Enabled, bool(HAL_SPI_Interface))
DYNALIB_FN(9, hal_spi, HAL_SPI_Info, void(HAL_SPI_Interface, hal_spi_info_t*, void*))
DYNALIB_FN(10, hal_spi, HAL_SPI_DMA_Transfer, void(HAL_SPI_Interface, void*, void*, uint32_t, HAL_SPI_DMA_UserCallback))
DYNALIB_FN(11, hal_spi, HAL_SPI_Begin_Ext, void(HAL_SPI_Interface, SPI_Mode, uint16_t, void*))
DYNALIB_FN(12, hal_spi, HAL_SPI_Set_Callback_On_Select, void(HAL_SPI_Interface, HAL_SPI_Select_UserCallback, void*))
DYNALIB_FN(13, hal_spi, HAL_SPI_DMA_Transfer_Cancel, void(HAL_SPI_Interface))
DYNALIB_FN(14, hal_spi, HAL_SPI_DMA_Transfer_Status, int32_t(HAL_SPI_Interface, HAL_SPI_TransferStatus*))
DYNALIB_FN(15, hal_spi, HAL_SPI_Set_Settings, int32_t(HAL_SPI_Interface, uint8_t, uint8_t, uint8_t, uint8_t, void*))

DYNALIB_END(hal_spi)


#endif	/* HAL_DYNALIB_SPI_H */

