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

#include "c_string.h"

#include <cstring>
#include <cstdint>

namespace particle {

class WifiNcpClient;

const size_t BSSID_SIZE = 6;

struct Bssid {
    uint8_t data[BSSID_SIZE];
};

const Bssid INVALID_BSSID = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

enum class WifiSecurity {
    NONE = 0,
    WEP = 1,
    WPA_PSK = 2,
    WPA2_PSK = 3,
    WPA_WPA2_PSK = 4,
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

    WifiNetworkConfig& security(WifiSecurity sec);
    WifiSecurity security() const;

    WifiNetworkConfig& credentials(WifiCredentials cred);
    const WifiCredentials& credentials() const;

private:
    CString ssid_;
    WifiCredentials cred_;
    WifiSecurity sec_;
};

class WifiNetworkInfo {
public:
    WifiNetworkInfo();

    WifiNetworkInfo& ssid(const char* ssid);
    const char* ssid() const;

    WifiNetworkInfo& bssid(const Bssid& bssid);
    const Bssid& bssid() const;

    WifiNetworkInfo& channel(int channel);
    int channel() const;

    WifiNetworkInfo& rssi(int rssi);
    int rssi() const;

private:
    CString ssid_;
    Bssid bssid_;
    int channel_;
    int rssi_;
};

class WifiScanResult {
public:
    WifiScanResult();

    WifiScanResult& ssid(const char* ssid);
    const char* ssid() const;

    WifiScanResult& bssid(const Bssid& bssid);
    const Bssid& bssid() const;

    WifiScanResult& security(WifiSecurity sec);
    WifiSecurity security() const;

    WifiScanResult& channel(int channel);
    int channel() const;

    WifiScanResult& rssi(int rssi);
    int rssi() const;

private:
    CString ssid_;
    Bssid bssid_;
    WifiSecurity sec_;
    int channel_;
    int rssi_;
};

typedef int(*WifiScanCallback)(WifiScanResult result, void* data);

class WifiManager {
public:
    enum AdapterState {
        OFF = 0,
        ON = 1
    };

    enum ConnectionState {
        DISCONNECTED = 0,
        CONNECTED = 1
    };

    typedef int(*GetConfiguredNetworksCallback)(WifiNetworkConfig conf, void* data);

    explicit WifiManager(WifiNcpClient* ncpClient);
    ~WifiManager();

    int on();
    void off();
    AdapterState adapterState();

    int connect(const char* ssid);
    int connect();
    void disconnect();
    ConnectionState connectionState();

    int getNetworkInfo(WifiNetworkInfo* info);

    int setNetworkConfig(const WifiNetworkConfig& conf);
    int getNetworkConfig(const char* ssid, WifiNetworkConfig* conf);

    int getConfiguredNetworks(GetConfiguredNetworksCallback callback, void* data);
    void removeConfiguredNetwork(const char* ssid);
    void clearConfiguredNetworks();

    int scan(WifiScanCallback callback, void* data);

    // TODO: Move this method to a subclass
    WifiNcpClient* ncpClient() const;

private:
    WifiNcpClient* ncpClient_;
};

inline bool operator==(const Bssid& bssid1, const Bssid& bssid2) {
    return (memcmp(bssid1.data, bssid2.data, BSSID_SIZE) == 0);
}

inline bool operator!=(const Bssid& bssid1, const Bssid& bssid2) {
    return !(bssid1 == bssid2);
}

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
        sec_(WifiSecurity::NONE) {
}

inline WifiNetworkConfig& WifiNetworkConfig::ssid(const char* ssid) {
    ssid_ = ssid;
    return *this;
}

inline const char* WifiNetworkConfig::ssid() const {
    return ssid_;
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
        bssid_(INVALID_BSSID),
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

inline WifiNetworkInfo& WifiNetworkInfo::bssid(const Bssid& bssid) {
    bssid_ = bssid;
    return *this;
}

inline const Bssid& WifiNetworkInfo::bssid() const {
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
        bssid_(INVALID_BSSID),
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

inline WifiScanResult& WifiScanResult::bssid(const Bssid& bssid) {
    bssid_ = bssid;
    return *this;
}

inline const Bssid& WifiScanResult::bssid() const {
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

inline WifiNcpClient* WifiManager::ncpClient() const {
    return ncpClient_;
}

} // particle
