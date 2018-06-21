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

#include "str_util.h"

#include "nrf52840.h"

#include <algorithm>

namespace {

using namespace particle;

const uint32_t DEVICE_ID_PREFIX = 0x68ce0fe0;

} // namespace

unsigned HAL_device_ID(uint8_t* dest, unsigned destLen)
{
    const uint32_t id[3] = { DEVICE_ID_PREFIX, NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1] };
    static_assert(sizeof(id) == HAL_DEVICE_ID_SIZE, "");
    if (dest && destLen > 0) {
        memcpy(dest, id, std::min(destLen, sizeof(id)));
    }
    return HAL_DEVICE_ID_SIZE;
}

unsigned HAL_Platform_ID()
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
    //
    // TODO: Retrieve the serial number from the OTP memory
    //
    if (!isPrintable(serial, sizeof(serial))) {
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
