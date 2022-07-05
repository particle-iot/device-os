/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#pragma once

#include <string>
#include <stdexcept>
#include <cstring>
#include "filesystem.h"
#include "spark_protocol_functions.h"

/**
 * Reads the device configuration and returns true if the device should start.
 * @param argc
 * @param argv
 * @return
 */
bool read_device_config(int argc, char* argv[]);


/**
 * The external configuration data.
 */
struct Configuration
{
    std::string device_id;
    std::string device_key;
    std::string server_key;
    std::string module_info;
    uint16_t log_level = 0;
    ProtocolFactory protocol = PROTOCOL_LIGHTSSL;
    int platform_id;
};


/**
 * The device configuration in internal form.
 */
struct DeviceConfig
{
    uint8_t device_id[12];
    uint8_t device_key[1024];
    uint8_t server_key[1024];
    ProtocolFactory protocol;
    int platform_id;

    void read(Configuration& configuration);

    size_t fetchDeviceID(uint8_t* dest, size_t destLen)
    {
        if (destLen>12)
            destLen = 12;
        if (dest)
            memcpy(dest, device_id, destLen);
        return 12;
    }

    ProtocolFactory get_protocol() { return protocol; }
};



extern DeviceConfig deviceConfig;
