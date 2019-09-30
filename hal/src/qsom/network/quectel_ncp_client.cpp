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

#define NO_STATIC_ASSERT
#include "quectel_ncp_client.h"

#include "at_command.h"
#include "at_response.h"
#include "network_config_db.h"

#include "serial_stream.h"
#include "check.h"
#include "scope_guard.h"
#include "pinmap_hal.h"

#include "gpio_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "core_hal.h"

#include "stream_util.h"

#include "spark_wiring_interrupts.h"
#include "spark_wiring_vector.h"

#include <algorithm>

#define CHECK_PARSER(_expr) \
        ({ \
            const auto _r = _expr; \
            if (_r < 0) { \
                this->parserError(_r); \
                return _r; \
            } \
            _r; \
        })

#define CHECK_PARSER_OK(_expr) \
        do { \
            const auto _r = _expr; \
            if (_r < 0) { \
                this->parserError(_r); \
                return _r; \
            } \
            if (_r != ::particle::AtResponse::OK) { \
                return SYSTEM_ERROR_AT_NOT_OK; \
            } \
        } while (false)

#define CHECK_PARSER_URC(_expr) \
        ({ \
            const auto _r = _expr; \
            if (_r < 0) { \
                self->parserError(_r); \
                return _r; \
            } \
            _r; \
        })

namespace particle {

namespace {

inline system_tick_t millis() {
    return HAL_Timer_Get_Milli_Seconds();
}

const auto QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE = 115200;
const auto QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE = 460800;  // BG96 and EG91 support up to 460800bit/s

const auto QUECTEL_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto QUECTEL_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto QUECTEL_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto QUECTEL_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto QUECTEL_NCP_AT_CHANNEL = 1;
const auto QUECTEL_NCP_PPP_CHANNEL = 2;

const auto QUECTEL_NCP_SIM_SELECT_PIN = 23;

const unsigned REGISTRATION_CHECK_INTERVAL = 15 * 1000;
const unsigned REGISTRATION_TIMEOUT = 5 * 60 * 1000;

} // namespace

QuectelNcpClient::QuectelNcpClient() {}

QuectelNcpClient::~QuectelNcpClient() {
    destroy();
}

int QuectelNcpClient::init(const NcpClientConfig& conf) {
    modemInit();
    conf_ = static_cast<const CellularNcpClientConfig&>(conf);

    // Initialize serial stream
    auto sconf = SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS;

    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_USART_SERIAL2, QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE, sconf));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);

    // Initialize muxed channel stream
    decltype(muxerAtStream_) muxStrm(new (std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, QUECTEL_NCP_AT_CHANNEL));
    CHECK_TRUE(muxStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxStrm->init(QUECTEL_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    CHECK(initParser(serial.get()));
    serial_ = std::move(serial);
    muxerAtStream_ = std::move(muxStrm);
    ncpState_ = NcpState::OFF;
    prevNcpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    regStartTime_ = 0;
    regCheckTime_ = 0;
    parserError_ = 0;
    ready_ = false;
    resetRegistrationState();
    return 0;
}

void QuectelNcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        modemPowerOff();
    }
    parser_.destroy();
    muxerAtStream_.reset();
    serial_.reset();
}

