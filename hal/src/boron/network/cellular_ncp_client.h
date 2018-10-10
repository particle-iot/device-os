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
#include "cellular_network_manager.h"

namespace particle {

class CellularNcpClientConfig: public NcpClientConfig {
public:
    CellularNcpClientConfig();

    CellularNcpClientConfig& simType(SimType type);
    SimType simType() const;

private:
    SimType simType_;
};

class CellularNcpClient: public NcpClient {
public:
    virtual int connect(const CellularNetworkConfig& conf) = 0;
    virtual int getIccid(char* buf, size_t size) = 0;
};

inline CellularNcpClientConfig::CellularNcpClientConfig() :
        simType_(SimType::INTERNAL) {
}

inline CellularNcpClientConfig& CellularNcpClientConfig::simType(SimType type) {
    simType_ = type;
    return *this;
}

inline SimType CellularNcpClientConfig::simType() const {
    return simType_;
}

} // particle
