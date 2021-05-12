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
#include "ota_flash_hal_impl.h"
#include "platform_ncp.h"
#include "platform_radio_stack.h"

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    add_system_properties(info, create, 0);
    for (int i = 0; i < info->module_count; i++) {
        hal_module_t* module = info->modules + i;
        if (!memcmp(&module->bounds, &module_ncp_mono, sizeof(module_ncp_mono))) {
            platform_ncp_fetch_module_info(module);
        } else if (!memcmp(&module->bounds, &module_radio_stack, sizeof(module_radio_stack))) {
            platform_radio_stack_fetch_module_info(module);
        }
    }
}
