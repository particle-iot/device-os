/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#include "network_config_db.h"

namespace particle {

class CellularNcpClient;

enum class SimType {
    INVALID = 0,
    INTERNAL = 1,
    EXTERNAL = 2
};

class CellularNetworkConfig {
public:
    CellularNetworkConfig();

    CellularNetworkConfig& apn(const char* apn);
    const char* apn() const;
    bool hasApn() const;

    CellularNetworkConfig& user(const char* user);
    const char* user() const;
    bool hasUser() const;

    CellularNetworkConfig& password(const char* pwd);
    const char* password() const;
    bool hasPassword() const;

    CellularNetworkConfig& netProv(CellularNetworkProv net_prov);
    CellularNetworkProv netProv() const;
    bool hasNetProv() const;

    bool isValid() const;

private:
    CString apn_;
    CString user_;
    CString pwd_;
    CellularNetworkProv net_prov_;
};

class CellularNetworkManager {
public:
    explicit CellularNetworkManager(CellularNcpClient* client);
    ~CellularNetworkManager();

    int connect();

    CellularNcpClient* ncpClient() const;

    static int setNetworkConfig(SimType sim, CellularNetworkConfig conf);
    static int getNetworkConfig(SimType sim, CellularNetworkConfig* conf);
    static int clearNetworkConfig(SimType sim);
    static int clearNetworkConfig();

    static int setActiveSim(SimType sim);
    static int getActiveSim(SimType* sim);

private:
    CellularNcpClient* client_;
};

inline CellularNetworkConfig::CellularNetworkConfig() {
    net_prov_ = CellularNetworkProv::NONE;
}

// APN
inline CellularNetworkConfig& CellularNetworkConfig::apn(const char* apn) {
    apn_ = apn;
    return *this;
}
inline const char* CellularNetworkConfig::apn() const {
    return apn_;
}
inline bool CellularNetworkConfig::hasApn() const {
    return apn_ && *apn_;
}

// USERNAME
inline CellularNetworkConfig& CellularNetworkConfig::user(const char* user) {
    user_ = user;
    return *this;
}
inline const char* CellularNetworkConfig::user() const {
    return user_;
}
inline bool CellularNetworkConfig::hasUser() const {
    return user_ && *user_;
}

// PASSWORD
inline CellularNetworkConfig& CellularNetworkConfig::password(const char* pwd) {
    pwd_ = pwd;
    return *this;
}
inline const char* CellularNetworkConfig::password() const {
    return pwd_;
}
inline bool CellularNetworkConfig::hasPassword() const {
    return pwd_ && *pwd_;
}

// NETWORK PROVIDER
inline CellularNetworkConfig& CellularNetworkConfig::netProv(CellularNetworkProv net_prov) {
    net_prov_ = net_prov;
    return *this;
}
inline CellularNetworkProv CellularNetworkConfig::netProv() const {
    return net_prov_;
}
inline bool CellularNetworkConfig::hasNetProv() const {
    return net_prov_ > CellularNetworkProv::NONE && net_prov_ < CellularNetworkProv::MAX;
}

// IS_VALID
inline bool CellularNetworkConfig::isValid() const {
    return (apn_ && user_ && pwd_);
}

inline CellularNcpClient* CellularNetworkManager::ncpClient() const {
    return client_;
}

} // particle
