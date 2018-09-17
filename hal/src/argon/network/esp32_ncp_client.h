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

#include "wifi_ncp_client.h"

namespace particle {

class Esp32NcpClient: public WifiNcpClient {
public:
    explicit Esp32NcpClient(services::at::ArgonNcpAtClient* atParser);

    // Reimplemented from NcpClient
    int waitReady() override;
    void off() override;
    NcpState ncpState() override;
    int connect() override;
    void disconnect() override;
    NcpConnectionState connectionState() override;
    int getFirmwareVersionString(char* buf, size_t size) override;
    int getFirmwareModuleVersion(uint16_t* ver) override;
    int updateFirmware(InputStream* file, size_t size) override;
    int ncpId() const override;

    // Reimplemented from WifiNcpClient
    int connect(const char* bssid, const WifiCredentials* cred) override;
    int scan(ScanCallback callback) override;
    services::at::ArgonNcpAtClient* atParser() const override;

private:
    services::at::ArgonNcpAtClient* atParser_;
};

} // particle
