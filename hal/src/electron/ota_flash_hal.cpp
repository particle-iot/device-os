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
