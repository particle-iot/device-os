/**
 ******************************************************************************
 * @file    spark_wiring_spi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_spi.c module
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

#ifndef __SPARK_WIRING_SPI_H
#define __SPARK_WIRING_SPI_H

#include "spark_wiring.h"
#include "spark_wiring_platform.h"
#include "spi_hal.h"

typedef void (*wiring_spi_dma_transfercomplete_callback_t)(void);

class SPIClass {
private:
  HAL_SPI_Interface _spi;

public:
  SPIClass(HAL_SPI_Interface spi);
  virtual ~SPIClass() {};

  void begin();
  void begin(uint16_t);
  void end();

  void setBitOrder(uint8_t);
  void setDataMode(uint8_t);
  void setClockDivider(uint8_t);

  byte transfer(byte _data);
  void transfer(void* tx_buffer, void* rx_buffer, size_t length, wiring_spi_dma_transfercomplete_callback_t user_callback);

  void attachInterrupt();
  void detachInterrupt();

  bool isEnabled(void);
};

#ifndef SPARK_WIRING_NO_SPI

extern SPIClass SPI;

#if Wiring_SPI1
#ifdef SPI1
#undef SPI1
#endif  // SPI1

extern SPIClass SPI1;
#endif  // Wiring_SPI1
#endif  // SPARK_WIRING_NO_SPI

#endif