int QuectelNcpClient::initParser(Stream* stream) {
    // Initialize AT parser
    auto parserConf = AtParserConfig().stream(stream).commandTerminator(AtCommandTerminator::CRLF);
    parser_.destroy();
    CHECK(parser_.init(std::move(parserConf)));
    CHECK(parser_.addUrcHandler("+CREG",
                                [](AtResponseReader* reader, const char* prefix, void* data) -> int {
                                    const auto self = (QuectelNcpClient*)data;
                                    int val[2];
                                    int r = CHECK_PARSER_URC(reader->scanf("+CREG: %d,%d", &val[0], &val[1]));
                                    CHECK_TRUE(r >= 1, SYSTEM_ERROR_UNKNOWN);
                                    // Home network or roaming
                                    if (val[r - 1] == 1 || val[r - 1] == 5) {
                                        self->creg_ = RegistrationState::Registered;
                                    } else {
                                        self->creg_ = RegistrationState::NotRegistered;
                                    }
                                    self->checkRegistrationState();
                                    return 0;
                                },
                                this));
    CHECK(parser_.addUrcHandler("+CGREG",
                                [](AtResponseReader* reader, const char* prefix, void* data) -> int {
                                    const auto self = (QuectelNcpClient*)data;
                                    int val[2];
                                    int r = CHECK_PARSER_URC(reader->scanf("+CGREG: %d,%d", &val[0], &val[1]));
                                    CHECK_TRUE(r >= 1, SYSTEM_ERROR_UNKNOWN);
                                    // Home network or roaming
                                    if (val[r - 1] == 1 || val[r - 1] == 5) {
                                        self->cgreg_ = RegistrationState::Registered;
                                    } else {
                                        self->cgreg_ = RegistrationState::NotRegistered;
                                    }
                                    self->checkRegistrationState();
                                    return 0;
                                },
                                this));
    CHECK(parser_.addUrcHandler("+CEREG",
                                [](AtResponseReader* reader, const char* prefix, void* data) -> int {
                                    const auto self = (QuectelNcpClient*)data;
                                    int val[2];
                                    int r = CHECK_PARSER_URC(reader->scanf("+CEREG: %d,%d", &val[0], &val[1]));
                                    CHECK_TRUE(r >= 1, SYSTEM_ERROR_UNKNOWN);
                                    // Home network or roaming
                                    if (val[r - 1] == 1 || val[r - 1] == 5) {
                                        self->cereg_ = RegistrationState::Registered;
                                    } else {
                                        self->cereg_ = RegistrationState::NotRegistered;
                                    }
                                    self->checkRegistrationState();
                                    return 0;
                                },
                                this));
    return 0;
}

int QuectelNcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    // Power on the modem
    CHECK(modemPowerOn());
    CHECK(waitReady());
    return 0;
}

int QuectelNcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    muxer_.stop();
    // Disable voltage translator
    modemSetUartState(false);
    // Power down
    modemPowerOff();
    ready_ = false;
    ncpState(NcpState::OFF);
    return 0;
}

int QuectelNcpClient::enable() {
    const NcpClientLock lock(this);
    if (ncpState_ != NcpState::DISABLED) {
        return 0;
    }
    serial_->enabled(true);
    muxerAtStream_->enabled(true);
    ncpState_ = prevNcpState_;
    off();
    return 0;
}

void QuectelNcpClient::disable() {
    // This method is used to unblock the network interface thread, so we're not trying to acquire
    // the client lock here
    const NcpState state = ncpState_;
    if (state == NcpState::DISABLED) {
        return;
    }
    prevNcpState_ = state;
    ncpState_ = NcpState::DISABLED;
    serial_->enabled(false);
    muxerAtStream_->enabled(false);
}

NcpState QuectelNcpClient::ncpState() {
    return ncpState_;
}

int QuectelNcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return 0;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    (void)r;
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    resetRegistrationState();

    connectionState(NcpConnectionState::DISCONNECTED);
    return 0;
}

NcpConnectionState QuectelNcpClient::connectionState() {
    return connState_;
}

int QuectelNcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGMR");
    CHECK_PARSER(resp.readLine(buf, size));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int QuectelNcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int QuectelNcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int QuectelNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    return muxer_.writeChannel(QUECTEL_NCP_PPP_CHANNEL, data, size);
}

void QuectelNcpClient::processEvents() {
    const NcpClientLock lock(this);
    processEventsImpl();
}

int QuectelNcpClient::ncpId() const {
    return conf_.ncpIdentifier();
}

int QuectelNcpClient::connect(const CellularNetworkConfig& conf) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ == NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK(checkParser());

    resetRegistrationState();
    CHECK(configureApn(conf));
    CHECK(registerNet());

    checkRegistrationState();

    return 0;
}

