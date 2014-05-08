/**
 ******************************************************************************
 * @file    spark_wiring_eeprom.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-April-2014
 * @brief   Functions/Class prototypes for the EEPROM emulation library.
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
#ifndef __SPARK_WIRING_EEPROM_H
#define __SPARK_WIRING_EEPROM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Internal Flash Page size = 1KByte */
#define PAGE_SIZE  (uint16_t)0x400

/* EEPROM emulation start address in Flash (just after the write protected bootloader program space) */
#define EEPROM_START_ADDRESS    ((uint32_t)0x08004000)

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + 0x000))
#define PAGE0_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))

#define PAGE1_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))

/* Used Flash pages for EEPROM emulation */
#define PAGE0                   ((uint16_t)0x0000)
#define PAGE1                   ((uint16_t)0x0001)

/* No valid page define */
#define NO_VALID_PAGE           ((uint16_t)0x00AB)

/* Page status definitions */
#define ERASED                  ((uint16_t)0xFFFF)     /* PAGE is empty */
#define RECEIVE_DATA            ((uint16_t)0xEEEE)     /* PAGE is marked to receive data */
#define VALID_PAGE              ((uint16_t)0x0000)     /* PAGE containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE    ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE     ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL               ((uint8_t)0x80)

/* EEPROM Emulation Size */
#define EEPROM_SIZE             ((uint8_t)0x64)       /* 100 bytes (Max 255/0xFF bytes) */

uint16_t EEPROM_Init(void);
uint16_t EEPROM_ReadVariable(uint16_t EepromAddress, uint16_t *EepromData);
uint16_t EEPROM_WriteVariable(uint16_t EepromAddress, uint16_t EepromData);

/* Arduino Compatibility Class -----------------------------------------------*/
class EEPROMClass
{
  public:
    EEPROMClass();
    uint8_t read(int);
    void write(int, uint8_t);
};

extern EEPROMClass EEPROM;

#endif /* __SPARK_WIRING_EEPROM_H */
