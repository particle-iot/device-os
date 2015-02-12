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
#include "eeprom_hal.h"

/* Arduino Compatibility Class -----------------------------------------------*/
class EEPROMClass
{
public:
    EEPROMClass();
    uint8_t read(int address) const;
    void write(int address, uint8_t value);
    size_t length() const;
};

extern EEPROMClass EEPROM;

#endif /* __SPARK_WIRING_EEPROM_H */
