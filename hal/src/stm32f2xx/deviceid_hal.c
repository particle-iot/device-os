/**
 ******************************************************************************
 * @file    deviceid_hal.c
 * @author  Matthew McGowan
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


#include "deviceid_hal.h"
#include "system_error.h"
#include "platform_config.h"
#ifndef HAL_DEVICE_ID_NO_DCT
#include "dct_hal.h"
#endif // HAL_DEVICE_ID_NO_DCT
#include <string.h>

#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

const unsigned device_id_len = 12;

unsigned HAL_device_ID(uint8_t* dest, unsigned destLen)
{
    if (dest!=NULL && destLen!=0)
        memcpy(dest, (char*)ID1, MIN(destLen, device_id_len));
    return device_id_len;
}

unsigned HAL_Platform_ID()
{
    return PLATFORM_ID;
}

#ifndef HAL_DEVICE_ID_NO_DCT
void HAL_save_device_id(uint32_t dct_offset)
{
    char saved_device_id = 0;
    dct_read_app_data_copy(dct_offset, &saved_device_id, sizeof(saved_device_id));
    if (saved_device_id == 0xFF)
    {
        uint8_t device_id[device_id_len];
        HAL_device_ID(device_id, sizeof(device_id));
        dct_write_app_data(device_id, dct_offset, device_id_len);
    }
}
#endif // HAL_DEVICE_ID_NO_DCT

int hal_get_device_serial_number(char* str, size_t size, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
