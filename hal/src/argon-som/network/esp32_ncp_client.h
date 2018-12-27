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
#include "platform_ncp.h"
#include "spark_wiring_thread.h"
#include <memory>
#include "static_recursive_mutex.h"
#include "gsm0710muxer/muxer.h"
#include "gsm0710muxer/channel_stream.h"

namespace particle {

class SerialStream;

class Esp32NcpClient: public WifiNcpClient {
public:
    Esp32NcpClient();
    ~Esp32NcpClient();

    // Reimplemented from NcpClient
    int init(const NcpClientConfig& conf) override;
    void destroy() override;
    int on() override;
    int off() override;
    int enable() override;
    void disable() override;
    NcpState ncpState() override;
    int disconnect() override;
    NcpConnectionState connectionState() override;
    int getFirmwareVersionString(char* buf, size_t size) override;
    int getFirmwareModuleVersion(uint16_t* ver) override;
    int updateFirmware(InputStream* file, size_t size) override;
    int dataChannelWrite(int id, const uint8_t* data, size_t size) override;
    void processEvents() override;
    AtParser* atParser() override;
    void lock() override;
    void unlock() override;
    int ncpId() const override;

    // Reimplemented from WifiNcpClient
    int connect(const char* ssid, const MacAddress& bssid, WifiSecurity sec, const WifiCredentials& cred) override;
    int getNetworkInfo(WifiNetworkInfo* info) override;
    int scan(WifiScanCallback callback, void* data) override;
    int getMacAddress(MacAddress* addr) override;

private:
    AtParser parser_;
    std::unique_ptr<SerialStream> serial_;
    RecursiveMutex mutex_;
    NcpClientConfig conf_;
    volatile NcpState ncpState_;
    volatile NcpState prevNcpState_;
    volatile NcpConnectionState connState_;
    int parserError_;
    bool ready_;
    gsm0710::Muxer<particle::Stream, StaticRecursiveMutex> muxer_;
    std::unique_ptr<particle::MuxerChannelStream<decltype(muxer_)> > muxerAtStream_;
    bool muxerNotStarted_;

    int initParser(Stream* stream);
    int checkParser();
    int waitReady();
    int initReady();
    int initMuxer();
    static int muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
            decltype(muxer_)::ChannelState newState, void* ctx);
    void ncpState(NcpState state);
    void connectionState(NcpConnectionState state);
    void parserError(int error);
    int getFirmwareModuleVersionImpl(uint16_t* ver);
};

inline void Esp32NcpClient::lock() {
    mutex_.lock();
}

inline void Esp32NcpClient::unlock() {
    mutex_.unlock();
}

inline int Esp32NcpClient::ncpId() const {
    return MeshNCPIdentifier::MESH_NCP_ESP32;
}

inline void Esp32NcpClient::parserError(int error) {
    parserError_ = error;
}

} // particle
