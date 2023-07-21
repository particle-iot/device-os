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

#include "network/ncp/cellular/cellular_ncp_client.h"
#include "platform_ncp.h"

#include "at_parser.h"

#include "spark_wiring_thread.h"
#include "gsm0710muxer/channel_stream.h"
#include "static_recursive_mutex.h"
#include "serial_stream.h"
#include "cellular_reg_status.h"

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
    NcpPowerState ncpPowerState() override;
    int disconnect() override;
    NcpConnectionState connectionState() override;
    int getFirmwareVersionString(char* buf, size_t size) override;
    int getFirmwareModuleVersion(uint16_t* ver) override;
    int updateFirmware(InputStream* file, size_t size) override;
    int dataChannelWrite(int id, const uint8_t* data, size_t size) override;
    int dataChannelFlowControl(bool state) override;
    void processEvents() override;
    int checkParser() override;
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
    virtual int setRegistrationTimeout(unsigned timeout) override;
    virtual int getTxDelayInDataChannel() override;
    virtual int enterDataMode() override;
    virtual int getMtu() override;
    virtual int urcs(bool enable) override;
    virtual int startNcpFwUpdate(bool update) override;
    virtual int dataModeError(int error) override;

private:
    AtParser parser_;
    AtParser dataParser_;
    std::unique_ptr<SerialStream> serial_;
    RecursiveMutex mutex_;
    CellularNcpClientConfig conf_;
    volatile NcpState ncpState_ = NcpState::OFF;
    volatile NcpState prevNcpState_;
    volatile NcpConnectionState connState_ = NcpConnectionState::DISCONNECTED;
    volatile NcpPowerState pwrState_ = NcpPowerState::UNKNOWN;
    int parserError_ = 0;
    bool ready_ = false;
    gsm0710::Muxer<EventGroupBasedStream, StaticRecursiveMutex> muxer_;
    std::unique_ptr<particle::MuxerChannelStream<decltype(muxer_)> > muxerAtStream_;
    std::unique_ptr<particle::MuxerChannelStream<decltype(muxer_)> > muxerDataStream_;
    CellularNetworkConfig netConf_;
    CellularGlobalIdentity cgi_ = {};
    CellularAccessTechnology act_ = CellularAccessTechnology::NONE;

    enum class ModemState {
        Unknown = 0,
        MuxerAtChannel = 1,
        RuntimeBaudrate = 2,
        DefaultBaudrate = 3
    };

    CellularRegistrationStatus csd_;
    CellularRegistrationStatus psd_;
    CellularRegistrationStatus eps_;

    system_tick_t regStartTime_;
    system_tick_t regCheckTime_;
    system_tick_t imsiCheckTime_;
    system_tick_t registeredTime_;
    system_tick_t powerOnTime_;
    unsigned int fwVersion_ = 0;
    bool memoryIssuePresent_;
    bool oldFirmwarePresent_;
    unsigned registrationTimeout_;
    unsigned registrationInterventions_;
    volatile bool inFlowControl_ = false;
    bool firmwareUpdateR510_ = false;
    int firmwareInstallRespCodeR510_ = 0;
    int lastFirmwareInstallRespCodeR510_ = 0;
    int waitReadyRetries_ = 0;
    bool sleepNoPPPWrite_ = false;

    system_tick_t lastWindow_ = 0;
    size_t bytesInWindow_ = 0;

    bool cgattWorkaroundApplied_ = false;

    int queryAndParseAtCops(CellularSignalQuality* qual);
    int initParser(Stream* stream);
    int waitReady(bool powerOn = false);
    int initReady(ModemState state);
    int checkRuntimeState(ModemState& state);
    bool checkRuntimeStateMuxer(unsigned baudrate);
    int initMuxer();
    int waitAtResponse(unsigned int timeout, unsigned int period = 1000);
    int waitAtResponse(AtParser& parser, unsigned int timeout, unsigned int period = 1000);
    int selectSimCard(ModemState& state);
    int selectNetworkProf(ModemState& state);
    int checkSimCard(bool* failure = nullptr);
    int configureApn(const CellularNetworkConfig& conf);
    int registerNet();
    int changeBaudRate(unsigned int baud);
    static int muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
            decltype(muxer_)::ChannelState newState, void* ctx);
    void ncpState(NcpState state);
    void ncpPowerState(NcpPowerState state);
    void connectionState(NcpConnectionState state);
    void parserError(int error);
    void resetRegistrationState();
    void checkRegistrationState();
    int interveneRegistration();
    int checkRunningImsi();
    int processEventsImpl();
    int getIccidImpl(char* buf, size_t size);
    int checkNetConfForImsi();

    int modemInit() const;
    bool waitModemPowerState(bool onOff, system_tick_t timeout);
    int modemPowerOn();
    int modemPowerOff();
    int modemSoftReset();
    int modemSoftPowerOff();
    int modemHardReset(bool powerOff = false);
    int modemEmergencyHardReset();
    bool modemPowerState() const;
    int modemSetUartState(bool state) const;
    void waitForPowerOff();
    int getAppFirmwareVersion();
    int waitAtResponseFromPowerOn(ModemState& modemState);
    int disablePsmEdrx();
    int checkSimReadiness(bool checkForRfReset = false);
    int getPowerSavingValue();
    int setPowerSavingValue(CellularPowerSavingValue upsv, bool check = false);
    int getOperationModeCached(CellularOperationMode& cemode);
    int setOperationModeCached(CellularOperationMode cemode);
    int getOperationMode();
    int setOperationMode(CellularOperationMode cemode, bool check = false, bool save = false);
    int setModuleFunctionality(CellularFunctionality cfun, bool check = false);
    int getModuleFunctionality();
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
