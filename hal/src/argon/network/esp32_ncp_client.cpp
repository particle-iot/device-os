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
#include "esp32_ncp_client.h"

#include "at_command.h"
#include "at_response.h"

#include "gpio_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "serial_stream.h"

#include "xmodem_sender.h"
#include "stream_util.h"
#include "check.h"

#include <cstdlib>

#define CHECK_PARSER(_expr) \
        ({ \
            const auto _r = _expr; \
            if (_r < 0) { \
                this->parserError(_r); \
                return _r; \
            } \
            _r; \
        })

namespace particle {

namespace {

void espReset() {
    HAL_GPIO_Write(ESPBOOT, 1);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(ESPEN, 0);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(ESPEN, 1);
    HAL_Delay_Milliseconds(100);
}

void espOff() {
    HAL_GPIO_Write(ESPEN, 0);
}

const auto ESP32_NCP_MAX_MUXER_FRAME_SIZE = 1536;
const auto ESP32_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto ESP32_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto ESP32_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto ESP32_NCP_AT_CHANNEL = 1;
const auto ESP32_NCP_STA_CHANNEL = 2;
const auto ESP32_NCP_AP_CHANNEL = 3;

} // unnamed

Esp32NcpClient::Esp32NcpClient() :
        ncpState_(NcpState::OFF),
        connState_(NcpConnectionState::DISCONNECTED),
        parserError_(0),
        ready_(false) {
}

Esp32NcpClient::~Esp32NcpClient() {
    destroy();
}

int Esp32NcpClient::init(const NcpClientConfig& conf) {
    // Make sure ESP32 is powered down
    HAL_Pin_Mode(ESPBOOT, OUTPUT);
    HAL_Pin_Mode(ESPEN, OUTPUT);
    espOff();
    // Initialize serial stream
    std::unique_ptr<SerialStream> serial(new(std::nothrow) SerialStream(HAL_USART_SERIAL2, 921600,
            SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);
    CHECK(initParser(serial.get()));
    serial_ = std::move(serial);
    conf_ = conf;
    ncpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    parserError_ = 0;
    ready_ = false;
    return 0;
}

int Esp32NcpClient::initParser(Stream* stream) {
    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(stream)
            .commandTerminator(AtCommandTerminator::CRLF);
    parser_.destroy();
    CHECK(parser_.init(std::move(parserConf)));
    // Register URC handlers
    CHECK(parser_.addUrcHandler("WIFI CONNECTED", nullptr, nullptr)); // Ignore
    CHECK(parser_.addUrcHandler("WIFI DISCONNECT", [](AtResponseReader* reader, const char* prefix, void* data) {
        const auto self = (Esp32NcpClient*)data;
        self->connectionState(NcpConnectionState::DISCONNECTED);
        return 0;
    }, this));
    return 0;
}

void Esp32NcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        espOff();
    }
    parser_.destroy();
    serial_.reset();
}

int Esp32NcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    CHECK(waitReady());
    return 0;
}

void Esp32NcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::OFF) {
        return;
    }
    espOff();
    ready_ = false;
    ncpState(NcpState::OFF);
}

NcpState Esp32NcpClient::ncpState() {
    const NcpClientLock lock(this);
    return ncpState_;
}

int Esp32NcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return 0;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+CWQAP"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    connectionState(NcpConnectionState::DISCONNECTED);
    return 0;
}

NcpConnectionState Esp32NcpClient::connectionState() {
    const NcpClientLock lock(this);
    return connState_;
}

int Esp32NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGMR");
    CHECK_PARSER(resp.readLine(buf, size));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int Esp32NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    char buf[16] = {};
    auto resp = parser_.sendCommand("AT+MVER");
    CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(buf[0] != '\0', SYSTEM_ERROR_UNKNOWN);
    char* end = nullptr;
    const auto val = strtol(buf, &end, 10);
    CHECK_TRUE(end != nullptr && *end == '\0', SYSTEM_ERROR_UNKNOWN);
    *ver = val;
    return 0;
}

