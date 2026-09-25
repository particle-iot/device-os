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

#include <cstdlib>

#include "hal_platform.h"

#include "network/ncp/cellular/cellular_ncp_client.h"
#include "network/ncp/cellular/cellular_registration_backoff.h"
#include "platform_ncp.h"

#include "at_parser.h"

#include "spark_wiring_thread.h"
#include "gsm0710muxer/channel_stream.h"
#include "static_recursive_mutex.h"
#include "serial_stream.h"
#include "cellular_reg_status.h"
#include "spark_wiring_vector.h"
#include "../../../../system/src/util/system_timer.h" // FIXME

namespace particle {

class SerialStream;

class QuectelNcpClient: public CellularNcpClient {
public:
    QuectelNcpClient();
    ~QuectelNcpClient();

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
    virtual int resetRegistrationBackoff() override;
    virtual int getRegistrationBackoffState(unsigned* stage, system_tick_t* cooldownRemaining,
            bool* inCooldown) override;
    virtual int setRegistrationBackoffSchedule(const CellularRegistrationBackoff::Config& conf) override;
    virtual int getTxDelayInDataChannel() override;
    virtual int enterDataMode() override;
    virtual int getMtu() override;
    virtual int urcs(bool enable) override;
    virtual int startNcpFwUpdate(bool update) override;
    virtual int dataModeError(int error) override;
    virtual int sendApdu(const char* cmd, size_t cmdSize, char* resp, size_t& respSize, bool autoClose) override;
    int sendApduImpl(const char* cmd, size_t cmdSize, char* resp, size_t& respSize, bool autoClose,
            bool checkState);

    auto getMuxer() {
        return &muxer_;
    }

private:
    AtParser parser_;
    AtParser dataParser_;
    std::unique_ptr<SerialStream> serial_;
    RecursiveMutex mutex_;
    bool apduSkipStateCheck_ = false; // Set only while the init time eSIM probe runs
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

    enum class ModemPowerReason {
        Unknown = 0,
        ModemOff = 1,
        AtUnresponsive = 2,
        RegTimeout = 3
    };

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
    system_tick_t policymanSrvModeCheckTime_;
    unsigned registrationTimeout_;
    unsigned registrationInterventions_;
    CellularRegistrationBackoff backoff_;
    volatile bool inFlowControl_ = false;
    bool checkImsi_ = false;
    unsigned int fwVersion_ = 0;
    system::SystemTimer apduChannelTimer_;
    int apduChannel_ = 0;
    volatile system_tick_t simSettleUntil_ = 0;
    bool apduRaisedCfun_ = false;
    bool apduCfunHold_ = false;
    bool configuredPlmn_ = false;
    system_tick_t atProbeTime_ = 0;
    unsigned atProbeFailStreak_ = 0;
    // Set by urcs(false) when going to sleep. On the muxer path the AT channel is suspended and a
    // probe cannot be answered; on the BG95 QINDCFG path it would be answered but would wake the
    // module, which is the thing sleep is trying to avoid. Suppress the probe either way.
    bool sleepUrcsDisabled_ = false;
    // Set once connect() has been called, cleared whenever we leave IDLE. connectionState() reports
    // DISCONNECTED while IDLE until this is set, otherwise the netif tears the IDLE state down on
    // every tick after Cellular.on(). See esimHasUsableProfile().
    bool connectRequested_ = false;

    int queryAndParseAtCops(CellularSignalQuality* qual);
    int initParser(Stream* stream);
    int waitReady(bool powerOn = false);
    int getAppFirmwareVersion();
    int initReady(ModemState state);
    int checkRuntimeState(ModemState& state);
    int initMuxer();
    int waitAtResponse(unsigned int timeout, unsigned int period = 1000);
    int waitAtResponse(AtParser& parser, unsigned int timeout, unsigned int period = 1000);
    int checkNetConfForImsi();
    int setupBands();
    int parseEfSize(unsigned int fid);
    int readAndClearEfByFid(unsigned int fid);
    int clearAllUserPlmn();
    int syncUserPlmn(const Vector<CString>& envPreferredPlmn);
    int configurePlmn();
    int selectSimCard();
    int checkSimCard();
    int getModuleFunctionality();
    int setModuleFunctionality(CellularFunctionality cfun, bool check = false);
    int getPolicymanServiceMode();
    int setPolicymanServiceMode(CellularPolicymanServiceMode mode, bool check);
    int set2gAttenuation3dB();
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
    void registrationFailed();
    int enterRegistrationBackoff();
    int checkRunningImsi();
    int processEventsImpl();
    void disableImpl();
    int getIccidImpl(char* buf, size_t size);
    bool checkAtWhileConnected();
    // Asks the eUICC whether any profile is enabled. Returns 1 if at least one is, 0 if all of them
    // are disabled (or there are none), and a negative error if we could not find out. Callers must
    // treat an error as "carry on as usual" and never enter IDLE on it. On a return of 1, iccid
    // holds the enabled profile's ICCID.
    int esimHasUsableProfile(char* iccid, size_t iccidSize, bool duringInit = false);
    int esimSyncSimIccid(const char* expected);
    int esimCheckProfiles();
    void publishConnectionState();
    int configModemPowerState(ModemPowerReason reason);

    /** Is this a Quectel Cat-M1 device ? */
    bool isQuecCatM1Device();
    /** Is this a Quectel Cat-1 device ? */
    bool isQuecCat1Device();
    /** Is this a Quectel Cat-NB1/Cat-NB2 device ? */
    bool isQuecCatNBxDevice();
    /** Is this a Quectel BG95* device ? */
    bool isQuecBG95xDevice();
    /** Is this a Quectel device we want to enable 2G and/or 3G on ? */
    bool isQuec2g3gEnabled();
    int getRuntimeBaudrate();
    int modemInit() const;
    bool waitModemPowerState(bool onOff, system_tick_t timeout);
    int modemPowerOn();
    int modemPowerOff();
    int modemSoftPowerOff();
    int modemHardReset(bool powerOff = false);
    bool modemPowerState() const;
    int modemSetUartState(bool state) const;
    uint32_t getDefaultSerialConfig() const;
    void exitDataModeWithDtr() const;
    int closeApduChannel(int channel);
    void apduWaitForSettle();
    int apduRaiseFunctionality();
    void apduRestoreFunctionality();

    static void apduChannelTimeoutCb(void* arg);
};

inline AtParser* QuectelNcpClient::atParser() {
    return &parser_;
}

inline void QuectelNcpClient::lock() {
    mutex_.lock();
}

inline void QuectelNcpClient::unlock() {
    mutex_.unlock();
}

inline void QuectelNcpClient::parserError(int error) {
    parserError_ = error;
}

} // particle
