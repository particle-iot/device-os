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

#include "stream_util.h"

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

void ubloxOff() {
    HAL_GPIO_Write(UBPWR, 0);
    HAL_Delay_Milliseconds(50);
}

void ubloxReset(unsigned int reset) {
    HAL_GPIO_Write(BUFEN, 0);

    HAL_GPIO_Write(UBRST, 0);
    HAL_Delay_Milliseconds(reset);
    HAL_GPIO_Write(UBRST, 1);

    HAL_GPIO_Write(UBPWR, 0);
    HAL_Delay_Milliseconds(50);
    HAL_GPIO_Write(UBPWR, 1);
    HAL_Delay_Milliseconds(10);
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

} // anonymous

SaraNcpClient::SaraNcpClient() {
}

SaraNcpClient::~SaraNcpClient() {
    destroy();
}

int SaraNcpClient::init(const NcpClientConfig& conf) {
    // Make sure Ublox module is powered down
    HAL_Pin_Mode(UBPWR, OUTPUT);
    HAL_Pin_Mode(UBRST, OUTPUT);
    HAL_Pin_Mode(BUFEN, OUTPUT);
    ubloxOff();

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
    CHECK(initParser(serial.get()));
    serial_ = std::move(serial);
    ncpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    parserError_ = 0;
    ready_ = false;
    resetRegistrationState();
    return 0;
}

void SaraNcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        ubloxOff();
    }
    parser_.destroy();
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
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    CHECK(waitReady());
    return 0;
}

void SaraNcpClient::off() {
    const NcpClientLock lock(this);
    muxer_.stop();
    ubloxOff();
    ready_ = false;
    ncpState(NcpState::OFF);
}

NcpState SaraNcpClient::ncpState() {
    const NcpClientLock lock(this);
    return ncpState_;
}

int SaraNcpClient::disconnect() {
    const NcpClientLock lock(this);
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
    const NcpClientLock lock(this);
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
    if (ncpState_ != NcpState::ON) {
        return;
    }
    parser_.processUrc();
    checkRegistrationState();
}

int SaraNcpClient::ncpId() const {
    return MeshNCPIdentifier::MESH_NCP_SARA_U201;
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
    ubloxReset(conf_.ncpIdentifier() != MESH_NCP_SARA_R410 ? 100 : 10000);
    skipAll(serial_.get(), 1000);
    parser_.reset();
    ready_ = waitAtResponse(20000) == 0;
    if (!ready_) {
        LOG(ERROR, "No response from NCP");
        ubloxOff();
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
        int r = CHECK_PARSER(resp.scanf("+UGPIOR: %d,%d", &p, &v));
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
            HAL_Delay_Milliseconds(100);
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
        muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED / 2 + 1);
        muxer_.useMscAsKeepAlive(true);
        muxer_.setMaxRetransmissions(3);
        muxer_.setAckTimeout(2530);
        muxer_.setControlResponseTimeout(2540);
    }

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    // Create AT channel stream
    if (!muxerAtStream_) {
        muxerAtStream_.reset(new (std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, UBLOX_NCP_AT_CHANNEL));
        CHECK_TRUE(muxerAtStream_, SYSTEM_ERROR_NO_MEMORY);
        CHECK(muxerAtStream_->init(UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    }

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

    return 0;
}

void SaraNcpClient::ncpState(NcpState state) {
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
}

void SaraNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if ((creg_ == RegistrationState::Registered &&
                    cgreg_ == RegistrationState::Registered) ||
                    cereg_ == RegistrationState::Registered) {
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            connectionState(NcpConnectionState::CONNECTING);
        }
    }
}

} // particle
