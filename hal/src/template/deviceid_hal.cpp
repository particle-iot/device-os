/**
 ******************************************************************************
 * @file    deviceid_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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


#include "deviceid_hal.h"
#include <stddef.h>
#include "system_error.h"

unsigned hal_get_device_id(uint8_t* dest, unsigned destLen)
{
    if (dest!=NULL && destLen>0)
        *dest = 0;
    return 0;
}

unsigned hal_get_platform_id()
{
    return PLATFORM_ID;
}

void hal_save_device_id(uint32_t offset) {
}

int hal_get_device_serial_number(char* str, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_device_secret(char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_set_device_secret(char* data, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_device_hw_version(uint32_t* revision, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_device_hw_model(uint32_t* model, uint32_t* variant, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_device_hw_info(hal_device_hw_info* info, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
