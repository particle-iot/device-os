/**
 ******************************************************************************
 * @file    eeprom_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    18-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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

#include "eeprom_hal.h"
#include "eeprom_emulation_impl.h"

FlashEEPROM flashEEPROM;

void HAL_EEPROM_Init(void)
{
  flashEEPROM.init();
}

uint8_t HAL_EEPROM_Read(uint32_t index)
{
  uint8_t value = 0xFF;
  flashEEPROM.get(index, value);
  return value;
}

void HAL_EEPROM_Write(uint32_t index, uint8_t data)
{
  flashEEPROM.put(index, data);
}

size_t HAL_EEPROM_Length()
{
  return flashEEPROM.capacity();
}

void HAL_EEPROM_Get(uint32_t index, void *data, size_t length)
{
    flashEEPROM.get(index, data, length);
}

void HAL_EEPROM_Put(uint32_t index, const void *data, size_t length)
{
    flashEEPROM.put(index, data, length);
}

void HAL_EEPROM_Clear()
{
    flashEEPROM.clear();
}

bool HAL_EEPROM_Has_Pending_Erase()
{
    return flashEEPROM.hasPendingErase();
}

void HAL_EEPROM_Perform_Pending_Erase()
{
    flashEEPROM.performPendingErase();
}
