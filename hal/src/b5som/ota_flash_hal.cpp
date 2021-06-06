/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include "ota_flash_hal_impl.h"
#include "cellular_hal.h"
#include "platform_radio_stack.h"

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    const int additional = 3;
    int count = add_system_properties(info, create, additional);
    if (create) {
        info->key_value_count = count + additional;

        CellularDevice device = {};
        device.size = sizeof(device);
        cellular_device_info(&device, NULL);
        set_key_value(info->key_values+count, "imei", device.imei);
        set_key_value(info->key_values+count+1, "iccid", device.iccid);
        set_key_value(info->key_values+count+2, "cellfw", device.radiofw);
    }
}

