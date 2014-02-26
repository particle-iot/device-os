/**
 ******************************************************************************
 * @file    spark_wiring_spi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_spi.c module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
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

#define SPI_MODE0			0x00
#define SPI_MODE1			0x01
#define SPI_MODE2			0x02
#define SPI_MODE3			0x03

#define SPI_CLOCK_DIV2		SPI_BaudRatePrescaler_2
#define SPI_CLOCK_DIV4		SPI_BaudRatePrescaler_4
#define SPI_CLOCK_DIV8		SPI_BaudRatePrescaler_8
#define SPI_CLOCK_DIV16		SPI_BaudRatePrescaler_16
#define SPI_CLOCK_DIV32		SPI_BaudRatePrescaler_32
#define SPI_CLOCK_DIV64		SPI_BaudRatePrescaler_64
#define SPI_CLOCK_DIV128	SPI_BaudRatePrescaler_128
#define SPI_CLOCK_DIV256	SPI_BaudRatePrescaler_256

class SPIClass {
private:
	static SPI_InitTypeDef SPI_InitStructure;

	static bool SPI_Bit_Order_Set;
	static bool SPI_Data_Mode_Set;
	static bool SPI_Clock_Divider_Set;
	static bool SPI_Enabled;

public:
	static void begin();
	static void begin(uint16_t);
	static void end();

	static void setBitOrder(uint8_t);
	static void setDataMode(uint8_t);
	static void setClockDivider(uint8_t);

	static byte transfer(byte _data);

	static void attachInterrupt();
	static void detachInterrupt();

	static bool isEnabled(void);
};

extern SPIClass SPI;

#endif
