/**
 ******************************************************************************
 * @file    ota_flash_hal.cpp
 * @author  Matthew McGowan, Satish Nair
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
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

#include <cstring>
#include "ota_module.h"
#include "spark_macros.h"
#include "ota_flash_hal_stm32f2xx.h"
#include "cellular_hal.h"
#include "core_hal.h"

// Electron!

#if MODULAR_FIRMWARE
const module_bounds_t module_bootloader = { 0x4000, 0x8000000, 0x8004000, MODULE_FUNCTION_BOOTLOADER, 0, MODULE_STORE_MAIN };
const module_bounds_t module_system_part1 = { 0x20000, 0x8020000, 0x8040000, MODULE_FUNCTION_SYSTEM_PART, 1, MODULE_STORE_MAIN };
const module_bounds_t module_system_part2 = { 0x20000, 0x8040000, 0x8060000, MODULE_FUNCTION_SYSTEM_PART, 2, MODULE_STORE_MAIN };
const module_bounds_t module_user = { 0x20000, 0x8080000, 0x80A0000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_MAIN};
const module_bounds_t module_factory = { 0x20000, 0x80A0000, 0x80C0000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_FACTORY};
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_system_part1, &module_system_part2, &module_user, &module_factory };

const module_bounds_t module_ota = { 0x20000, 0x80C0000, 0x80E0000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD};
#else
const module_bounds_t module_bootloader = { 0x4000, 0x8000000, 0x8004000, MODULE_FUNCTION_BOOTLOADER, 0, MODULE_STORE_MAIN};
const module_bounds_t module_user = { 0x60000, 0x8020000, 0x8080000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_MAIN};
const module_bounds_t module_factory = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_FACTORY};
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_user, &module_factory };

const module_bounds_t module_ota = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD};
#endif

const unsigned module_bounds_length = arraySize(module_bounds);


void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    if (create) {
        info->key_value_count = 2;
        info->key_values = new key_value[info->key_value_count];

        CellularDevice device;
        memset(&device, 0, sizeof(device));
        device.size = sizeof(device);
        cellular_device_info(&device, NULL);

        set_key_value(info->key_values, "imei", device.imei);
        set_key_value(info->key_values+1, "iccid", device.iccid);
    }
    else
    {
        delete info->key_values;
        info->key_values = NULL;
    }
}
