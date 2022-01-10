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

#include "ota_flash_hal.h"

#include "platforms.h"

extern const module_bounds_t* module_bounds[];
extern const unsigned module_bounds_length;

extern const module_bounds_t module_bootloader;

// Modular firmware
extern const module_bounds_t module_system_part1;
extern const module_bounds_t module_system_part2;
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
extern const module_bounds_t module_system_part3;
#endif
extern const module_bounds_t module_user;
extern const module_bounds_t module_ota;
extern const module_bounds_t module_factory;

// Monolithic firmware
extern const module_bounds_t module_user_mono;
extern const module_bounds_t module_factory_mono;

const uint8_t* fetch_server_public_key(uint8_t lock);
const uint8_t* fetch_device_private_key(uint8_t lock);
const uint8_t* fetch_device_public_key(uint8_t lock);

void set_key_value(key_value* kv, const char* key, const char* value);

/**
 * Fetches the key-value instances and the count of system properties for mesh.
 * @param storage	When not-null, points to storage with keyCount free entries.
 * 					When null, is used to retrieve the number of keys available.
 * @return The number of keys available, when storage is null, or the number of keys
 * copied to storage when not null.
 */
int fetch_system_properties(key_value* storage, int keyCount);

/**
 * Adds the system properties for mesh devices to the system info, allocating the storage required
 * plus an additional amount for platform-specific properties.
 */
int add_system_properties(hal_system_info_t* info, bool create, size_t additional);

#endif	/* OTA_FLASH_HAL_STM32F2XX_H */

