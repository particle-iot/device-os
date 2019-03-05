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

#include "wlan_hal.h"

#include "network/ncp.h"
#include "wifi_network_manager.h"
#include "wifi_ncp_client.h"
#include "ifapi.h"

#include "system_network.h" // FIXME: For network_interface_index

#include "scope_guard.h"
#include "endian_util.h"
#include "check.h"

#include <algorithm>
#include <cstring>

#include "softap_http.h"

namespace {

using namespace particle;

bool isSupportedSecurityType(WLanSecurityType type) {
    switch (type) {
    case WLanSecurityType::WLAN_SEC_UNSEC:
    case WLanSecurityType::WLAN_SEC_WEP:
    case WLanSecurityType::WLAN_SEC_WPA:
    case WLanSecurityType::WLAN_SEC_WPA2:
        return true;
    default:
        return false;
    }
}

WifiSecurity toWifiSecurity(WLanSecurityType type) {
    switch (type) {
    case WLanSecurityType::WLAN_SEC_UNSEC:
        return WifiSecurity::NONE;
    case WLanSecurityType::WLAN_SEC_WEP:
        return WifiSecurity::WEP;
    case WLanSecurityType::WLAN_SEC_WPA:
        return WifiSecurity::WPA_PSK;
    case WLanSecurityType::WLAN_SEC_WPA2:
        return WifiSecurity::WPA2_PSK;
    default:
        return WifiSecurity::NONE;
    }
}

WLanSecurityType fromWifiSecurity(WifiSecurity sec) {
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
    default:
        return WLanSecurityType::WLAN_SEC_UNSEC;
    }
}

} // unnamed

int wlan_connect_init() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_connect_finalize() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

bool wlan_reset_credentials_store_required() {
    return false;
}

int wlan_reset_credentials_store() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void Set_NetApp_Timeout() {
}

int wlan_disconnect_now() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_activate() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_deactivate() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_connected_rssi() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_connected_info(void* reserved, wlan_connected_info_t* halInfo, void* reserved1) {
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    WifiNetworkInfo info;
    CHECK(client->getNetworkInfo(&info));
    const int32_t rssi = info.rssi();
    const int32_t noise = -90;
    halInfo->rssi = rssi * 100;
    halInfo->snr = (rssi - noise) * 100;
    halInfo->noise = noise * 100;
    halInfo->strength = std::min(std::max(2 * (rssi + 100), 0L), 100L) * 65535 / 100;
    halInfo->quality = std::min(std::max(halInfo->snr / 100 - 9, 0L), 31L) * 65535 / 31;
    return 0;
}

int wlan_clear_credentials() {
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    mgr->clearNetworkConfig();
    return 0;
}

int wlan_has_credentials() {
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    if (!mgr->hasNetworkConfig()) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    return 0;
}

