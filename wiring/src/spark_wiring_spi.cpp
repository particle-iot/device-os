/**
 ******************************************************************************
 * @file    spark_wiring_spi.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring SPI module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>

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

#include "spark_wiring_spi.h"

#ifndef SPARK_WIRING_NO_SPI
SPIClass SPI(HAL_SPI_INTERFACE1);

#if Wiring_SPI1
SPIClass SPI1(HAL_SPI_INTERFACE2);
#endif

#endif

SPIClass::SPIClass(HAL_SPI_Interface spi)
{
  _spi = spi;
  HAL_SPI_Init(_spi);
}

void SPIClass::begin()
{
  begin(SS);
}

void SPIClass::begin(uint16_t ss_pin)
{
  if (ss_pin >= TOTAL_PINS)
  {
    return;
  }

  HAL_SPI_Begin(_spi, ss_pin);
}

void SPIClass::end()
{
  HAL_SPI_End(_spi);
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
  HAL_SPI_Set_Bit_Order(_spi, bitOrder);
}

void SPIClass::setDataMode(uint8_t mode)
{
  HAL_SPI_Set_Data_Mode(_spi, mode);
}

void SPIClass::setClockDivider(uint8_t rate)
{
  HAL_SPI_Set_Clock_Divider(_spi, rate);
}

byte SPIClass::transfer(byte _data)
{
  return HAL_SPI_Send_Receive_Data(_spi, _data);
}

void SPIClass::transfer(void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback)
{
  HAL_SPI_DMA_Transfer(_spi, tx_buffer, rx_buffer, length, user_callback);
}

void SPIClass::attachInterrupt()
{
  //To Do
}

void SPIClass::detachInterrupt()
{
  //To Do
}

bool SPIClass::isEnabled()
{
  return HAL_SPI_Is_Enabled(_spi);
}
