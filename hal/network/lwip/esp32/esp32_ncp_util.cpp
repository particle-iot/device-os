/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

// FIXME
#undef LOG_COMPILE_TIME_LEVEL

#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#include "network/ncp/ncp_client.h"
#include "system_cache.h"
#include "check.h"
#include <algorithm>

namespace particle {

using namespace particle::services;

int wifiNcpUpdateInfoCache(uint16_t version, MacAddress mac) {
    int result = 0;
    uint16_t cached = 0;
    int versionRes = SystemCache::instance().get(SystemCacheKey::WIFI_NCP_FIRMWARE_VERSION, &cached, sizeof(cached));
    if (versionRes != sizeof(cached) || cached != version) {
        LOG(INFO, "Updating ESP32 cached module version to %u", version);
        versionRes = SystemCache::instance().set(SystemCacheKey::WIFI_NCP_FIRMWARE_VERSION, &version, sizeof(version));
    }
    if (versionRes < 0) {
        result = versionRes;
    }
    MacAddress cachedMac = {};
    int macRes = SystemCache::instance().get(SystemCacheKey::WIFI_NCP_MAC_ADDRESS, cachedMac.data, sizeof(cachedMac.data));
    if (macRes != sizeof(cachedMac.data) || cachedMac != mac) {
        char macStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        macAddressToString(mac, macStr, sizeof(macStr));
        LOG(INFO, "Updating ESP32 cached MAC to %s", macStr);
        macRes = SystemCache::instance().set(SystemCacheKey::WIFI_NCP_MAC_ADDRESS, mac.data, sizeof(mac.data));
    }
    if (macRes < 0) {
        result = macRes;
    }
    return result;
}

int wifiNcpGetCachedMacAddress(MacAddress* ncpMac) {
    MacAddress mac = INVALID_MAC_ADDRESS;
    int res = SystemCache::instance().get(SystemCacheKey::WIFI_NCP_MAC_ADDRESS, mac.data, sizeof(mac.data));
    if (res != sizeof(mac.data)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if (ncpMac) {
        *ncpMac = mac;
    }
    return 0;
}

int wifiNcpGetCachedModuleVersion(uint16_t* ncpVersion) {
    uint16_t version = 0;
    int res = SystemCache::instance().get(SystemCacheKey::WIFI_NCP_FIRMWARE_VERSION, &version, sizeof(version));
    if (res != sizeof(version)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    LOG(TRACE, "Cached ESP32 NCP firmware version: %d", (int)version);
    if (ncpVersion) {
        *ncpVersion = version;
    }
    return 0;
}

int wifiNcpInvalidateInfoCache() {
    int result = 0;
    LOG(TRACE, "Invalidating cached ESP32 NCP info");
    int versionError = SystemCache::instance().del(SystemCacheKey::WIFI_NCP_FIRMWARE_VERSION);
    if (versionError < 0) {
        result = versionError;
    }
    int macError = SystemCache::instance().del(SystemCacheKey::WIFI_NCP_MAC_ADDRESS);
    if (macError < 0) {
        result = macError;
    }
    return result;
}

} // particle
