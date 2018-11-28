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
#include "sara_ncp_client.h"

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

#include <algorithm>
#include "spark_wiring_interrupts.h"

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
                return SYSTEM_ERROR_UNKNOWN; \
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

const auto UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE = 115200;
const auto UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2 = 921600;

const auto UBLOX_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto UBLOX_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto UBLOX_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto UBLOX_NCP_AT_CHANNEL = 1;
const auto UBLOX_NCP_PPP_CHANNEL = 2;

const auto UBLOX_NCP_SIM_SELECT_PIN = 23;

const unsigned REGISTRATION_CHECK_INTERVAL = 15 * 1000;
const unsigned REGISTRATION_TIMEOUT = 5 * 60 * 1000;

} // anonymous

SaraNcpClient::SaraNcpClient() {
}

SaraNcpClient::~SaraNcpClient() {
    destroy();
}

int SaraNcpClient::init(const NcpClientConfig& conf) {
    modemInit();
    conf_ = static_cast<const CellularNcpClientConfig&>(conf);
    // Initialize serial stream
    auto sconf = SERIAL_8N1;
    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        sconf |= SERIAL_FLOW_CONTROL_RTS_CTS;
    } else {
        HAL_Pin_Mode(RTS1, OUTPUT);
        HAL_GPIO_Write(RTS1, 0);
    }
    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_USART_SERIAL2,
            UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE, sconf));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);
    // Initialize muxed channel stream
    decltype(muxerAtStream_) muxStrm(new(std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, UBLOX_NCP_AT_CHANNEL));
    CHECK_TRUE(muxStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxStrm->init(UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
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

void SaraNcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        modemPowerOff();
    }
    parser_.destroy();
    muxerAtStream_.reset();
    serial_.reset();
}

int SaraNcpClient::initParser(Stream* stream) {
    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(stream)
            .commandTerminator(AtCommandTerminator::CRLF);
    parser_.destroy();
    CHECK(parser_.init(std::move(parserConf)));
    CHECK(parser_.addUrcHandler("+CREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
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
    }, this));
    CHECK(parser_.addUrcHandler("+CGREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
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
    }, this));
    CHECK(parser_.addUrcHandler("+CEREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
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
    }, this));
    return 0;
}

