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

#include "sara_u2_ncp_client.h"

#include "at_command.h"
#include "at_response.h"

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

void ubloxReset() {
    HAL_GPIO_Write(BUFEN, 0);

    HAL_GPIO_Write(UBRST, 0);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(UBRST, 1);

    HAL_GPIO_Write(UBPWR, 0);
    HAL_Delay_Milliseconds(50);
    HAL_GPIO_Write(UBPWR, 1);
    HAL_Delay_Milliseconds(10);
}

const auto UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE = 115200;
const auto UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE = 921600;

const auto UBLOX_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto UBLOX_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto UBLOX_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto UBLOX_NCP_AT_CHANNEL = 1;
const auto UBLOX_NCP_PPP_CHANNEL = 2;

} // anonymous

SaraU2NcpClient::SaraU2NcpClient() {
}

SaraU2NcpClient::~SaraU2NcpClient() {
    destroy();
}

int SaraU2NcpClient::init(const NcpClientConfig& conf) {
    // Make sure Ublox module is powered down
    HAL_Pin_Mode(UBPWR, OUTPUT);
    HAL_Pin_Mode(UBRST, OUTPUT);
    HAL_Pin_Mode(BUFEN, OUTPUT);
    ubloxOff();
    // Initialize serial stream
    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_USART_SERIAL2,
            UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);
    CHECK(initParser(serial.get()));
    serial_ = std::move(serial);
    conf_ = conf;
    ncpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    parserError_ = 0;
    ready_ = false;
    resetRegistrationState();
    return 0;
}

void SaraU2NcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        ubloxOff();
    }
    parser_.destroy();
    serial_.reset();
}

int SaraU2NcpClient::initParser(Stream* stream) {
    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(stream)
            .commandTerminator(AtCommandTerminator::CRLF);
    parser_.destroy();
    CHECK(parser_.init(std::move(parserConf)));
    CHECK(parser_.addUrcHandler("+CME ERROR", nullptr, nullptr)); // Ignore
    CHECK(parser_.addUrcHandler("+CMS ERROR", nullptr, nullptr)); // Ignore
    CHECK(parser_.addUrcHandler("+CREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraU2NcpClient*)data;
        int val;
        int r = CHECK_PARSER_URC(reader->scanf("+CREG: %d", &val));
        CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
        // Home network or roaming
        if (val == 1 || val == 5) {
            self->creg_ = RegistrationState::Registered;
        } else {
            self->creg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
        return 0;
    }, this));
    CHECK(parser_.addUrcHandler("+CGREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraU2NcpClient*)data;
        int val;
        int r = CHECK_PARSER_URC(reader->scanf("+CGREG: %d", &val));
        CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
        // Home network or roaming
        if (val == 1 || val == 5) {
            self->cgreg_ = RegistrationState::Registered;
        } else {
            self->cgreg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
        return 0;
    }, this));
    CHECK(parser_.addUrcHandler("+CEREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraU2NcpClient*)data;
        int val;
        int r = CHECK_PARSER_URC(reader->scanf("+CEREG: %d", &val));
        CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
        // Home network or roaming
        if (val == 1 || val == 5) {
            self->cereg_ = RegistrationState::Registered;
        } else {
            self->cereg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
        return 0;
    }, this));
    return 0;
}

int SaraU2NcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    CHECK(waitReady());
    return 0;
}

void SaraU2NcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::OFF) {
        return;
    }
    ubloxOff();
    ready_ = false;
    ncpState(NcpState::OFF);
}

NcpState SaraU2NcpClient::ncpState() {
    const NcpClientLock lock(this);
    return ncpState_;
}

int SaraU2NcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return 0;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    resetRegistrationState();

    connectionState(NcpConnectionState::DISCONNECTED);
    return 0;
}

NcpConnectionState SaraU2NcpClient::connectionState() {
    const NcpClientLock lock(this);
    return connState_;
}

int SaraU2NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGMR");
    CHECK_PARSER(resp.readLine(buf, size));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int SaraU2NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraU2NcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraU2NcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    return muxer_.writeChannel(UBLOX_NCP_PPP_CHANNEL, data, size);
}

void SaraU2NcpClient::processEvents() {
    const NcpClientLock lock(this);
    if (ncpState_ != NcpState::ON) {
        return;
    }
    parser_.processUrc();
}

int SaraU2NcpClient::ncpId() const {
    return MeshNCPIdentifier::MESH_NCP_SARA_U201;
}

int SaraU2NcpClient::connect(const CellularNetworkConfig& conf) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ == NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK(checkParser());

    resetRegistrationState();
    int simState = 0;
    for (unsigned i = 0; i < 10; ++i) {
        simState = checkSimCard();
        if (!simState) {
            break;
        }
        HAL_Delay_Milliseconds(1000);
    }
    CHECK(simState);
    CHECK(configureApn(conf));
    CHECK(registerNet());

    connectionState(NcpConnectionState::CONNECTING);

    return 0;
}

