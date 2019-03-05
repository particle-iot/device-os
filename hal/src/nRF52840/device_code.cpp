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

#include "deviceid_hal.h"

#include "str_util.h"
#include "preprocessor.h"
#include "debug.h"

#include "dct.h"

#include <algorithm>
#include <cstring>

namespace {

using namespace particle;

// TODO: Rename ssid_prefix to device_name in DCT?
const auto DEVICE_NAME_DCT_SIZE = DCT_SSID_PREFIX_SIZE;
const auto DEVICE_NAME_DCT_OFFSET = DCT_SSID_PREFIX_OFFSET;

const auto DEVICE_NAME_MAX_SIZE = DEVICE_NAME_DCT_SIZE - 1;

const auto SETUP_CODE_SIZE = DCT_DEVICE_CODE_SIZE;
const auto SETUP_CODE_DCT_OFFSET = DCT_DEVICE_CODE_OFFSET;

int getDeviceSetupCode(char* code) {
    // Check if the device setup code is initialized in the DCT
    int ret = dct_read_app_data_copy(SETUP_CODE_DCT_OFFSET, code, SETUP_CODE_SIZE);
    if (ret < 0 || !isPrintable(code, SETUP_CODE_SIZE)) {
        // Check the OTP memory
        char serial[HAL_DEVICE_SERIAL_NUMBER_SIZE] = {};
        ret = hal_get_device_serial_number(serial, sizeof(serial), nullptr);
        if (ret >= (int)SETUP_CODE_SIZE && (size_t)ret <= sizeof(serial)) {
            // Use last characters of the serial number as the setup code
            memcpy(code, &serial[ret - SETUP_CODE_SIZE], SETUP_CODE_SIZE);
        } else {
            // Return a dummy setup code
            memset(code, 'X', SETUP_CODE_SIZE);
        }
    }
    return 0;
}

} // ::

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
        char code[SETUP_CODE_SIZE] = {};
        ret = getDeviceSetupCode(code);
        if (ret < 0) {
            return ret;
        }
        // Get platform name
        const char* const platform = PP_STR(PLATFORM_NAME);
        nameSize = sizeof(PP_STR(PLATFORM_NAME)) - 1; // Exclude term. null
        if (nameSize + SETUP_CODE_SIZE + 1 > DEVICE_NAME_MAX_SIZE) { // Reserve 1 character for '-'
            nameSize = DEVICE_NAME_MAX_SIZE - SETUP_CODE_SIZE - 1;
        }
        // Generate device name
        memcpy(name, platform, nameSize);
        name[0] = toupper(name[0]); // Ensure the first letter is capitalized
        name[nameSize++] = '-';
        memcpy(name + nameSize, code, SETUP_CODE_SIZE);
        nameSize += SETUP_CODE_SIZE;
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
