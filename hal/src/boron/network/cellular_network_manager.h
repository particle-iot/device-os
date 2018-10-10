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

#include <memory>

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

    CellularNetworkConfig& user(const char* user);
    const char* user() const;

    CellularNetworkConfig& password(const char* pwd);
    const char* password() const;

    bool isEmpty() const;

private:
    CString apn_;
    CString user_;
    CString pwd_;
};

class CellularNetworkManager {
public:
    explicit CellularNetworkManager();
    ~CellularNetworkManager();

    int init();
    void destroy();

    int connect();

    int setNetworkConfig(SimType simType, CellularNetworkConfig conf);
    int getNetworkConfig(SimType simType, CellularNetworkConfig* conf);
    int clearNetworkConfig(SimType simType);
    int clearNetworkConfig();

    int setActiveSimType(SimType simType);
    int getActiveSimType(SimType* simType);

    CellularNcpClient* ncpClient() const;

private:
    std::unique_ptr<CellularNcpClient> client_;
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

inline CellularNetworkConfig& CellularNetworkConfig::user(const char* user) {
    user_ = user;
    return *this;
}

inline const char* CellularNetworkConfig::user() const {
    return user_;
}

inline CellularNetworkConfig& CellularNetworkConfig::password(const char* pwd) {
    pwd_ = pwd;
    return *this;
}

inline const char* CellularNetworkConfig::password() const {
    return pwd_;
}

inline bool CellularNetworkConfig::isEmpty() const {
    return (!apn_ && !user_ && !pwd_);
}

inline CellularNcpClient* CellularNetworkManager::ncpClient() const {
    return client_.get();
}

} // particle
