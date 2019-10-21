/*
  ******************************************************************************
  * @file    dfu_mal.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   Header for dfu_mal.c file.
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
#ifndef __DFU_MAL_H
#define __DFU_MAL_H

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "usb_desc.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define MAL_OK   0
#define MAL_FAIL 1
#define MAX_USED_MEDIA 3
#define MAL_MASK 0xFC000000

#define INTERNAL_FLASH_BASE 0x08000000
#define SPI_FLASH_BASE      0x00000000

/* utils macro ---------------------------------------------------------------*/
#define _1st_BYTE(x)  (uint8_t)((x)&0xFF)             /* 1st addressing cycle */
#define _2nd_BYTE(x)  (uint8_t)(((x)&0xFF00)>>8)      /* 2nd addressing cycle */
#define _3rd_BYTE(x)  (uint8_t)(((x)&0xFF0000)>>16)   /* 3rd addressing cycle */
#define _4th_BYTE(x)  (uint8_t)(((x)&0xFF000000)>>24) /* 4th addressing cycle */
/* Exported macro ------------------------------------------------------------*/
#define SET_POLLING_TIMING(x)   buffer[1] = _1st_BYTE(x);\
                                buffer[2] = _2nd_BYTE(x);\
                                buffer[3] = _3rd_BYTE(x);

/* Exported functions ------------------------------------------------------- */

uint16_t MAL_Init (void);
uint16_t MAL_Erase (uint32_t SectorAddress);
uint16_t MAL_Write (uint32_t SectorAddress, uint32_t DataLength);
uint8_t  *MAL_Read (uint32_t SectorAddress, uint32_t DataLength);
uint16_t MAL_GetStatus(uint32_t SectorAddress ,uint8_t Cmd, uint8_t *buffer);

extern uint8_t  MAL_Buffer[wTransferSize]; /* RAM Buffer for Downloaded Data */
#endif /* __DFU_MAL_H */


