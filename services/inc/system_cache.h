/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "tlv_file.h"

namespace particle { namespace services {

enum class SystemCacheKey : uint16_t {
    WIFI_NCP_FIRMWARE_VERSION = 0x0000,
    WIFI_NCP_MAC_ADDRESS = 0x0001
};

class SystemCache {
public:
    static SystemCache& instance();

    // Just straight up proxying to TlvFile
    // TODO: add support for multiple values for a key?
    int get(SystemCacheKey key, void* value, size_t length);
    int set(SystemCacheKey key, const void* value, size_t length);
    int del(SystemCacheKey key);

    SystemCache(SystemCache const&) = delete;
    SystemCache(SystemCache&&) = delete;
    SystemCache& operator=(SystemCache const&) = delete;
    SystemCache& operator=(SystemCache &&) = delete;

protected:
    SystemCache();

private:
    settings::TlvFile tlv_;
};

} } // particle::service