int SaraU2NcpClient::getIccid(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CCID");
    char iccid[32] = {};
    const int r = resp.scanf("+CCID: %31s", iccid);
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
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

int SaraU2NcpClient::checkParser() {
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

int SaraU2NcpClient::waitAtResponse(unsigned int timeout) {
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(1000, "AT");
        if (r == AtResponse::OK) {
            return 0;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= timeout) {
            break;
        }
        if (r != SYSTEM_ERROR_TIMEOUT) {
            HAL_Delay_Milliseconds(1000);
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

int SaraU2NcpClient::waitReady() {
    if (ready_) {
        return 0;
    }
    muxer_.stop();
    CHECK(serial_->setBaudRate(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
    ubloxReset();
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

int SaraU2NcpClient::selectSimCard() {
    // TODO: for now always using external SIM card
    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=23,0,0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    return 0;
}

int SaraU2NcpClient::changeBaudRate(unsigned int baud) {
    auto resp = parser_.sendCommand("AT+IPR=%u", baud);
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return serial_->setBaudRate(baud);
}

int SaraU2NcpClient::initReady() {
    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard());

    // Just in case disconnect
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Change the baudrate to 921600
    CHECK(changeBaudRate(UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE));
    // Check that the modem is responsive at the new baudrate
    skipAll(serial_.get(), 1000);
    CHECK(waitAtResponse(10000));

    // Send AT+CMUX and initialize multiplexer
    r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,,1509,,,,,"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Initialize muxer
    muxer_.setStream(serial_.get());
    muxer_.setMaxFrameSize(UBLOX_NCP_MAX_MUXER_FRAME_SIZE);
    muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD);
    muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED);

    // XXX: we might need to adjust these:
    muxer_.setMaxRetransmissions(10);
    muxer_.setAckTimeout(100);
    muxer_.setControlResponseTimeout(500);

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

    CHECK(waitAtResponse(10000));
    ncpState(NcpState::ON);
    LOG_DEBUG(TRACE, "Muxer AT channel live");

    muxerSg.dismiss();

    return 0;
}

int SaraU2NcpClient::checkSimCard() {
    auto resp = parser_.sendCommand("AT+CPIN?");
    char code[32] = {};
    int r = CHECK_PARSER(resp.scanf("+CPIN: %32[^\n]", code));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    if (!strcmp(code, "READY")) {
        return 0;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int SaraU2NcpClient::configureApn(const CellularNetworkConfig& conf) {
    // FIXME: for now IPv4 context only
    auto resp = parser_.sendCommand("AT+CGDCONT=1,\"IP\",\"%s%s\"",
            conf.user() && conf.password() ? "CHAP:" : "", conf.apn() ? conf.apn() : "");
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int SaraU2NcpClient::registerNet() {
    int r = CHECK_PARSER(parser_.execCommand("AT+CREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(parser_.execCommand("AT+CGREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    // Only applies to LTE:
    // r = CHECK_PARSER(parser_.execCommand("AT+CEREG=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(parser_.execCommand("AT+COPS=0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

void SaraU2NcpClient::ncpState(NcpState state) {
    if (ncpState_ == state) {
        return;
    }
    LOG(TRACE, "NCP state changed: %d", (int)ncpState_);
    ncpState_ = state;
    if (ncpState_ == NcpState::OFF) {
        connectionState(NcpConnectionState::DISCONNECTED);
    }
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpStateChangedEvent event = {};
        event.type = NcpEvent::NCP_STATE_CHANGED;
        event.state = ncpState_;
        handler(event, conf_.eventHandlerData());
    }
}

void SaraU2NcpClient::connectionState(NcpConnectionState state) {
    if (connState_ == state) {
        return;
    }
    LOG(TRACE, "NCP connection state changed: %d", (int)state);
    connState_ = state;

    if (connState_ == NcpConnectionState::CONNECTED) {
        // Open data channel
        int r = muxer_.openChannel(UBLOX_NCP_PPP_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
            auto self = (SaraU2NcpClient*)ctx;
            const auto handler = self->conf_.dataHandler();
            if (handler) {
                handler(0, data, size, self->conf_.dataHandlerData());
            }
            return 0;
        }, this);

        if (r) {
            // Some kind of an error
            connectionState(NcpConnectionState::DISCONNECTED);
            return;
        }
    } else {
        muxer_.closeChannel(UBLOX_NCP_PPP_CHANNEL);
    }

    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpConnectionStateChangedEvent event = {};
        event.type = NcpEvent::CONNECTION_STATE_CHANGED;
        event.state = connState_;
        handler(event, conf_.eventHandlerData());
    }
}

int SaraU2NcpClient::muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
        decltype(muxer_)::ChannelState newState, void* ctx) {
    auto self = (SaraU2NcpClient*)ctx;
    // We are only interested in Closed state
    if (newState == decltype(muxer_)::ChannelState::Closed) {
        switch (channel) {
            case 0: {
                // Muxer stopped
                self->ncpState(NcpState::OFF);
                break;
            }
            case UBLOX_NCP_PPP_CHANNEL: {
                // Notify that the underlying data channel closed
                self->connectionState(NcpConnectionState::CONNECTING);
                // Attempt to re-establish
                self->checkRegistrationState();
                break;
            }
        }
    }

    return 0;
}

void SaraU2NcpClient::resetRegistrationState() {
    creg_ = RegistrationState::NotRegistered;
    cgreg_ = RegistrationState::NotRegistered;
    cereg_ = RegistrationState::NotRegistered;
}

void SaraU2NcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        // FIXME: CEREG
        if (creg_ == RegistrationState::Registered && cgreg_ == RegistrationState::Registered) {
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            connectionState(NcpConnectionState::CONNECTING);
        }
    }
}

} // particle
