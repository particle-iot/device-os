/**
 ******************************************************************************
 * @file    usart_hal.c
 * @author  Satish Nair, Brett Walach
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
#include "usart_hal.h"

void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t *rx_buffer, hal_usart_ring_buffer_t *tx_buffer)
{
}

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud, uint8_t config, void*)
{
}

void hal_usart_end(hal_usart_interface_t serial)
{
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data)
{
  return 0;
}

int32_t hal_usart_available(hal_usart_interface_t serial)
{
    return 0;
}

int32_t hal_usart_read(hal_usart_interface_t serial)
{
    return 0;
}

int32_t hal_usart_peek(hal_usart_interface_t serial)
{
    return 0;
}

void hal_usart_flush(hal_usart_interface_t serial)
{
}

bool hal_usart_is_enabled(hal_usart_interface_t serial)
{
    return false;
}

void hal_usart_half_duplex(hal_usart_interface_t serial, bool Enable)
{
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial)
{
    return 0;
}
