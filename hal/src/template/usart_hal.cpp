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

void hal_usart_init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
}

void hal_usart_begin(HAL_USART_Serial serial, uint32_t baud, uint8_t config, void*)
{
}

void hal_usart_end(HAL_USART_Serial serial)
{
}

uint32_t hal_usart_write(HAL_USART_Serial serial, uint8_t data)
{
  return 0;
}

int32_t hal_usart_available(HAL_USART_Serial serial)
{
    return 0;
}

int32_t hal_usart_read(HAL_USART_Serial serial)
{
    return 0;
}

int32_t hal_usart_peek(HAL_USART_Serial serial)
{
    return 0;
}

void hal_usart_flush(HAL_USART_Serial serial)
{
}

bool hal_usart_is_enabled(HAL_USART_Serial serial)
{
    return false;
}

void hal_usart_half_duplex(HAL_USART_Serial serial, bool Enable)
{
}

int32_t hal_usart_available_data_for_write(HAL_USART_Serial serial)
{
    return 0;
}