int Esp32NcpClient::updateFirmware(InputStream* file, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+FWUPD=%u", (unsigned)size);
    char buf[32] = {};
    CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
    if (strcmp(buf, "+FWUPD: ONGOING") != 0) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    LOG(TRACE, "Initiating XMODEM transfer");
    parser_.reset();
    const auto strm = parser_.config().stream();
    CHECK(skipWhitespace(strm, 1000));
    XmodemSender sender;
    CHECK(sender.init(strm, file, size));
    bool ok = false;
    for (;;) {
        const int r = sender.run();
        if (r != XmodemSender::RUNNING) {
            if (r == XmodemSender::DONE) {
                LOG(INFO, "XMODEM transfer finished");
                ok = true;
            } else {
                LOG(ERROR, "XMODEM transfer failed: %d", r);
            }
            break;
        }
    }
    if (!ok) {
        CHECK(skipAll(strm, 3000));
        return SYSTEM_ERROR_UNKNOWN;
    }
    CHECK(skipNonPrintable(strm, 30000));
    CHECK(readLine(strm, buf, sizeof(buf), 1000));
    if (strcmp(buf, "OK") != 0) {
        LOG(ERROR, "Unexpected result code: \"%s\"", buf);
        return SYSTEM_ERROR_UNKNOWN;
    }
    // FIXME: Find a better way to reset the client state
    off();
    CHECK(on());
    return 0;
}

void Esp32NcpClient::processEvents() {
    const NcpClientLock lock(this);
    if (ncpState_ != NcpState::ON) {
        return;
    }
    parser_.processUrc();
}

AtParser* Esp32NcpClient::atParser() {
    return &parser_;
}

int Esp32NcpClient::connect(const char* ssid, const MacAddress& bssid, WifiSecurity sec, const WifiCredentials& cred) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ == NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK(checkParser());

    // Open data channel
    CHECK(muxer_.openChannel(ESP32_NCP_STA_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
        auto self = (Esp32NcpClient*)ctx;
        const auto handler = self->conf_.dataHandler();
        if (handler) {
            handler(0, data, size, self->conf_.dataHandlerData());
        }
        return 0;
    }, this));

    auto cmd = parser_.command();
    cmd.printf("AT+CWJAP=\"%s\"", ssid);
    switch (cred.type()) {
    case WifiCredentials::PASSWORD:
        cmd.printf(",\"%s\"", cred.password());
        break;
    case WifiCredentials::NONE:
        cmd.print(",\"\"");
        break;
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (bssid != INVALID_MAC_ADDRESS) {
        char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        macAddressToString(bssid, bssidStr, sizeof(bssidStr));
        cmd.printf(",\"%s\"", bssidStr);
    }
    const int r = CHECK_PARSER(cmd.exec());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    connectionState(NcpConnectionState::CONNECTED);
    return 0;
}

int Esp32NcpClient::getNetworkInfo(WifiNetworkInfo* info) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CWJAP?");
    char ssid[MAX_SSID_SIZE + 1] = {};
    char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
    int channel = 0;
    int rssi = 0;
    int r = CHECK_PARSER(resp.scanf("+CWJAP: \"%32[^\"]\",\"%17[^\"]\",%d,%d", ssid, bssidStr, &channel, &rssi));
    CHECK_TRUE(r == 4, SYSTEM_ERROR_UNKNOWN);
    MacAddress bssid = INVALID_MAC_ADDRESS;
    CHECK_TRUE(macAddressFromString(&bssid, bssidStr), SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    *info = WifiNetworkInfo().ssid(ssid).bssid(bssid).channel(channel).rssi(rssi);
    return 0;
}

