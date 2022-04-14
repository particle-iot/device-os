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

#pragma once

#include "addr_util.h"
#include "c_string.h"

#include <cstdint>

namespace particle {

class WifiNcpClient;

// Maximum number of WiFi network settings that can be saved to a persistent storage
const unsigned MAX_CONFIGURED_WIFI_NETWORK_COUNT = 10;

// Maximum length of an SSID
const size_t MAX_SSID_SIZE = 32;

// Maximum length of a WPA/WPA2 key
const size_t MAX_WPA_WPA2_PSK_SIZE = 64;

enum class WifiSecurity {
    NONE = 0,
    WEP = 1,
    WPA_PSK = 2,
    WPA2_PSK = 3,
    WPA_WPA2_PSK = 4
};

class WifiCredentials {
public:
    enum Type {
        NONE = 0,
        PASSWORD = 1
    };

    WifiCredentials();

    WifiCredentials& password(const char* pwd);
    const char* password() const;

    WifiCredentials& type(Type type);
    Type type() const;

private:
    CString pwd_;
    Type type_;
};

class WifiNetworkConfig {
public:
    WifiNetworkConfig();

    WifiNetworkConfig& ssid(const char* ssid);
    const char* ssid() const;

    WifiNetworkConfig& bssid(const MacAddress& bssid);
    const MacAddress& bssid() const;

    WifiNetworkConfig& security(WifiSecurity sec);
    WifiSecurity security() const;

    WifiNetworkConfig& credentials(WifiCredentials cred);
    const WifiCredentials& credentials() const;

private:
    CString ssid_;
    MacAddress bssid_;
    WifiCredentials cred_;
    WifiSecurity sec_;
};

class WifiNetworkInfo {
public:
    WifiNetworkInfo();

    WifiNetworkInfo& ssid(const char* ssid);
    const char* ssid() const;

    WifiNetworkInfo& bssid(const MacAddress& bssid);
    const MacAddress& bssid() const;

    WifiNetworkInfo& channel(int channel);
    int channel() const;

    WifiNetworkInfo& rssi(int rssi);
    int rssi() const;

private:
    CString ssid_;
    MacAddress bssid_;
    int channel_;
    int rssi_;
};

class WifiScanResult {
public:
    WifiScanResult();

    WifiScanResult& ssid(const char* ssid);
    const char* ssid() const;

    WifiScanResult& bssid(const MacAddress& bssid);
    const MacAddress& bssid() const;

    WifiScanResult& security(WifiSecurity sec);
    WifiSecurity security() const;

    WifiScanResult& channel(int channel);
    int channel() const;

    WifiScanResult& rssi(int rssi);
    int rssi() const;

private:
    CString ssid_;
    MacAddress bssid_;
    WifiSecurity sec_;
    int channel_;
    int rssi_;
};

typedef int(*WifiScanCallback)(WifiScanResult result, void* data);

class WifiNetworkManager {
public:
    typedef int(*GetNetworkConfigCallback)(WifiNetworkConfig conf, void* data);

    explicit WifiNetworkManager(WifiNcpClient* client);
    ~WifiNetworkManager();

    int connect(const char* ssid);
    int connect();

    static int setNetworkConfig(WifiNetworkConfig conf);
    static int getNetworkConfig(const char* ssid, WifiNetworkConfig* conf);
    static int getNetworkConfig(GetNetworkConfigCallback callback, void* data);
    static void removeNetworkConfig(const char* ssid);
    static void clearNetworkConfig();
    static bool hasNetworkConfig();

    WifiNcpClient* ncpClient() const;

private:
    WifiNcpClient* client_;
};

inline WifiCredentials::WifiCredentials() :
        type_(Type::NONE) {
}

inline WifiCredentials& WifiCredentials::password(const char* pwd) {
    pwd_ = pwd;
    return *this;
}

inline const char* WifiCredentials::password() const {
    return pwd_;
}

inline WifiCredentials& WifiCredentials::type(Type type) {
    type_ = type;
    return *this;
}

inline WifiCredentials::Type WifiCredentials::type() const {
    return type_;
}

inline WifiNetworkConfig::WifiNetworkConfig() :
        bssid_(INVALID_MAC_ADDRESS),
        sec_(WifiSecurity::NONE) {
}

inline WifiNetworkConfig& WifiNetworkConfig::ssid(const char* ssid) {
    ssid_ = ssid;
    return *this;
}

inline const char* WifiNetworkConfig::ssid() const {
    return ssid_;
}

inline WifiNetworkConfig& WifiNetworkConfig::bssid(const MacAddress& bssid) {
    bssid_ = bssid;
    return *this;
}

inline const MacAddress& WifiNetworkConfig::bssid() const {
    return bssid_;
}

inline WifiNetworkConfig& WifiNetworkConfig::security(WifiSecurity sec) {
    sec_ = sec;
    return *this;
}

inline WifiSecurity WifiNetworkConfig::security() const {
    return sec_;
}

inline WifiNetworkConfig& WifiNetworkConfig::credentials(WifiCredentials cred) {
    cred_ = std::move(cred);
    return *this;
}

inline const WifiCredentials& WifiNetworkConfig::credentials() const {
    return cred_;
}

inline WifiNetworkInfo::WifiNetworkInfo() :
        bssid_(INVALID_MAC_ADDRESS),
        channel_(0),
        rssi_(0) {
}

inline WifiNetworkInfo& WifiNetworkInfo::ssid(const char* ssid) {
    ssid_ = ssid;
    return *this;
}

inline const char* WifiNetworkInfo::ssid() const {
    return ssid_;
}

inline WifiNetworkInfo& WifiNetworkInfo::bssid(const MacAddress& bssid) {
    bssid_ = bssid;
    return *this;
}

inline const MacAddress& WifiNetworkInfo::bssid() const {
    return bssid_;
}

inline WifiNetworkInfo& WifiNetworkInfo::channel(int channel) {
    channel_ = channel;
    return *this;
}

inline int WifiNetworkInfo::channel() const {
    return channel_;
}

inline WifiNetworkInfo& WifiNetworkInfo::rssi(int rssi) {
    rssi_ = rssi;
    return *this;
}

inline int WifiNetworkInfo::rssi() const {
    return rssi_;
}

inline WifiScanResult::WifiScanResult() :
        bssid_(INVALID_MAC_ADDRESS),
        sec_(WifiSecurity::NONE),
        channel_(0),
        rssi_(0) {
}

inline WifiScanResult& WifiScanResult::ssid(const char* ssid) {
    ssid_ = ssid;
    return *this;
}

inline const char* WifiScanResult::ssid() const {
    return ssid_;
}

inline WifiScanResult& WifiScanResult::bssid(const MacAddress& bssid) {
    bssid_ = bssid;
    return *this;
}

inline const MacAddress& WifiScanResult::bssid() const {
    return bssid_;
}

inline WifiScanResult& WifiScanResult::security(WifiSecurity sec) {
    sec_ = sec;
    return *this;
}

inline WifiSecurity WifiScanResult::security() const {
    return sec_;
}

inline WifiScanResult& WifiScanResult::channel(int channel) {
    channel_ = channel;
    return *this;
}

inline int WifiScanResult::channel() const {
    return channel_;
}

inline WifiScanResult& WifiScanResult::rssi(int rssi) {
    rssi_ = rssi;
    return *this;
}

inline int WifiScanResult::rssi() const {
    return rssi_;
}

inline int WifiNetworkManager::connect() {
    return connect(nullptr);
}

inline WifiNcpClient* WifiNetworkManager::ncpClient() const {
    return client_;
}

} // particle
