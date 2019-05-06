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
#include "str_util.h"
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

size_t espEscape(const char* src, char* dest, size_t destSize) {
    return escape(src, ",\"\\", '\\', dest, destSize);
}

const auto ESP32_NCP_MAX_MUXER_FRAME_SIZE = 1536;
const auto ESP32_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto ESP32_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto ESP32_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto ESP32_NCP_AT_CHANNEL = 1;
const auto ESP32_NCP_STA_CHANNEL = 2;
const auto ESP32_NCP_AP_CHANNEL = 3;

const auto ESP32_NCP_MIN_MVER_WITH_CMUX = 4;

} // unnamed

Esp32NcpClient::Esp32NcpClient() :
        ncpState_(NcpState::OFF),
        prevNcpState_(NcpState::OFF),
        connState_(NcpConnectionState::DISCONNECTED),
        parserError_(0),
        ready_(false),
        muxerNotStarted_(false) {
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
    // Initialize muxed channel stream
    decltype(muxerAtStream_) muxStrm(new(std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, ESP32_NCP_AT_CHANNEL));
    CHECK_TRUE(muxStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxStrm->init(ESP32_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    CHECK(initParser(serial.get()));
    serial_ = std::move(serial);
    muxerAtStream_ = std::move(muxStrm);
    conf_ = conf;
    ncpState_ = NcpState::OFF;
    prevNcpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    parserError_ = 0;
    ready_ = false;
    muxerNotStarted_ = false;
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
    muxerAtStream_.reset();
    serial_.reset();
}

int Esp32NcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    CHECK(waitReady());
    return 0;
}

int Esp32NcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    muxer_.stop();
    espOff();
    ready_ = false;
    ncpState(NcpState::OFF);
    return 0;
}

int Esp32NcpClient::enable() {
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

void Esp32NcpClient::disable() {
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

NcpState Esp32NcpClient::ncpState() {
    return ncpState_;
}

int Esp32NcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return 0;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+CWQAP"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    connectionState(NcpConnectionState::DISCONNECTED);
    return 0;
}

NcpConnectionState Esp32NcpClient::connectionState() {
    return connState_;
}

int Esp32NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGMR");
    CHECK_PARSER(resp.readLine(buf, size));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    return 0;
}

int Esp32NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    return getFirmwareModuleVersionImpl(ver);
}

int Esp32NcpClient::getFirmwareModuleVersionImpl(uint16_t* ver) {
    char buf[16] = {};
    auto resp = parser_.sendCommand("AT+MVER");
    CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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
    // Stop the muxer
    muxer_.stop();
    // Wait a bit for the NCP to reset
    HAL_Delay_Milliseconds(2000);
    // Re-enable the client in case it was asynchronously disabled by the muxer
    // due to the NCP reset
    enable();
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
    int r = muxer_.openChannel(ESP32_NCP_STA_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
        auto self = (Esp32NcpClient*)ctx;
        const auto handler = self->conf_.dataHandler();
        if (handler) {
            handler(0, data, size, self->conf_.dataHandlerData());
        }
        return 0;
    }, this);
    // We are running an older NCP firmware without muxer support, ignore error here
    CHECK_TRUE(r == 0 || (r == gsm0710::GSM0710_ERROR_INVALID_STATE && muxerNotStarted_),
            SYSTEM_ERROR_UNKNOWN);

    auto cmd = parser_.command();
    char escSsid[MAX_SSID_SIZE * 2 + 1] = {}; // Escaped SSID
    espEscape(ssid, escSsid, sizeof(escSsid) - 1);
    cmd.printf("AT+CWJAP=\"%s\"", escSsid);
    switch (cred.type()) {
    case WifiCredentials::PASSWORD: {
        LOG(TRACE, "Connecting to \"%s\"", ssid);
        char escPwd[MAX_WPA_WPA2_PSK_SIZE * 2 + 1] = {}; // Escaped password
        espEscape(cred.password(), escPwd, sizeof(escPwd) - 1);
        cmd.printf(",\"%s\"", escPwd);
        break;
    }
    case WifiCredentials::NONE: {
        LOG(TRACE, "Connecting to \"%s\" (no security)", ssid);
        cmd.print(",\"\"");
        break;
    }
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (bssid != INVALID_MAC_ADDRESS) {
        char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        macAddressToString(bssid, bssidStr, sizeof(bssidStr));
        cmd.printf(",\"%s\"", bssidStr);
    }
    parser_.logEnabled(false); // Avoid logging the password
    auto resp = cmd.send();
    parser_.logEnabled(AtParserConfig::DEFAULT_LOG_ENABLED);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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
    CHECK_TRUE(r == 4, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    MacAddress bssid = INVALID_MAC_ADDRESS;
    CHECK_TRUE(macAddressFromString(&bssid, bssidStr), SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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
        if (r != 5) {
            // FIXME: ESP32 doesn't escape special characters, such as ',' and '"', in SSIDs. For now,
            // we're skipping such entries
            LOG(WARN, "Unable to parse AP info");
            continue;
        }
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
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    return 0;
}

int Esp32NcpClient::getMacAddress(MacAddress* addr) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+GETMAC=0"); // WiFi station
    char addrStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
    int r = CHECK_PARSER(resp.scanf("+GETMAC: \"%32[^\"]\"", addrStr));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    CHECK_TRUE(macAddressFromString(addr, addrStr), SYSTEM_ERROR_BAD_DATA);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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
        espOff();
        ncpState(NcpState::OFF);
        return SYSTEM_ERROR_INVALID_STATE;
    }

    return 0;
}

int Esp32NcpClient::initReady() {
    // Send AT+CMUX and initialize multiplexer
    int r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0"));

    if (r != AtResponse::OK) {
        // Check current NCP firmware module version
        uint16_t mver = 0;
        CHECK(getFirmwareModuleVersionImpl(&mver));

        // If it's < ESP32_NCP_MIN_MVER_WITH_CMUX, AT+CMUX is not supposed to work
        // We simply won't initialize it
        CHECK_TRUE(mver < ESP32_NCP_MIN_MVER_WITH_CMUX, SYSTEM_ERROR_UNKNOWN);

        muxerNotStarted_ = true;
    } else {
        CHECK(initMuxer());
        muxerNotStarted_ = false;
    }

    // Disable DHCP on both STA and AP interfaces
    r = CHECK_PARSER(parser_.execCommand("AT+CWDHCP=0,3"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    // Now we are ready
    ncpState(NcpState::ON);
    return 0;
}

int Esp32NcpClient::initMuxer() {
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

void Esp32NcpClient::connectionState(NcpConnectionState state) {
    if (ncpState_ == NcpState::DISABLED) {
        return;
    }
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
    // This callback is executed from the multiplexer thread, not safe to use the lock here
    // because it might get called while blocked inside some muxer function

    // We are only interested in Closed state
    if (newState == decltype(muxer_)::ChannelState::Closed) {
        switch (channel) {
            case 0:
                // Muxer stopped
                self->disable();
                // NOTE: fall-through
            case ESP32_NCP_STA_CHANNEL: {
                // Notify that the underlying data channel closed
                // It should be safe to call this here
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
