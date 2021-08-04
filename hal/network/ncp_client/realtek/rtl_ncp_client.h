
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

#include "network/ncp/wifi/wifi_ncp_client.h"
#include "at_parser.h"
#include "platform_ncp.h"
#include "spark_wiring_thread.h"
#include <memory>
#include "static_recursive_mutex.h"

namespace particle {

class SerialStream;

class RealtekNcpClient: public WifiNcpClient {
public:
    RealtekNcpClient();
    ~RealtekNcpClient();

    // Reimplemented from NcpClient
    int init(const NcpClientConfig& conf) override;
    void destroy() override;
    int on() override;
    int off() override;
    int enable() override;
    void disable() override;
    NcpState ncpState() override;
    NcpPowerState ncpPowerState() override;
    int disconnect() override;
    NcpConnectionState connectionState() override;
    void lock() override;
    void unlock() override;
    int ncpId() const override;

    // Reimplemented from WifiNcpClient
    int connect(const char* ssid, const MacAddress& bssid, WifiSecurity sec, const WifiCredentials& cred) override;
    int getNetworkInfo(WifiNetworkInfo* info) override;
    int scan(WifiScanCallback callback, void* data) override;
    int getMacAddress(MacAddress* addr) override;

    virtual int getFirmwareVersionString(char* buf, size_t size) override;
    virtual int getFirmwareModuleVersion(uint16_t* ver) override;
    virtual int updateFirmware(InputStream* file, size_t size) override;

    virtual int dataChannelWrite(int id, const uint8_t* data, size_t size) override;
    virtual int dataChannelFlowControl(bool state) override;
    virtual void processEvents() override;

    virtual int checkParser() override;
    AtParser* atParser() override;

private:
    RecursiveMutex mutex_;
    NcpClientConfig conf_;
    volatile NcpState ncpState_;
    volatile NcpState prevNcpState_;
    volatile NcpConnectionState connState_;
    volatile NcpPowerState pwrState_;

    void ncpState(NcpState state);
    void ncpPowerState(NcpPowerState state);
    void connectionState(NcpConnectionState state);

    int rltkOn();
    int rltkOff();
};

inline void RealtekNcpClient::lock() {
    mutex_.lock();
}

inline void RealtekNcpClient::unlock() {
    mutex_.unlock();
}

inline int RealtekNcpClient::ncpId() const {
    return PlatformNCPIdentifier::PLATFORM_NCP_REALTEK_RTL872X;
}

} // particle