int SaraNcpClient::on() {
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

int SaraNcpClient::off() {
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

int SaraNcpClient::enable() {
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

void SaraNcpClient::disable() {
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

NcpState SaraNcpClient::ncpState() {
    return ncpState_;
}

int SaraNcpClient::disconnect() {
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

NcpConnectionState SaraNcpClient::connectionState() {
    return connState_;
}

int SaraNcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGMR");
    CHECK_PARSER(resp.readLine(buf, size));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int SaraNcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraNcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    return muxer_.writeChannel(UBLOX_NCP_PPP_CHANNEL, data, size);
}

void SaraNcpClient::processEvents() {
    const NcpClientLock lock(this);
    processEventsImpl();
}

int SaraNcpClient::ncpId() const {
    return conf_.ncpIdentifier();
}

int SaraNcpClient::connect(const CellularNetworkConfig& conf) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ == NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK(checkParser());

    resetRegistrationState();
    CHECK(configureApn(conf));
    CHECK(registerNet());

    checkRegistrationState();

    return 0;
}

int SaraNcpClient::getIccid(char* buf, size_t size) {
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

int SaraNcpClient::getImei(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGSN");
    const size_t n = CHECK_PARSER(resp.readLine(buf, size));
    CHECK_PARSER_OK(resp.readResult());
    return n;
}

int SaraNcpClient::getSignalQuality(CellularSignalQuality* qual) {
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

    if (ncpId() == MESH_NCP_SARA_R410) {
        int rxlev, rxqual, rscp, ecn0, rsrq, rsrp;
        auto resp = parser_.sendCommand("AT+CESQ");
        int r = CHECK_PARSER(resp.scanf("+CESQ: %d,%d,%d,%d,%d,%d", &rxlev, &rxqual,
                &rscp, &ecn0, &rsrq, &rsrp));
        CHECK_TRUE(r == 6, SYSTEM_ERROR_BAD_DATA);
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

        switch (qual->strengthUnits()) {
            case CellularStrengthUnits::RXLEV: {
                qual->strength(rxlev);
                break;
            }
            case CellularStrengthUnits::RSCP: {
                qual->strength(rscp);
                break;
            }
            case CellularStrengthUnits::RSRP: {
                qual->strength(rsrp);
                break;
            }
            default: {
                // Do nothing
                break;
            }
        }

        switch (qual->qualityUnits()) {
            case CellularQualityUnits::RXQUAL: {
                qual->quality(rxqual);
                break;
            }
            case CellularQualityUnits::ECN0: {
                qual->quality(ecn0);
                break;
            }
            case CellularQualityUnits::RSRQ: {
                qual->quality(rsrq);
                break;
            }
            default: {
                // Do nothing
                break;
            }
        }
    } else {
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
    }

    return 0;
}

int SaraNcpClient::checkParser() {
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

int SaraNcpClient::waitAtResponse(unsigned int timeout, unsigned int period) {
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(period, "AT");
        if (r == AtResponse::OK) {
            return 0;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= timeout) {
            break;
        }
        if (r != SYSTEM_ERROR_TIMEOUT) {
            HAL_Delay_Milliseconds(period);
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

int SaraNcpClient::waitReady() {
    if (ready_) {
        return 0;
    }
    muxer_.stop();
    CHECK(serial_->setBaudRate(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
    // Enable voltage translator
    CHECK(modemSetUartState(true));
    skipAll(serial_.get(), 1000);
    parser_.reset();
    ready_ = waitAtResponse(20000) == 0;
    if (!ready_) {
        LOG(ERROR, "No response from NCP");
        // Disable voltage translator
        modemSetUartState(false);
        // Hard reset the modem
        modemHardReset();
        ncpState(NcpState::OFF);
        return SYSTEM_ERROR_INVALID_STATE;
    }
    skipAll(serial_.get(), 1000);
    parser_.reset();
    parserError_ = 0;
    LOG(TRACE, "NCP ready to accept AT commands");
    return initReady();
}

int SaraNcpClient::selectSimCard() {
    // Read current GPIO configuration
    int mode = -1;
    int value = -1;
    {
        auto resp = parser_.sendCommand("AT+UGPIOC?");
        char buf[32] = {};
        CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
        if (!strcmp(buf, "+UGPIOC:")) {
            while (resp.hasNextLine()) {
                int p, m;
                const int r = CHECK_PARSER(resp.scanf("%d,%d", &p, &m));
                if (r == 2 && p == UBLOX_NCP_SIM_SELECT_PIN) {
                    mode = m;
                }
            }
        }
        const int r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    if (mode == 0) {
        int p, v;
        auto resp = parser_.sendCommand("AT+UGPIOR=%u", UBLOX_NCP_SIM_SELECT_PIN);
        int r = CHECK_PARSER(resp.scanf("+UGPIO%*[R:] %d%*[ ,]%d", &p, &v));
        if (r == 2 && p == UBLOX_NCP_SIM_SELECT_PIN) {
            value = v;
        }
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    bool reset = false;

    switch (conf_.simType()) {
        case SimType::EXTERNAL: {
            LOG(INFO, "Using external Nano SIM card");
            const int externalSimMode = 0;
            const int externalSimValue = 0;
            if (mode != externalSimMode || externalSimValue != value) {
                const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d,%d",
                        UBLOX_NCP_SIM_SELECT_PIN, externalSimMode, externalSimValue));
                CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
                reset = true;
            }
            break;
        }
        case SimType::INTERNAL:
        default: {
            LOG(INFO, "Using internal SIM card");
            if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
                const int internalSimMode = 255;
                if (mode != internalSimMode) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
                    reset = true;
                }
            } else {
                const int internalSimMode = 0;
                const int internalSimValue = 1;
                if (mode != internalSimMode || value != internalSimValue) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode, internalSimValue));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
                    reset = true;
                }
            }
            break;
        }
    }

    if (reset) {
        if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
            // U201
            const int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=16"));
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
            HAL_Delay_Milliseconds(1000);
        } else {
            // R410
            const int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=15"));
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
            HAL_Delay_Milliseconds(10000);
        }

        CHECK(waitAtResponse(20000));
    }

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

int SaraNcpClient::changeBaudRate(unsigned int baud) {
    auto resp = parser_.sendCommand("AT+IPR=%u", baud);
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return serial_->setBaudRate(baud);
}

int SaraNcpClient::initReady() {
    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard());

    // Just in case disconnect
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        // Change the baudrate to 921600
        CHECK(changeBaudRate(UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2));
        // Check that the modem is responsive at the new baudrate
        skipAll(serial_.get(), 1000);
        CHECK(waitAtResponse(10000));
    }

    if (ncpId() == MESH_NCP_SARA_R410) {
        // Force eDRX mode to be disabled
        // 18/23 hardware doesn't seem to be disabled by default
        r = CHECK_PARSER(parser_.execCommand("AT+CEDRXS=0"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        // Force Power Saving mode to be disabled for good measure
        r = CHECK_PARSER(parser_.execCommand("AT+CPSMS=0"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

        r = CHECK_PARSER(parser_.execCommand("AT+CEDRXS?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        r = CHECK_PARSER(parser_.execCommand("AT+CPSMS?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    } else {
        // Power saving
        r = CHECK_PARSER(parser_.execCommand("AT+UPSV=0"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    // Send AT+CMUX and initialize multiplexer
    r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,,1509,,,,,"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Initialize muxer
    muxer_.setStream(serial_.get());
    muxer_.setMaxFrameSize(UBLOX_NCP_MAX_MUXER_FRAME_SIZE);
    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD);
        muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED);
        muxer_.setMaxRetransmissions(10);
        muxer_.setAckTimeout(100);
        muxer_.setControlResponseTimeout(500);
    } else {
        muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD * 2);
        muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED);
        muxer_.useMscAsKeepAlive(true);
        muxer_.setMaxRetransmissions(3);
        muxer_.setAckTimeout(2530);
        muxer_.setControlResponseTimeout(2540);
    }

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    NAMED_SCOPE_GUARD(muxerSg, {
        muxer_.stop();
    });

    // Start muxer (blocking call)
    CHECK_TRUE(muxer_.start(true) == 0, SYSTEM_ERROR_UNKNOWN);

    // Open AT channel and connect it to AT channel stream
    if (muxer_.openChannel(UBLOX_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
        // Failed to open AT channel
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Just in case resume AT channel
    muxer_.resumeChannel(UBLOX_NCP_AT_CHANNEL);

    // Reinitialize parser with a muxer-based stream
    CHECK(initParser(muxerAtStream_.get()));

    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        CHECK(waitAtResponse(10000));
    } else {
        CHECK(waitAtResponse(20000, 5000));
    }
    ncpState(NcpState::ON);
    LOG_DEBUG(TRACE, "Muxer AT channel live");

    muxerSg.dismiss();

    return 0;
}

int SaraNcpClient::checkSimCard() {
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

int SaraNcpClient::configureApn(const CellularNetworkConfig& conf) {
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

int SaraNcpClient::registerNet() {
    int r = 0;
    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        r = CHECK_PARSER(parser_.execCommand("AT+CREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        r = CHECK_PARSER(parser_.execCommand("AT+CGREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    } else {
        r = CHECK_PARSER(parser_.execCommand("AT+CEREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    connectionState(NcpConnectionState::CONNECTING);

    // NOTE: up to 3 mins
    r = CHECK_PARSER(parser_.execCommand(3 * 60 * 1000, "AT+COPS=0"));
    // Ignore response code here
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        r = CHECK_PARSER(parser_.execCommand("AT+CREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        r = CHECK_PARSER(parser_.execCommand("AT+CGREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    } else {
        r = CHECK_PARSER(parser_.execCommand("AT+CEREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;

    return 0;
}

void SaraNcpClient::ncpState(NcpState state) {
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

void SaraNcpClient::connectionState(NcpConnectionState state) {
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
        int r = muxer_.openChannel(UBLOX_NCP_PPP_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
            auto self = (SaraNcpClient*)ctx;
            const auto handler = self->conf_.dataHandler();
            if (handler) {
                handler(0, data, size, self->conf_.dataHandlerData());
            }
            return 0;
        }, this);
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

int SaraNcpClient::muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
        decltype(muxer_)::ChannelState newState, void* ctx) {
    auto self = (SaraNcpClient*)ctx;
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
            case UBLOX_NCP_PPP_CHANNEL: {
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

void SaraNcpClient::resetRegistrationState() {
    creg_ = RegistrationState::NotRegistered;
    cgreg_ = RegistrationState::NotRegistered;
    cereg_ = RegistrationState::NotRegistered;
    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;
}

void SaraNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if ((creg_ == RegistrationState::Registered &&
                    cgreg_ == RegistrationState::Registered) ||
                    cereg_ == RegistrationState::Registered) {
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            connectionState(NcpConnectionState::CONNECTING);
            regStartTime_ = millis();
            regCheckTime_ = regStartTime_;
        }
    }
}

int SaraNcpClient::processEventsImpl() {
    CHECK_TRUE(ncpState_ == NcpState::ON, SYSTEM_ERROR_INVALID_STATE);
    parser_.processUrc(); // Ignore errors
    checkRegistrationState();
    if (connState_ != NcpConnectionState::CONNECTING ||
            millis() - regCheckTime_ < REGISTRATION_CHECK_INTERVAL) {
        return 0;
    }
    SCOPE_GUARD({
        regCheckTime_ = millis();
    });
    if (conf_.ncpIdentifier() != MESH_NCP_SARA_R410) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    } else {
        CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
    }
    if (connState_ == NcpConnectionState::CONNECTING &&
            millis() - regStartTime_ >= REGISTRATION_TIMEOUT) {
        LOG(WARN, "Resetting the modem due to the network registration timeout");
        muxer_.stop();
        modemHardReset();
        ncpState(NcpState::OFF);
    }
    return 0;
}

int SaraNcpClient::modemInit() const {
    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = 0,
        .mode = OUTPUT,
        .set_value = true,
        .value = 1
    };

    // Configure PWR_ON and RESET_N pins as Open-Drain and set to high by default
    CHECK(HAL_Pin_Configure(UBPWR, &conf));
    CHECK(HAL_Pin_Configure(UBRST, &conf));

    conf.mode = OUTPUT;
    // Configure BUFEN as Push-Pull Output and default to 1 (disabled)
    CHECK(HAL_Pin_Configure(BUFEN, &conf));

    // Configure VINT as Input for modem power state monitoring
    conf.mode = INPUT;
    CHECK(HAL_Pin_Configure(UBVINT, &conf));

    LOG(TRACE, "Modem low level initialization OK");

    return 0;
}

int SaraNcpClient::modemPowerOn() const {
    if (!modemPowerState()) {
        LOG(TRACE, "Powering modem on");
        // Perform power-on sequence depending on the NCP type
        if (ncpId() != MESH_NCP_SARA_R410) {
            // U201
            // Low pulse 50-80us
            ATOMIC_BLOCK() {
                HAL_GPIO_Write(UBPWR, 0);
                HAL_Delay_Microseconds(50);
                HAL_GPIO_Write(UBPWR, 1);
            }
        } else {
            // R410
            // Low pulse 150-3200ms
            HAL_GPIO_Write(UBPWR, 0);
            HAL_Delay_Milliseconds(150);
            HAL_GPIO_Write(UBPWR, 1);
        }

        bool powerGood;
        // Verify that the module was powered up by checking the VINT pin up to 1 sec
        for (unsigned i = 0; i < 10; i++) {
            powerGood = modemPowerState();
            if (powerGood) {
                break;
            }
            HAL_Delay_Milliseconds(100);
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

int SaraNcpClient::modemPowerOff() const {
    static std::once_flag f;
    std::call_once(f, [this]() {
        if (ncpId() != MESH_NCP_SARA_R410 && modemPowerState()) {
            // U201 will auto power-on when it detects a rising VIN
            // If we perform a power-off sequence immediately after it just started
            // to power-on, it will not be detected. Add an artificial delay here.
            int reason;
            if (!HAL_Core_Get_Last_Reset_Info(&reason, nullptr, nullptr) &&
                    (reason == RESET_REASON_POWER_DOWN || reason == RESET_REASON_POWER_BROWNOUT)) {
                auto now = HAL_Timer_Get_Milli_Seconds();
                if (now < 5000) {
                    HAL_Delay_Milliseconds(5000 - now);
                }
            }
        }
    });

    if (modemPowerState()) {
        LOG(TRACE, "Powering modem off");
        // Important! We need to disable voltage translator here
        // otherwise V_INT will never go low
        modemSetUartState(false);
        // Perform power-off sequence depending on the NCP type
        if (ncpId() != MESH_NCP_SARA_R410) {
            // U201
            // Low pulse 1s+
            HAL_GPIO_Write(UBPWR, 0);
            HAL_Delay_Milliseconds(1500);
            HAL_GPIO_Write(UBPWR, 1);
        } else {
            // R410
            // Low pulse 1.5s+
            HAL_GPIO_Write(UBPWR, 0);
            HAL_Delay_Milliseconds(1600);
            HAL_GPIO_Write(UBPWR, 1);
        }

        bool powerGood;
        // Verify that the module was powered down by checking the VINT pin up to 10 sec
        for (unsigned i = 0; i < 100; i++) {
            powerGood = modemPowerState();
            if (!powerGood) {
                break;
            }
            HAL_Delay_Milliseconds(100);
        }
        if (!powerGood) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem");
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return 0;
}

int SaraNcpClient::modemHardReset() const {
    const auto pwrState = modemPowerState();
    // We can only reset the modem in the powered state
    if (!pwrState) {
        LOG(ERROR, "Cannot hard reset the modem, it's not on");
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(TRACE, "Hard resetting the modem");
    if (ncpId() != MESH_NCP_SARA_R410) {
        // U201
        // Low pulse for 50ms
        HAL_GPIO_Write(UBRST, 0);
        HAL_Delay_Milliseconds(50);
        HAL_GPIO_Write(UBRST, 1);
        HAL_Delay_Milliseconds(1000);
    } else {
        // R410
        // Low pulse for 10s
        HAL_GPIO_Write(UBRST, 0);
        HAL_Delay_Milliseconds(10000);
        HAL_GPIO_Write(UBRST, 1);
        // Just in case waiting here for one more second,
        // won't hurt, we've already waited for 10
        HAL_Delay_Milliseconds(1000);
        // IMPORTANT: R4 is powered-off after applying RESET!
        return modemPowerOn();
    }
    return 0;
}

bool SaraNcpClient::modemPowerState() const {
    return HAL_GPIO_Read(UBVINT);
}

int SaraNcpClient::modemSetUartState(bool state) const {
    LOG(TRACE, "Setting UART voltage translator state %d", state);
    HAL_GPIO_Write(BUFEN, state ? 0 : 1);
    return 0;
}

} // particle
