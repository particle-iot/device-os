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

#include "str_util.h"

#include <algorithm>

#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

namespace {

using namespace particle;

const uintptr_t SERIAL_NUMBER_OTP_ADDRESS = 0x00000000;
const uintptr_t DEVICE_SECRET_OTP_ADDRESS = 0x00000010;

const unsigned device_id_len = 12;

} // namespace

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

#ifndef HAL_DEVICE_ID_NO_DCT
int hal_get_device_serial_number(char* str, size_t size, void* reserved)
{
    char serial[HAL_DEVICE_SERIAL_NUMBER_SIZE] = {};

    int r = FLASH_ReadOTP(SERIAL_NUMBER_OTP_ADDRESS, (uint8_t*)serial, HAL_DEVICE_SERIAL_NUMBER_SIZE);

    if (r != 0 || !isPrintable(serial, sizeof(serial))) {
        return -1;
    }
    if (str) {
        memcpy(str, serial, std::min(size, sizeof(serial)));
        // Ensure the output is null-terminated
        if (sizeof(serial) < size) {
            str[sizeof(serial)] = '\0';
        }
    }
    return HAL_DEVICE_SERIAL_NUMBER_SIZE;
}
#endif // HAL_DEVICE_ID_NO_DCT

#ifndef HAL_DEVICE_ID_NO_DCT
int hal_get_device_secret(char* data, size_t size, void* reserved)
{
    // Check if the device secret data is initialized in the DCT
    char secret[HAL_DEVICE_SECRET_SIZE] = {};
    static_assert(sizeof(secret) == DCT_DEVICE_SECRET_SIZE, "");
    int ret = dct_read_app_data_copy(DCT_DEVICE_SECRET_OFFSET, secret, sizeof(secret));
    if (ret < 0) {
        return ret;
    }
    if (!isPrintable(secret, sizeof(secret))) {
        // Check the OTP memory
        ret = FLASH_ReadOTP(DEVICE_SECRET_OTP_ADDRESS, (uint8_t*)secret, sizeof(secret));
        if (ret < 0) {
            return ret;
        }
        if (!isPrintable(secret, sizeof(secret))) {
            return SYSTEM_ERROR_NOT_FOUND;
        };
    }
    memcpy(data, secret, std::min(size, sizeof(secret)));
    if (size>HAL_DEVICE_SECRET_SIZE) {
    	data[HAL_DEVICE_SECRET_SIZE] = 0;
    }
    return HAL_DEVICE_SECRET_SIZE;
}
#endif /* HAL_DEVICE_ID_NO_DCT */

int hal_get_device_hw_version(uint32_t* revision, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_get_device_hw_model(uint32_t* model, uint32_t* variant, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
