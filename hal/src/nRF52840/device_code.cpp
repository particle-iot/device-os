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

#include "device_code.h"
#include "hal_platform.h"
#include "deviceid_hal.h"

#include "str_util.h"
#include "preprocessor.h"
#include "debug.h"
#include "system_error.h"

#include "dct.h"

#include <algorithm>
#include <cstring>

namespace {

using namespace particle;

// TODO: Rename ssid_prefix to device_name in DCT?
const auto DEVICE_NAME_DCT_SIZE = DCT_SSID_PREFIX_SIZE;
const auto DEVICE_NAME_DCT_OFFSET = DCT_SSID_PREFIX_OFFSET;

const auto DEVICE_NAME_MAX_SIZE = DEVICE_NAME_DCT_SIZE - 1;

const auto SETUP_CODE_DCT_OFFSET = DCT_DEVICE_CODE_OFFSET;

} // ::

int get_device_setup_code(char* code, size_t size) {
    // Check if the device setup code is initialized in the DCT
    char setupCode[HAL_SETUP_CODE_SIZE] = {};
    size_t codeSize = 0;
    int ret = dct_read_app_data_copy(SETUP_CODE_DCT_OFFSET, setupCode, sizeof(setupCode));
    if (ret < 0 || !particle::isPrintable(setupCode, sizeof(setupCode))) {
        // Check the OTP memory
        char serial[HAL_DEVICE_SERIAL_NUMBER_SIZE] = {};
        ret = hal_get_device_serial_number(serial, sizeof(serial), nullptr);
        if ((ret > 0) && ((size_t)ret >= sizeof(setupCode)) && ((size_t)ret <= sizeof(serial))) {
            // Use last characters of the serial number as the setup code
            codeSize = std::min(size, (size_t)HAL_SETUP_CODE_SIZE);
            memcpy(code, &serial[ret - sizeof(setupCode)], codeSize);
        } else {
            // Return a dummy setup code
            codeSize = std::min(sizeof(setupCode), size);
            memset(code, 'X', codeSize);
            // Overwrite return error as this is a valid case
            ret = SYSTEM_ERROR_NONE;
        }
    } else {
        codeSize = std::min(sizeof(setupCode), size);
        memcpy(code, setupCode, codeSize);
    }
    // Null terminate if passed buffer is greater than actual size
    if (codeSize < size) {
        code[codeSize] = '\0';
    }
    return (ret < 0) ? ret : codeSize;
}

bool fetch_or_generate_setup_ssid(device_code_t* code) {
    int ret = get_device_name((char*)code->value, sizeof(device_code_t::value));
    if (ret < 0) {
        return false;
    }
    if ((size_t)ret < sizeof(device_code_t::value)) {
        code->length = ret;
    } else {
        code->length = sizeof(device_code_t::value) - 1; // Exclude term. null
    }
    return true;
}

int get_device_name(char* buf, size_t size) {
    char dctName[DEVICE_NAME_DCT_SIZE] = {};
    int ret = dct_read_app_data_copy(DEVICE_NAME_DCT_OFFSET, dctName, DEVICE_NAME_DCT_SIZE);
    if (ret < 0) {
        return ret;
    }
    size_t nameSize = (unsigned char)dctName[0]; // First byte is the length
    char* const name = &dctName[1];
    if (nameSize == 0 || nameSize > DEVICE_NAME_MAX_SIZE || !isPrintable(name, nameSize)) {
        // Get device setup code
        char code[HAL_SETUP_CODE_SIZE] = {};
        int codeSize = get_device_setup_code(code, sizeof(code));
        if (codeSize < 0) {
            return codeSize;
        }
        // Get platform name
        const char* const platform = PRODUCT_SERIES;
        nameSize = sizeof(PRODUCT_SERIES) - 1; // Exclude term. null
        if (nameSize + HAL_SETUP_CODE_SIZE + 1 > DEVICE_NAME_MAX_SIZE) { // Reserve 1 character for '-'
            nameSize = DEVICE_NAME_MAX_SIZE - HAL_SETUP_CODE_SIZE - 1;
        }
        // Generate device name
        memcpy(name, platform, nameSize);
        name[0] = toupper(name[0]); // Ensure the first letter is capitalized
        name[nameSize++] = '-';
        memcpy(name + nameSize, code, codeSize);
        nameSize += codeSize;
    }
    memcpy(buf, name, std::min(nameSize, size));
    // Ensure the output buffer is always null-terminated
    if (nameSize < size) {
        buf[nameSize] = '\0';
    } else if (size > 0) {
        buf[size - 1] = '\0';
    }
    return nameSize;
}
