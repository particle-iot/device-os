/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "spi_hal.h"

void hal_spi_init(hal_spi_interface_t spi)
{
}

void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin)
{
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, hal_spi_config_t* spi_config)
{
}

void hal_spi_end(hal_spi_interface_t spi)
{
}

void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order)
{
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode)
{
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate)
{
}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data)
{
    return 0;
}

bool hal_spi_is_enabled(hal_spi_interface_t spi)
{
    return false;
}

void hal_spi_transfer_dma(hal_spi_interface_t spi, void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback)
{
}

void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved)
{
}

void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved)
{
}

void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi)
{
}

int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st)
{
    return 0;
}

int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved)
{
  return 0;
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    return 0;
}
