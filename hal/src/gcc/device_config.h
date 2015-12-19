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

extern const char* DEVICE_ID;
extern const char* DEVICE_PRIVATE_KEY;
extern const char* SERVER_PUBLIC_KEY;


std::string get_configuration_value(const char* name);

void read_config_file(const char* config_name, void* data, size_t length);

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
    std::string periph_directory;
    uint16_t log_level = 0;
    ProtocolFactory protocol = PROTOCOL_LIGHTSSL;
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

    size_t hex2bin(const std::string& hex, uint8_t* dest, size_t destLen);

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
