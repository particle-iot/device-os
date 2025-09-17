/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

typedef enum {
    WLAN_SEC_UNSEC = 0,
    WLAN_SEC_WEP,
    WLAN_SEC_WPA,
    WLAN_SEC_WPA2,
    WLAN_SEC_WPA_ENTERPRISE,
    WLAN_SEC_WPA2_ENTERPRISE,
    WLAN_SEC_WPA3,
    WLAN_SEC_NOT_SET = 0xFF
} WLanSecurityType;

namespace particle {

enum class WifiSecurity {
    NONE = 0,
    WEP = 1,
    WPA_PSK = 2,
    WPA2_PSK = 3,
    WPA_WPA2_PSK = 4,
    WPA3_PSK = 5,
    WPA2_WPA3_PSK = 6
};

inline WLanSecurityType fromWifiSecurity(WifiSecurity sec) {
    switch (sec) {
    case WifiSecurity::NONE:
        return WLanSecurityType::WLAN_SEC_UNSEC;
    case WifiSecurity::WEP:
        return WLanSecurityType::WLAN_SEC_WEP;
    case WifiSecurity::WPA_PSK:
        return WLanSecurityType::WLAN_SEC_WPA;
    case WifiSecurity::WPA2_PSK:
    case WifiSecurity::WPA_WPA2_PSK:
        return WLanSecurityType::WLAN_SEC_WPA2;
    case WifiSecurity::WPA3_PSK:
    case WifiSecurity::WPA2_WPA3_PSK:
        return WLanSecurityType::WLAN_SEC_WPA3;
    default:
        return WLanSecurityType::WLAN_SEC_UNSEC;
    }
}

} // namespace particle
