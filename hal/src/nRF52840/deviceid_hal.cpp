/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "deviceid_hal.h"
#include "exflash_hal.h"
#include "str_util.h"
#include "system_error.h"
#ifndef HAL_DEVICE_ID_NO_DCT
#include "dct.h"
#endif /* HAL_DEVICE_ID_NO_DCT */
#include "check.h"

#include "nrf52840.h"

#include <algorithm>

#include "platform_ncp.h"

namespace {

using namespace particle;

const uint32_t DEVICE_ID_PREFIX = 0x68ce0fe0;

const uintptr_t SERIAL_NUMBER_OTP_ADDRESS = 0x00000000;
const uintptr_t DEVICE_SECRET_OTP_ADDRESS = 0x00000010;
const uintptr_t HW_DATA_OTP_ADDRESS = 0x00000020;
const uintptr_t HW_MODEL_OTP_ADDRESS = 0x00000024;

} // namespace

unsigned hal_get_device_id(uint8_t* dest, unsigned destLen)
{
    const uint32_t id[3] = { DEVICE_ID_PREFIX, NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1] };
    static_assert(sizeof(id) == HAL_DEVICE_ID_SIZE, "");
    if (dest && destLen > 0) {
        memcpy(dest, id, std::min(destLen, sizeof(id)));
    }
    return HAL_DEVICE_ID_SIZE;
}

unsigned hal_get_platform_id()
{
    return PLATFORM_ID;
}

int HAL_Get_Device_Identifier(const char** name, char* buf, size_t buflen, unsigned index, void* reserved)
{
    return -1;
}

int hal_get_device_serial_number(char* str, size_t size, void* reserved)
{
    char serial[HAL_DEVICE_SERIAL_NUMBER_SIZE] = {};

    int r = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, SERIAL_NUMBER_OTP_ADDRESS,
                                     (uint8_t*)serial, HAL_DEVICE_SERIAL_NUMBER_SIZE);

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

int hal_get_device_hw_version(uint32_t* revision, void* reserved)
{
    // HW Data format: | NCP_ID | HW_VERSION | HW Feature Flags |
    //                 | byte 0 |   byte 1   |    byte 2/3      |
    uint8_t hw_data[4] = {};
    int r = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, HW_DATA_OTP_ADDRESS, hw_data, 4);
    if (r) {
        return SYSTEM_ERROR_INTERNAL;
    }
    if (hw_data[1] == 0xFF) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    *revision = hw_data[1];
    return SYSTEM_ERROR_NONE;
}

int hal_get_device_hw_model(uint32_t* model, uint32_t* variant, void* reserved)
{
    // HW Model format: | Model Number LSB | Model Number MSB | Model Variant LSB | Model Variant MSB |
    //                  |      byte 0      |      byte 1      |      byte 2       |      byte 3       |
    uint8_t hw_model[4] = {};
    int r = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, HW_MODEL_OTP_ADDRESS, hw_model, 4);
    if (r) {
        return SYSTEM_ERROR_INTERNAL;
    }
    // Model and variant values of 0xFFFF are acceptable
    *model = ((uint32_t)hw_model[1] << 8) | (uint32_t)hw_model[0];
    *variant = ((uint32_t)hw_model[3] << 8) | (uint32_t)hw_model[2];
    return SYSTEM_ERROR_NONE;
}

int hal_get_device_hw_info(hal_device_hw_info* info, void* reserved) {
    CHECK_TRUE(info, SYSTEM_ERROR_INVALID_ARGUMENT);
    // HW Data format: | NCP_ID | HW_VERSION | HW Feature Flags |
    //                 | byte 0 |   byte 1   |    byte 2/3      |
    uint8_t hw_data[4] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, HW_DATA_OTP_ADDRESS, hw_data, sizeof(hw_data)));
    CHECK(hal_get_device_hw_model(&info->model, &info->variant, nullptr));
    info->revision = hw_data[1];
    if (info->revision == 0xff) {
        info->revision = 0xffffffff;
    }
    info->features = ((uint32_t)hw_data[3] << 8) | (uint32_t)hw_data[2];
    memset(info->ncp, 0xff, sizeof(info->ncp));
    for (int i = 0; i < HAL_PLATFORM_NCP_COUNT; i++) {
        PlatformNCPInfo ncpInfo = {};
        if (!platform_ncp_get_info(i, &ncpInfo)) {
            info->ncp[i] = (uint16_t)ncpInfo.identifier;
        }
    }
    return 0;
}

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
        ret = hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, DEVICE_SECRET_OTP_ADDRESS, (uint8_t*)secret, sizeof(secret));
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

int hal_set_device_secret(char* data, size_t size, void* reserved) {
    uint8_t blankSecret[DCT_DEVICE_SECRET_SIZE] = {0};
    if ((data == nullptr) || (size == 0)) {
        dct_write_app_data(blankSecret, DCT_DEVICE_SECRET_OFFSET, sizeof(blankSecret));
        return SYSTEM_ERROR_NONE;
    }

    if (size != HAL_DEVICE_SECRET_SIZE || !isPrintable(data, HAL_DEVICE_SECRET_SIZE)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    int ret = dct_write_app_data(data, DCT_DEVICE_SECRET_OFFSET, size);
    if (ret < 0) {
        dct_write_app_data(blankSecret, DCT_DEVICE_SECRET_OFFSET, sizeof(blankSecret));
    }
    return ret;
}

#endif /* HAL_DEVICE_ID_NO_DCT */