int wlan_set_credentials(WLanCredentials* halCred) {
    if (!isSupportedSecurityType((WLanSecurityType)halCred->security) ||
            (halCred->security != WLanSecurityType::WLAN_SEC_UNSEC && halCred->password_len == 0) ||
            halCred->ssid_len == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    WifiCredentials cred;
    if (halCred->security != WLanSecurityType::WLAN_SEC_UNSEC) {
        const auto pwd = CString::wrap(strndup(halCred->password, halCred->password_len));
        cred.type(WifiCredentials::PASSWORD);
        cred.password(pwd);
    }
    const auto ssid = CString::wrap(strndup(halCred->ssid, halCred->ssid_len));
    WifiNetworkConfig conf;
    conf.ssid(ssid);
    conf.security(toWifiSecurity(halCred->security));
    conf.credentials(std::move(cred));
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    CHECK(mgr->setNetworkConfig(std::move(conf)));
    return 0;
}

void wlan_smart_config_init() {
}

void wlan_smart_config_cleanup() {
}

void wlan_set_error_count(uint32_t errorCount) {
}

bool wlan_smart_config_finalize() {
    return false;
}

int wlan_fetch_ipconfig(WLanConfig* conf) {
    if_t iface = nullptr;
    CHECK(if_get_by_index(NETWORK_INTERFACE_WIFI_STA, &iface));
    CHECK_TRUE(iface, SYSTEM_ERROR_INVALID_STATE);
    unsigned flags = 0;
    CHECK(if_get_flags(iface, &flags));
    // MAC address
    sockaddr_ll hwAddr = {};
    CHECK(if_get_lladdr(iface, &hwAddr));
    CHECK_TRUE((size_t)hwAddr.sll_halen == MAC_ADDRESS_SIZE, SYSTEM_ERROR_UNKNOWN);
    memcpy(conf->nw.uaMacAddr, hwAddr.sll_addr, MAC_ADDRESS_SIZE);
    CHECK_TRUE((flags & IFF_UP) && (flags & IFF_LOWER_UP), SYSTEM_ERROR_INVALID_STATE);
    // IP address
    if_addrs* ifAddrList = nullptr;
    CHECK(if_get_addrs(iface, &ifAddrList));
    SCOPE_GUARD({
        if_free_if_addrs(ifAddrList);
    });
    if_addr* ifAddr = nullptr;
    for (if_addrs* i = ifAddrList; i; i = i->next) {
        if (i->if_addr->addr->sa_family == AF_INET) { // Skip non-IPv4 addresses
            ifAddr = i->if_addr;
            break;
        }
    }
    CHECK_TRUE(ifAddr, SYSTEM_ERROR_INVALID_STATE);
    auto sockAddr = (const sockaddr_in*)ifAddr->addr;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    static_assert(sizeof(conf->nw.aucIP.ipv4) == sizeof(sockAddr->sin_addr), "");
    memcpy(&conf->nw.aucIP.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucIP.ipv4 = reverseByteOrder(conf->nw.aucIP.ipv4);
    conf->nw.aucIP.v = 4;
    // Subnet mask
    sockAddr = (const sockaddr_in*)ifAddr->netmask;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    memcpy(&conf->nw.aucSubnetMask.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucSubnetMask.ipv4 = reverseByteOrder(conf->nw.aucSubnetMask.ipv4);
    conf->nw.aucSubnetMask.v = 4;
    // Gateway address
    sockAddr = (const sockaddr_in*)ifAddr->gw;
    CHECK_TRUE(sockAddr, SYSTEM_ERROR_INVALID_STATE);
    memcpy(&conf->nw.aucDefaultGateway.ipv4, &sockAddr->sin_addr, sizeof(sockAddr->sin_addr));
    conf->nw.aucDefaultGateway.ipv4 = reverseByteOrder(conf->nw.aucDefaultGateway.ipv4);
    conf->nw.aucDefaultGateway.v = 4;
    // SSID
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    WifiNetworkInfo info;
    CHECK(client->getNetworkInfo(&info));
    if (info.ssid()) {
        size_t n = strlen(info.ssid());
        if (n >= sizeof(conf->uaSSID)) {
            n = sizeof(conf->uaSSID) - 1;
        }
        memcpy(conf->uaSSID, info.ssid(), n);
        conf->uaSSID[n] = '\0';
    }
    // BSSID
    static_assert(sizeof(conf->BSSID) == MAC_ADDRESS_SIZE, "");
    memcpy(conf->BSSID, &info.bssid(), MAC_ADDRESS_SIZE);
    return 0;
}

void wlan_setup() {
}

void SPARK_WLAN_SmartConfigProcess() {
}

void HAL_WLAN_notify_simple_config_done() {
}

int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna) {
    if (antenna != ANT_AUTO) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

WLanSelectAntenna_TypeDef wlan_get_antenna(void* reserved) {
    return ANT_AUTO;
}

void wlan_connect_cancel(bool called_from_isr) {
}

void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved) {
}

IPAddressSource wlan_get_ipaddress_source(void* reserved) {
    return DYNAMIC_IP;
}

void wlan_set_ipaddress(const HAL_IPAddress* device, const HAL_IPAddress* netmask,
        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1, const HAL_IPAddress* dns2, void* reserved) {
}

int wlan_get_ipaddress(IPConfig* conf, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_scan(wlan_scan_result_t callback, void* cookie) {
    struct Data {
        wlan_scan_result_t callback;
        void* data;
        size_t count;
    };
    Data d = {
        .callback = callback,
        .data = cookie,
        .count = 0
    };
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = mgr->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    CHECK(client->scan([](WifiScanResult result, void* data) {
        WiFiAccessPoint ap = {};
        ap.size = sizeof(WiFiAccessPoint);
        if (result.ssid()) {
            ap.ssidLength = strlen(result.ssid());
            if (ap.ssidLength >= sizeof(ap.ssid)) {
                ap.ssidLength = sizeof(ap.ssid) - 1;
            }
            memcpy(ap.ssid, result.ssid(), ap.ssidLength);
            ap.ssid[ap.ssidLength] = '\0';
        }
        static_assert(sizeof(ap.bssid) == MAC_ADDRESS_SIZE, "");
        memcpy(ap.bssid, &result.bssid(), MAC_ADDRESS_SIZE);
        ap.security = fromWifiSecurity(result.security());
        // FIXME: As the ESP32 doesn't return the cipher type of an AP, we manually set it here.
        if (ap.security == WLAN_SEC_WPA || ap.security == WLAN_SEC_WPA2) {
            ap.cipher = WLAN_CIPHER_AES_TKIP;
        }
        else {
            ap.cipher = WLAN_CIPHER_NOT_SET;
        }
        ap.channel = result.channel();
        ap.rssi = result.rssi();
        const auto d = (Data*)data;
        d->callback(&ap, d->data);
        ++d->count;
        return 0;
    }, &d));
    return d.count;
}

int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data) {
    struct Data {
        wlan_scan_result_t callback;
        void* data;
        size_t count;
    };
    Data d = {
        .callback = callback,
        .data = callback_data,
        .count = 0
    };
    const auto mgr = wifiNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    CHECK(mgr->getNetworkConfig([](WifiNetworkConfig conf, void* data) {
        WiFiAccessPoint ap = {};
        ap.size = sizeof(WiFiAccessPoint);
        if (conf.ssid()) {
            ap.ssidLength = strlen(conf.ssid());
            if (ap.ssidLength >= sizeof(ap.ssid)) {
                ap.ssidLength = sizeof(ap.ssid) - 1;
            }
            memcpy(ap.ssid, conf.ssid(), ap.ssidLength);
            ap.ssid[ap.ssidLength] = '\0';
        }
        static_assert(sizeof(ap.bssid) == MAC_ADDRESS_SIZE, "");
        memcpy(ap.bssid, &conf.bssid(), MAC_ADDRESS_SIZE);
        ap.security = fromWifiSecurity(conf.security());
        // FIXME: As the ESP32 doesn't return the cipher type of an AP, we manually set it here.
        if (ap.security == WLAN_SEC_WPA || ap.security == WLAN_SEC_WPA2) {
            ap.cipher = WLAN_CIPHER_AES_TKIP;
        }
        else {
            ap.cipher = WLAN_CIPHER_NOT_SET;
        }
        const auto d = (Data*)data;
        d->callback(&ap, d->data);
        ++d->count;
        return 0;
    }, &d));
    return d.count;
}

bool isWiFiPowersaveClockDisabled() {
    return false;
}

int wlan_restart(void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_set_hostname(const char* hostname, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int wlan_get_hostname(char* buf, size_t len, void* reserved) {
    if (len > 0) {
        buf[0] = '\0';
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int softap_set_application_page_handler(PageProvider* provider, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}
