/**
 ******************************************************************************
 * @file    spi_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

void HAL_SPI_Begin(uint16_t pin)
{
}

void HAL_SPI_End(void)
{
}

void HAL_SPI_Set_Bit_Order(uint8_t order)
{
}

void HAL_SPI_Set_Data_Mode(uint8_t mode)
{
}

void HAL_SPI_Set_Clock_Divider(uint8_t rate)
{
}

uint16_t HAL_SPI_Send_Receive_Data(uint16_t data)
{
    return 0;
}

bool HAL_SPI_Is_Enabled(void)
{
    return false;
}
