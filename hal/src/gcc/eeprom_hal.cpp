/*
 *******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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
#include "eeprom_file.h"
#include "filesystem.h"
#include <string.h>
#include <string>

/*
 * Implements eeprom either as a transient storage,
 * or as a persisted storage, depending upon if the file exists.
 *
 */

static uint8_t eeprom[2048];

std::string eeprom_file;

/**
 * Write the eeprom state to the file.
 * This currently writes the entire file.
 */
void GCC_EEPROM_Flush()
{
	if (eeprom_file.length()) {
		GCC_EEPROM_Save(eeprom_file.c_str());
	}
}

/**
 * Initializes the eeprom. If the eeprom file has already been
 * loaded, this function does nothing. Otherwise, it erases the
 * eeprom storage to 0xFF to emulate NAND flash.
 */
void HAL_EEPROM_Init()
{
	if (!eeprom_file.length())
		HAL_EEPROM_Clear();
}

uint8_t HAL_EEPROM_Read(uint32_t index)
{
	uint8_t val;
	HAL_EEPROM_Get(index, &val, 1);
	return val;
}

void HAL_EEPROM_Write(uint32_t index, uint8_t data)
{
	HAL_EEPROM_Put(index, &data, 1);
}

void HAL_EEPROM_Get(uint32_t index, void *data, size_t length)
{
	memcpy(data, eeprom+index, length);
}

void HAL_EEPROM_Put(uint32_t index, const void *data, size_t length)
{
	memcpy(eeprom+index, data, length);
	GCC_EEPROM_Flush();
}

size_t HAL_EEPROM_Length()
{
	return sizeof(eeprom);
}

void HAL_EEPROM_Clear()
{
	memset(eeprom, 0xFF, sizeof(eeprom));
	GCC_EEPROM_Flush();
}

bool HAL_EEPROM_Has_Pending_Erase()
{
	return false;
}

void HAL_EEPROM_Perform_Pending_Erase()
{
}

void GCC_EEPROM_Load(const char* filename)
{
	read_file(filename, eeprom, sizeof(eeprom));
	eeprom_file = filename;
}

void GCC_EEPROM_Save(const char* filename)
{
	write_file(filename, eeprom, sizeof(eeprom));
}


