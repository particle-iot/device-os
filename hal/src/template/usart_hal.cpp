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

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud, uint8_t config, void*)
{
}

void HAL_USART_End(HAL_USART_Serial serial)
{
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
  return 0;
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    return 0;
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    return 0;
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    return 0;
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return false;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool Enable)
{
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial)
{
    return 0;
}