int Esp32NcpClient::scan(WifiScanCallback callback, void* data) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CWLAP");
    while (resp.hasNextLine()) {
        char ssid[MAX_SSID_SIZE + 2] = {};
        char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        int security = 0;
        int channel = 0;
        int rssi = 0;
        const int r = CHECK_PARSER(resp.scanf("+CWLAP:(%d,\"%33[^,],%d,\"%17[^\"]\",%d)", &security, ssid, &rssi,
                bssidStr, &channel));
        CHECK_TRUE(r == 5, SYSTEM_ERROR_UNKNOWN);
        // Fixup SSID
        CHECK_TRUE(strlen(ssid) > 0, SYSTEM_ERROR_UNKNOWN);
        ssid[strlen(ssid) - 1] = '\0';
        MacAddress bssid = INVALID_MAC_ADDRESS;
        CHECK_TRUE(macAddressFromString(&bssid, bssidStr), SYSTEM_ERROR_UNKNOWN);
        auto result = WifiScanResult().ssid(ssid).bssid(bssid).security((WifiSecurity)security).channel(channel)
                .rssi(rssi);
        CHECK(callback(std::move(result), data));
    }
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int Esp32NcpClient::getMacAddress(MacAddress* addr) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+GETMAC=0"); // WiFi station
    char addrStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
    int r = CHECK_PARSER(resp.scanf("+GETMAC: \"%32[^\"]\"", addrStr));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(macAddressFromString(addr, addrStr), SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int Esp32NcpClient::checkParser() {
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

int Esp32NcpClient::waitReady() {
    if (ready_) {
        return 0;
    }
    muxer_.stop();
    CHECK(initParser(serial_.get()));
    espReset();
    skipAll(serial_.get(), 1000);
    parser_.reset();
    const unsigned timeout = 10000;
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(1000, "AT");
        if (r == AtResponse::OK) {
            ready_ = true;
            break;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= timeout) {
            break;
        }
        if (r != SYSTEM_ERROR_TIMEOUT) {
            HAL_Delay_Milliseconds(1000);
        }
    }
    if (!ready_) {
        LOG(ERROR, "No response from NCP");
        espOff();
        ncpState(NcpState::OFF);
        return SYSTEM_ERROR_INVALID_STATE;
    }
    skipAll(serial_.get(), 1000);
    parser_.reset();
    parserError_ = 0;
    LOG(TRACE, "NCP ready to accept AT commands");
    return initReady();
}

int Esp32NcpClient::initReady() {
    // Send AT+CMUX and initialize multiplexer
    const int r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    // Initialize muxer
    muxer_.setStream(serial_.get());
    muxer_.setMaxFrameSize(ESP32_NCP_MAX_MUXER_FRAME_SIZE);
    muxer_.setKeepAlivePeriod(ESP32_NCP_KEEPALIVE_PERIOD);
    muxer_.setKeepAliveMaxMissed(ESP32_NCP_KEEPALIVE_MAX_MISSED);

    // XXX: we might need to adjust these:
    muxer_.setMaxRetransmissions(10);
    muxer_.setAckTimeout(100);
    muxer_.setControlResponseTimeout(500);

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    // Create AT channel stream
    if (!muxerAtStream_) {
        muxerAtStream_.reset(new (std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, ESP32_NCP_AT_CHANNEL));
        CHECK_TRUE(muxerAtStream_, SYSTEM_ERROR_NO_MEMORY);
        CHECK(muxerAtStream_->init(ESP32_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    }

    // Start muxer (blocking call)
    CHECK_TRUE(muxer_.start(true) == 0, SYSTEM_ERROR_UNKNOWN);

    // Open AT channel and connect it to AT channel stream
    if (muxer_.openChannel(ESP32_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
        // Failed to open AT channel
        muxer_.stop();
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Just in case resume AT channel
    muxer_.resumeChannel(ESP32_NCP_AT_CHANNEL);

    // Reinitialize parser with a muxer-based stream
    CHECK(initParser(muxerAtStream_.get()));

    const unsigned timeout = 5000;
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(1000, "AT");
        if (r == AtResponse::OK) {
            // Success, remote end answered to an AT command
            LOG_DEBUG(TRACE, "Muxer AT channel live");
            const int r = CHECK_PARSER(parser_.execCommand("AT+CWDHCP=0,3"));
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
            ncpState(NcpState::ON);
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

    return SYSTEM_ERROR_UNKNOWN;
}

void Esp32NcpClient::ncpState(NcpState state) {
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

void Esp32NcpClient::connectionState(NcpConnectionState state) {
    if (connState_ == state) {
        return;
    }
    LOG(TRACE, "NCP connection state changed: %d", (int)state);
    connState_ = state;
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpConnectionStateChangedEvent event = {};
        event.type = NcpEvent::CONNECTION_STATE_CHANGED;
        event.state = connState_;
        handler(event, conf_.eventHandlerData());
    }
}

int Esp32NcpClient::muxChannelStateCb(uint8_t channel, decltype(muxer_)::ChannelState oldState,
        decltype(muxer_)::ChannelState newState, void* ctx) {
    auto self = (Esp32NcpClient*)ctx;
    // We are only interested in Closed state
    if (newState == decltype(muxer_)::ChannelState::Closed) {
        switch (channel) {
            case 0: {
                // Muxer stopped
                self->ncpState(NcpState::OFF);
                break;
            }
            case ESP32_NCP_STA_CHANNEL: {
                // Notify that the underlying data channel closed
                self->connectionState(NcpConnectionState::DISCONNECTED);
                break;
            }
        }
    }

    return 0;
}

int Esp32NcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    if (id == 0) {
        return muxer_.writeChannel(ESP32_NCP_STA_CHANNEL, data, size);
    } else if (id == 1) {
        return muxer_.writeChannel(ESP32_NCP_AP_CHANNEL, data, size);
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

} // particle
