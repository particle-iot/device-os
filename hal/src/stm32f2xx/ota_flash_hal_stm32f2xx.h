/**
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

#ifndef OTA_FLASH_HAL_STM32F2XX_H
#define	OTA_FLASH_HAL_STM32F2XX_H

extern const module_bounds_t* module_bounds[];
extern const unsigned module_bounds_length;
extern const module_bounds_t module_ota;
extern const module_bounds_t module_user;


void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved);

const uint8_t* fetch_server_public_key();
const uint8_t* fetch_device_private_key();
const uint8_t* fetch_device_public_key();

void set_key_value(key_value* kv, const char* key, const char* value);

#endif	/* OTA_FLASH_HAL_STM32F2XX_H */

