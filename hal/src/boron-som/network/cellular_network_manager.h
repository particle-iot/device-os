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

    bool isValid() const;

private:
    CString apn_;
    CString user_;
    CString pwd_;
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
}

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

inline bool CellularNetworkConfig::isValid() const {
    return (apn_ && user_ && pwd_);
}

inline CellularNcpClient* CellularNetworkManager::ncpClient() const {
    return client_;
}

} // particle