int QuectelNcpClient::getIccid(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());

    auto resp = parser_.sendCommand("AT+CCID");
    char iccid[32] = {};
    int r = CHECK_PARSER(resp.scanf("+CCID: %31s", iccid));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    size_t n = std::min(strlen(iccid), size);
    memcpy(buf, iccid, n);
    if (size > 0) {
        if (n == size) {
            --n;
        }
        buf[n] = '\0';
    }
    return n;
}

int QuectelNcpClient::getImei(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGSN");
    const size_t n = CHECK_PARSER(resp.readLine(buf, size));
    CHECK_PARSER_OK(resp.readResult());
    return n;
}

int QuectelNcpClient::queryAndParseAtCops(CellularSignalQuality* qual) {
    int act;
    char mobileCountryCode[4] = {0};
    char mobileNetworkCode[4] = {0};

    // Reformat the operator string to be numeric
    // (allows the capture of `mcc` and `mnc`)
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=3,2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    auto resp = parser_.sendCommand("AT+COPS?");
    r = CHECK_PARSER(resp.scanf("+COPS: %*d,%*d,\"%3[0-9]%3[0-9]\",%d", mobileCountryCode,
                                    mobileNetworkCode, &act));
    CHECK_TRUE(r == 3, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    // `atoi` returns zero on error, which is an invalid `mcc` and `mnc`
    cgi_.mobile_country_code = static_cast<uint16_t>(::atoi(mobileCountryCode));
    cgi_.mobile_network_code = static_cast<uint16_t>(::atoi(mobileNetworkCode));

    switch (static_cast<CellularAccessTechnology>(act)) {
        case CellularAccessTechnology::NONE:
        case CellularAccessTechnology::GSM:
        case CellularAccessTechnology::GSM_COMPACT:
        case CellularAccessTechnology::UTRAN:
        case CellularAccessTechnology::GSM_EDGE:
        case CellularAccessTechnology::UTRAN_HSDPA:
        case CellularAccessTechnology::UTRAN_HSUPA:
        case CellularAccessTechnology::UTRAN_HSDPA_HSUPA:
        case CellularAccessTechnology::LTE:
        case CellularAccessTechnology::EC_GSM_IOT:
        case CellularAccessTechnology::E_UTRAN: {
            break;
        }
        default: {
            return SYSTEM_ERROR_BAD_DATA;
        }
    }
    if (qual) {
        qual->accessTechnology(static_cast<CellularAccessTechnology>(act));
    }

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getCellularGlobalIdentity(CellularGlobalIdentity* cgi) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ != NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(cgi, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(checkParser());
    CHECK(queryAndParseAtCops(nullptr));

    *cgi = cgi_;

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getSignalQuality(CellularSignalQuality* qual) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ != NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(qual, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(checkParser());

    {
        int act;
        int v;
        auto resp = parser_.sendCommand("AT+COPS?");
        int r = CHECK_PARSER(resp.scanf("+COPS: %d,%*d,\"%*[^\"]\",%d", &v, &act));
        CHECK_TRUE(r == 2, SYSTEM_ERROR_UNKNOWN);
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

        switch (static_cast<CellularAccessTechnology>(act)) {
            case CellularAccessTechnology::NONE:
            case CellularAccessTechnology::GSM:
            case CellularAccessTechnology::GSM_COMPACT:
            case CellularAccessTechnology::UTRAN:
            case CellularAccessTechnology::GSM_EDGE:
            case CellularAccessTechnology::UTRAN_HSDPA:
            case CellularAccessTechnology::UTRAN_HSUPA:
            case CellularAccessTechnology::UTRAN_HSDPA_HSUPA:
            case CellularAccessTechnology::LTE:
            case CellularAccessTechnology::EC_GSM_IOT:
            case CellularAccessTechnology::E_UTRAN: {
                break;
            }
            default: {
                return SYSTEM_ERROR_BAD_DATA;
            }
        }
        qual->accessTechnology(static_cast<CellularAccessTechnology>(act));
    }

    int rxlev, rxqual;
    auto resp = parser_.sendCommand("AT+CSQ");
    int r = CHECK_PARSER(resp.scanf("+CSQ: %d,%d", &rxlev, &rxqual));
    CHECK_TRUE(r == 2, SYSTEM_ERROR_BAD_DATA);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Fixup values
    switch (qual->strengthUnits()) {
        case CellularStrengthUnits::RXLEV: {
            qual->strength((rxlev != 99) ? (2 * rxlev) : rxlev);
            break;
        }
        case CellularStrengthUnits::RSCP: {
            qual->strength((rxlev != 99) ? (3 + 2 * rxlev) : 255);
            break;
        }
        case CellularStrengthUnits::RSRP: {
            qual->strength((rxlev != 99) ? (rxlev * 97) / 31 : 255);
            break;
        }
        default: {
            // Do nothing
            break;
        }
    }

    if (qual->accessTechnology() == CellularAccessTechnology::GSM_EDGE) {
        qual->qualityUnits(CellularQualityUnits::MEAN_BEP);
    }

    switch (qual->qualityUnits()) {
    case CellularQualityUnits::RXQUAL:
    case CellularQualityUnits::MEAN_BEP: {
        qual->quality(rxqual);
        break;
    }
    case CellularQualityUnits::ECN0: {
        qual->quality((rxqual != 99) ? std::min((7 + (7 - rxqual) * 6), 44) : 255);
        break;
    }
    case CellularQualityUnits::RSRQ: {
        qual->quality((rxqual != 99) ? (rxqual * 34) / 7 : 255);
        break;
    }
    default: {
        // Do nothing
        break;
    }
    }
    // }

    return 0;
}

int QuectelNcpClient::checkParser() {
    if (ncpState_ != NcpState::ON) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ready_ && parserError_ != 0) {
        const int r = parser_.execCommand(1000, "AT");
        if (r == AtResponse::OK) {
            parserError_ = 0;
        } else {
            ready_ = false;
        }
    }
    CHECK(waitReady());
    return 0;
}

int QuectelNcpClient::waitAtResponse(unsigned int timeout, unsigned int period) {
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(period, "AT");
        if (r < 0 && r != SYSTEM_ERROR_TIMEOUT) {
            return r;
        }
        if (r == AtResponse::OK) {
            return 0;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= timeout) {
            break;
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

int QuectelNcpClient::waitReady() {
    if (ready_) {
        return 0;
    }
    muxer_.stop();
    CHECK(serial_->setBaudRate(QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
    // Enable voltage translator
    CHECK(modemSetUartState(true));
    skipAll(serial_.get(), 1000);
    parser_.reset();
    ready_ = waitAtResponse(20000) == 0;

    if (ready_) {
        skipAll(serial_.get(), 1000);
        parser_.reset();
        parserError_ = 0;
        LOG(TRACE, "NCP ready to accept AT commands");

        auto r = initReady();
        if (r != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to perform early initialization");
            ready_ = false;
        }
    } else {
        LOG(ERROR, "No response from NCP");
    }

    if (!ready_) {
        // Disable voltage translator
        modemSetUartState(false);
        // Hard reset the modem
        modemHardReset(true);
        ncpState(NcpState::OFF);

        return SYSTEM_ERROR_INVALID_STATE;
    }

    return 0;
}

int QuectelNcpClient::selectSimCard() {
    // Auto detect SIM card
    int r = CHECK_PARSER(parser_.execCommand("AT+QDSIM?"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Set modem full functionality
    r = CHECK_PARSER(parser_.execCommand("AT+CFUN=1,0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Using numeric CME ERROR codes
    // int r = CHECK_PARSER(parser_.execCommand("AT+CMEE=1"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    int simState = 0;
    for (unsigned i = 0; i < 10; ++i) {
        simState = checkSimCard();
        if (!simState) {
            break;
        }
        HAL_Delay_Milliseconds(1000);
    }
    return simState;
}

int QuectelNcpClient::changeBaudRate(unsigned int baud) {
    auto resp = parser_.sendCommand("AT+IPR=%u", baud);
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return serial_->setBaudRate(baud);
}

int QuectelNcpClient::initReady() {
    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard());

    // Just in case disconnect
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Change the baudrate to 921600
    CHECK(changeBaudRate(QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE));
    // Check that the modem is responsive at the new baudrate
    skipAll(serial_.get(), 1000);
    CHECK(waitAtResponse(10000));

    // Send AT+CMUX and initialize multiplexer
    r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,,1509,,,,,"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Initialize muxer
    muxer_.setStream(serial_.get());
    muxer_.setMaxFrameSize(QUECTEL_NCP_MAX_MUXER_FRAME_SIZE);
    muxer_.setKeepAlivePeriod(QUECTEL_NCP_KEEPALIVE_PERIOD * 2);
    muxer_.setKeepAliveMaxMissed(QUECTEL_NCP_KEEPALIVE_MAX_MISSED);
    muxer_.useMscAsKeepAlive(true);
    muxer_.setMaxRetransmissions(3);
    muxer_.setAckTimeout(2530);
    muxer_.setControlResponseTimeout(2540);

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    NAMED_SCOPE_GUARD(muxerSg, { muxer_.stop(); });

    // Start muxer (blocking call)
    CHECK_TRUE(muxer_.start(true) == 0, SYSTEM_ERROR_UNKNOWN);

    // Open AT channel and connect it to AT channel stream
    if (muxer_.openChannel(QUECTEL_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
        // Failed to open AT channel
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Just in case resume AT channel
    muxer_.resumeChannel(QUECTEL_NCP_AT_CHANNEL);

    // Reinitialize parser with a muxer-based stream
    CHECK(initParser(muxerAtStream_.get()));
    CHECK(waitAtResponse(20000, 5000));

    ncpState(NcpState::ON);
    LOG_DEBUG(TRACE, "Muxer AT channel live");

    muxerSg.dismiss();

    return 0;
}

int QuectelNcpClient::checkSimCard() {
    auto resp = parser_.sendCommand("AT+CPIN?");
    char code[33] = {};
    int r = CHECK_PARSER(resp.scanf("+CPIN: %32[^\n]", code));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    if (!strcmp(code, "READY")) {
        r = parser_.execCommand("AT+CCID");
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        return 0;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int QuectelNcpClient::configureApn(const CellularNetworkConfig& conf) {
    netConf_ = conf;
    if (!netConf_.isValid()) {
        // Look for network settings based on IMSI
        char buf[32] = {};
        auto resp = parser_.sendCommand("AT+CIMI");
        CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
        const int r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        netConf_ = networkConfigForImsi(buf, strlen(buf));
    }
    // FIXME: for now IPv4 context only
    auto resp = parser_.sendCommand("AT+CGDCONT=1,\"IP\",\"%s%s\"", 
            (netConf_.hasUser() && netConf_.hasPassword()) ? "CHAP:" : "", 
            netConf_.hasApn() ? netConf_.apn() : "");
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int QuectelNcpClient::registerNet() {
    int r = 0;

    // Register GPRS, LET, NB-IOT network
    r = CHECK_PARSER(parser_.execCommand("AT+CREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(parser_.execCommand("AT+CGREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(parser_.execCommand("AT+CEREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    connectionState(NcpConnectionState::CONNECTING);

    // NOTE: up to 3 mins
    r = CHECK_PARSER(parser_.execCommand(3 * 60 * 1000, "AT+COPS=0"));
    // Ignore response code here
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
    r = CHECK_PARSER(parser_.execCommand("AT+CREG?"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(parser_.execCommand("AT+CGREG?"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    // } else {
    r = CHECK_PARSER(parser_.execCommand("AT+CEREG?"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    // }

    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;

    return 0;
}

void QuectelNcpClient::ncpState(NcpState state) {
    if (ncpState_ == NcpState::DISABLED) {
        return;
    }
    if (state == NcpState::OFF) {
        ready_ = false;
        connectionState(NcpConnectionState::DISCONNECTED);
    }

    if (ncpState_ == state) {
        return;
    }
    ncpState_ = state;
    LOG(TRACE, "NCP state changed: %d", (int)ncpState_);

    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpStateChangedEvent event = {};
        event.type = NcpEvent::NCP_STATE_CHANGED;
        event.state = ncpState_;
        handler(event, conf_.eventHandlerData());
    }
}

void QuectelNcpClient::connectionState(NcpConnectionState state) {
    if (ncpState_ == NcpState::DISABLED) {
        return;
    }
    if (connState_ == state) {
        return;
    }
    LOG(TRACE, "NCP connection state changed: %d", (int)state);
    connState_ = state;

    if (connState_ == NcpConnectionState::CONNECTED) {
        // Open data channel
        int r = muxer_.openChannel(QUECTEL_NCP_PPP_CHANNEL,
                                   [](const uint8_t* data, size_t size, void* ctx) -> int {
                                       auto self = (QuectelNcpClient*)ctx;
                                       const auto handler = self->conf_.dataHandler();
                                       if (handler) {
                                           handler(0, data, size, self->conf_.dataHandlerData());
                                       }
                                       return 0;
                                   },
                                   this);
        if (r) {
            connState_ = NcpConnectionState::DISCONNECTED;
        }
    }

    const auto handler = conf_.eventHandler();
    if (handler) {
        if (state == NcpConnectionState::CONNECTED) {
            CellularNcpAuthEvent event = {};
            event.type = CellularNcpEvent::AUTH;
            event.user = netConf_.user();
            event.password = netConf_.password();
            handler(event, conf_.eventHandlerData());
        }
        NcpConnectionStateChangedEvent event = {};
        event.type = NcpEvent::CONNECTION_STATE_CHANGED;
        event.state = connState_;
        handler(event, conf_.eventHandlerData());
    }
}

int QuectelNcpClient::muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState, decltype(muxer_)::ChannelState newState, void* ctx) {
    auto self = (QuectelNcpClient*)ctx;
    // This callback is executed from the multiplexer thread, not safe to use the lock here
    // because it might get called while blocked inside some muxer function

    // We are only interested in Closed state
    if (newState == decltype(muxer_)::ChannelState::Closed) {
        switch (channel) {
        case 0: {
            // Muxer stopped
            self->disable();
            self->connState_ = NcpConnectionState::DISCONNECTED;
            break;
        }
        case QUECTEL_NCP_PPP_CHANNEL: {
            // PPP channel closed
            if (self->connState_ != NcpConnectionState::DISCONNECTED) {
                self->connState_ = NcpConnectionState::CONNECTING;
            }
            break;
        }
        }
    }

    return 0;
}

void QuectelNcpClient::resetRegistrationState() {
    creg_ = RegistrationState::NotRegistered;
    cgreg_ = RegistrationState::NotRegistered;
    cereg_ = RegistrationState::NotRegistered;
    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;
}

void QuectelNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if ((creg_ == RegistrationState::Registered && cgreg_ == RegistrationState::Registered) || cereg_ == RegistrationState::Registered) {
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            connectionState(NcpConnectionState::CONNECTING);
            regStartTime_ = millis();
            regCheckTime_ = regStartTime_;
        }
    }
}

int QuectelNcpClient::processEventsImpl() {
    CHECK_TRUE(ncpState_ == NcpState::ON, SYSTEM_ERROR_INVALID_STATE);
    parser_.processUrc(); // Ignore errors
    checkRegistrationState();
    if (connState_ != NcpConnectionState::CONNECTING || millis() - regCheckTime_ < REGISTRATION_CHECK_INTERVAL) {
        return 0;
    }
    SCOPE_GUARD({ regCheckTime_ = millis(); });

    // Check GPRS, LET, NB-IOT network registration status
    CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
    CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));

    if (connState_ == NcpConnectionState::CONNECTING && millis() - regStartTime_ >= REGISTRATION_TIMEOUT) {
        LOG(WARN, "Resetting the modem due to the network registration timeout");
        muxer_.stop();
        modemHardReset();
        ncpState(NcpState::OFF);
    }
    return 0;
}

int QuectelNcpClient::modemInit() const {
    hal_gpio_config_t conf = {.size = sizeof(conf), .version = 0, .mode = OUTPUT, .set_value = false, .value = 0};

    // Configure PWR_ON and RESET_N pins as Open-Drain and set to high by default
    CHECK(HAL_Pin_Configure(BGPWR, &conf));
    CHECK(HAL_Pin_Configure(BGRST, &conf));
    CHECK(HAL_Pin_Configure(BGDTR, &conf)); // Set DTR=0 to wake up modem

    // BGDTR=1: normal mode, BGDTR=0: sleep mode
    conf.value = 1;
    CHECK(HAL_Pin_Configure(BGDTR, &conf));

    // Configure VINT as Input for modem power state monitoring
    conf.mode = INPUT_PULLUP;
    CHECK(HAL_Pin_Configure(BGVINT, &conf));

    LOG(TRACE, "Modem low level initialization OK");

    return 0;
}

int QuectelNcpClient::modemPowerOn() const {
    if (!modemPowerState()) {
        LOG(TRACE, "Powering modem on");
        // Power on, power on pulse >= 500ms
        HAL_GPIO_Write(BGPWR, 1);
        HAL_Delay_Milliseconds(500);
        HAL_GPIO_Write(BGPWR, 0);

        bool powerGood;
        // After power on the device, we can't assume the device is ready for operation:
        // BG96: status pin ready requires >= 4.8s, uart ready requires >= 4.9s
        // EG91: status pin ready requires >= 10s, uart ready requires >= 12s
        for (unsigned i = 0; i < 100; i++) {
            powerGood = modemPowerState();
            if (powerGood) {
                break;
            }
            HAL_Delay_Milliseconds(150);
        }
        if (powerGood) {
            LOG(TRACE, "Modem powered on");
        } else {
            LOG(ERROR, "Failed to power on modem");
        }
    } else {
        LOG(TRACE, "Modem already on");
    }
    CHECK_TRUE(modemPowerState(), SYSTEM_ERROR_INVALID_STATE);

    return 0;
}

int QuectelNcpClient::modemPowerOff() const {
    if (modemPowerState()) {
        LOG(TRACE, "Powering modem off");
        // Important! We need to disable voltage translator here
        // otherwise V_INT will never go low
        modemSetUartState(false);

        // Power off, power off pulse >= 650ms
        HAL_GPIO_Write(BGPWR, 1);
        HAL_Delay_Milliseconds(650);
        HAL_GPIO_Write(BGPWR, 0);

        bool powerGood;
        // Verify that the module was powered down by checking the status pin (BGVINT)
        // BG96: >=2s
        // EG91: >=30s
        for (unsigned i = 0; i < 100; i++) {
            powerGood = modemPowerState();
            if (!powerGood) {
                break;
            }
            HAL_Delay_Milliseconds(300);
        }
        if (!powerGood) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem, try hard reset");
            modemHardReset(true);
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return 0;
}

int QuectelNcpClient::modemHardReset(bool powerOff) const {
    LOG(TRACE, "Hard resetting the modem");

    // BG96 reset, 150ms <= reset pulse <= 460ms
    HAL_GPIO_Write(BGRST, 1);
    HAL_Delay_Milliseconds(300);
    HAL_GPIO_Write(BGRST, 0);

    return 0;
}

bool QuectelNcpClient::modemPowerState() const {
    // LOG(TRACE, "BGVINT: %d", HAL_GPIO_Read(BGVINT));
    return !HAL_GPIO_Read(BGVINT);
}

int QuectelNcpClient::modemSetUartState(bool state) const {
    // Use BG96 status pin to enable/disable voltage convert IC Automatically
    return 0;
}

} // namespace particle
