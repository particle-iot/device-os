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

#include <cstdlib>

#include "cellular_ncp_client.h"
#include "platform_ncp.h"

#include "at_parser.h"

#include "spark_wiring_thread.h"
#include "gsm0710muxer/channel_stream.h"
#include "static_recursive_mutex.h"

namespace particle {

class SerialStream;

class SaraNcpClient: public CellularNcpClient {
public:
    SaraNcpClient();
    ~SaraNcpClient();

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

    // Reimplemented from CellularNcpClient
    virtual int connect(const CellularNetworkConfig& conf) override;
    virtual int getCellularGlobalIdentity(CellularGlobalIdentity* cgi) override;
    virtual int getIccid(char* buf, size_t size) override;
    virtual int getImei(char* buf, size_t size) override;
    virtual int getSignalQuality(CellularSignalQuality* qual) override;

private:
    AtParser parser_;
    std::unique_ptr<SerialStream> serial_;
    RecursiveMutex mutex_;
    CellularNcpClientConfig conf_;
    volatile NcpState ncpState_ = NcpState::OFF;
    volatile NcpState prevNcpState_;
    volatile NcpConnectionState connState_ = NcpConnectionState::DISCONNECTED;
    int parserError_ = 0;
    bool ready_ = false;
    gsm0710::Muxer<particle::Stream, StaticRecursiveMutex> muxer_;
    std::unique_ptr<particle::MuxerChannelStream<decltype(muxer_)> > muxerAtStream_;
    CellularNetworkConfig netConf_;
    CellularGlobalIdentity cgi_ = {};

    enum class RegistrationState {
        NotRegistered = 0,
        Registered    = 1,
    };

    RegistrationState creg_ = RegistrationState::NotRegistered;
    RegistrationState cgreg_ = RegistrationState::NotRegistered;
    RegistrationState cereg_ = RegistrationState::NotRegistered;
    system_tick_t regStartTime_;
    system_tick_t regCheckTime_;
    system_tick_t registeredTime_;
    system_tick_t powerOnTime_;
    bool memoryIssuePresent_ = false;

    int queryAndParseAtCops(CellularSignalQuality* qual);
    int initParser(Stream* stream);
    int checkParser();
    int waitReady();
    int initReady();
    int waitAtResponse(unsigned int timeout, unsigned int period = 1000);
    int selectSimCard();
    int checkSimCard();
    int configureApn(const CellularNetworkConfig& conf);
    int registerNet();
    int changeBaudRate(unsigned int baud);
    static int muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
            decltype(muxer_)::ChannelState newState, void* ctx);
    void ncpState(NcpState state);
    void connectionState(NcpConnectionState state);
    void parserError(int error);
    void resetRegistrationState();
    void checkRegistrationState();
    int processEventsImpl();

    int modemInit() const;
    int modemPowerOn() const;
    int modemPowerOff();
    int modemHardReset(bool powerOff = false);
    bool modemPowerState() const;
    int modemSetUartState(bool state) const;
    void waitForPowerOff();
};

inline AtParser* SaraNcpClient::atParser() {
    return &parser_;
}

inline void SaraNcpClient::lock() {
    mutex_.lock();
}

inline void SaraNcpClient::unlock() {
    mutex_.unlock();
}

inline void SaraNcpClient::parserError(int error) {
    parserError_ = error;
}

} // particle
