/**
 ******************************************************************************
 * @file    eeprom_hal.h
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
#ifndef __EEPROM_HAL_H
#define __EEPROM_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
/* Internal Flash Page size = 1KByte */
#define PAGE_SIZE               (uint16_t)0x400
/* EEPROM emulation start address in Flash (just after the write protected bootloader program space) */
#define EEPROM_START_ADDRESS    ((uint32_t)0x08004000)
/* EEPROM Emulation Size */
#define EEPROM_SIZE             ((uint8_t)0x64)       /* 100 bytes (Max 255/0xFF bytes) */

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_EEPROM_Init(void);
uint8_t HAL_EEPROM_Read(uint32_t address);
void HAL_EEPROM_Write(uint32_t address, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif  /* __EEPROM_HAL_H */
