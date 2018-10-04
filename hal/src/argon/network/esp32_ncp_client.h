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
#include "at_parser.h"

namespace particle {

class Esp32NcpClient: public WifiNcpClient {
public:
    // Reimplemented from NcpClient
    int init(const NcpClientConfig& conf) override;
    void destroy() override;
    int on() override;
    void off() override;
    NcpState ncpState() override;
    void disconnect() override;
    NcpConnectionState connectionState() override;
    int getFirmwareVersionString(char* buf, size_t size) override;
    int getFirmwareModuleVersion(uint16_t* ver) override;
    int updateFirmware(InputStream* file, size_t size) override;
    AtParser* atParser() override;
    void lock() override;
    void unlock() override;
    int ncpId() const override;

    // Reimplemented from WifiNcpClient
    int connect(const char* ssid, const Bssid& bssid, WifiSecurity sec, const WifiCredentials& cred) override;
    int getNetworkInfo(WifiNetworkInfo* info) override;
    int scan(WifiScanCallback callback, void* data) override;

private:
    AtParser atParser_;
};

} // particle
