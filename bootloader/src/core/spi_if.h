/**
  ******************************************************************************
  * @file    spi_if.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    20-May-2013
  * @brief   Header for spi_if.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_IF_MAL_H
#define __SPI_IF_MAL_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

uint16_t SPI_If_Init(void);
uint16_t SPI_If_Erase (uint32_t SectorAddress);
uint16_t SPI_If_Write (uint32_t SectorAddress, uint32_t DataLength);
uint8_t *SPI_If_Read (uint32_t SectorAddress, uint32_t DataLength);

#endif /* __SPI_IF_MAL_H */

