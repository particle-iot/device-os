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

#include "ncp_client.h"

namespace particle {

namespace services {

namespace at {

class ArgonNcpAtClient;

} // particle::services::at

} // particle::services

enum class WiFiEncryption {
    NONE = 0,
    WEP = 1,
    WPA_PSK = 2,
    WPA2_PSK = 3,
    WPA_WPA2_PSK = 4,
    WPA2_ENT = 5
};

class WifiCredentials {
public:
    WifiCredentials();

    WifiCredentials& password(const char* pwd);
    const char* password() const;

private:
    const char* pwd_;
};

class WifiScanResult {
public:
    WifiScanResult();

    WifiScanResult& ssid(const char* ssid);
    const char* ssid() const;

    WifiScanResult& bssid(const char* bssid);
    const char* bssid() const;

    WifiScanResult& encryption(WiFiEncryption encr);
    WiFiEncryption encryption() const;

    WifiScanResult& channel(int channel);
    int channel() const;

    WifiScanResult& rssi(int rssi);
    int rssi() const;

private:
    const char* ssid_;
    const char* bssid_;
    WiFiEncryption encr_;
    int channel_;
    int rssi_;
};

class WifiNcpClient: public NcpClient {
public:
    typedef void(*ScanCallback)(const WifiScanResult* result);

    virtual int connect(const char* bssid, const WifiCredentials* cred = nullptr) = 0;

    virtual int scan(ScanCallback callback) = 0;

    // TODO: Move this method to a base class
    virtual services::at::ArgonNcpAtClient* atParser() const = 0;
};

inline WifiScanResult::WifiScanResult() :
        ssid_(nullptr),
        bssid_(nullptr),
        encr_(WiFiEncryption::NONE),
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

inline WifiScanResult& WifiScanResult::bssid(const char* bssid) {
    bssid_ = bssid;
    return *this;
}

inline const char* WifiScanResult::bssid() const {
    return bssid_;
}

inline WifiScanResult& WifiScanResult::encryption(WiFiEncryption encr) {
    encr_ = encr;
    return *this;
}

inline WiFiEncryption WifiScanResult::encryption() const {
    return encr_;
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

inline WifiCredentials::WifiCredentials() :
        pwd_(nullptr) {
}

inline WifiCredentials& WifiCredentials::password(const char* pwd) {
    pwd_ = pwd;
    return *this;
}

inline const char* WifiCredentials::password() const {
    return pwd_;
}

} // particle
