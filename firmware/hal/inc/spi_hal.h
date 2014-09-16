/**
 ******************************************************************************
 * @file    spi_hal.h
 * @author  Satish Nair, Brett Walach
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_HAL_H
#define __SPI_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define SPI_MODE0               0x00
#define SPI_MODE1               0x01
#define SPI_MODE2               0x02
#define SPI_MODE3               0x03

#define SPI_CLOCK_DIV2          SPI_BaudRatePrescaler_2
#define SPI_CLOCK_DIV4          SPI_BaudRatePrescaler_4
#define SPI_CLOCK_DIV8          SPI_BaudRatePrescaler_8
#define SPI_CLOCK_DIV16         SPI_BaudRatePrescaler_16
#define SPI_CLOCK_DIV32         SPI_BaudRatePrescaler_32
#define SPI_CLOCK_DIV64         SPI_BaudRatePrescaler_64
#define SPI_CLOCK_DIV128        SPI_BaudRatePrescaler_128
#define SPI_CLOCK_DIV256        SPI_BaudRatePrescaler_256

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_SPI_Begin(uint16_t pin);
void HAL_SPI_End(void);
void HAL_SPI_Set_Bit_Order(uint8_t order);
void HAL_SPI_Set_Data_Mode(uint8_t mode);
void HAL_SPI_Set_Clock_Divider(uint8_t rate);
uint16_t HAL_SPI_Send_Receive_Data(uint16_t data);
bool HAL_SPI_Is_Enabled(void);

#ifdef __cplusplus
}
#endif

#endif  /* __SPI_HAL_H */
