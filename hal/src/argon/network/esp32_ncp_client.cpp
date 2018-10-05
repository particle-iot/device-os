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

#include "esp32_ncp_client.h"

#include "at_command.h"
#include "at_response.h"

#include "gpio_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "serial_stream.h"

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

int flushInput(InputStream* strm, unsigned timeout = 0) {
    size_t bytesRead = 0;
    auto t = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const size_t n = CHECK(strm->availForRead());
        bytesRead += CHECK(strm->skip(n));
        if (timeout == 0) {
            break;
        }
        const int r = strm->waitEvent(InputStream::READABLE, timeout);
        if (r == SYSTEM_ERROR_TIMEOUT) {
            break;
        }
        if (r < 0) {
            return r;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        t = t2 - t;
        if (t < timeout) {
            timeout -= t;
            t = t2;
        } else {
            timeout = 0;
        }
    }
    return bytesRead;
}

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
    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(serial.get())
            .commandTerminator(AtCommandTerminator::CRLF);
    CHECK(parser_.init(std::move(parserConf)));
    serial_ = std::move(serial);
    conf_ = conf;
    ncpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    parserError_ = 0;
    ready_ = false;
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
    // TODO
    return 0;
}

AtParser* Esp32NcpClient::atParser() {
    return &parser_;
}

int Esp32NcpClient::connect(const char* ssid, const MacAddress& bssid, WifiSecurity sec, const WifiCredentials& cred) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto cmd = parser_.command();
    cmd.printf("AT+CWJAP=%s", ssid);
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
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CWJAP?");
    char ssid[MAX_SSID_SIZE + 1] = {};
    char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
    int channel = 0;
    int rssi = 0;
    int r = CHECK_PARSER(resp.scanf("+CWJAP: \"%32[^\"]\",\"%17[^\"]\",%d,%d", ssid, bssidStr, &channel, &rssi));
    CHECK_TRUE(r == 4, SYSTEM_ERROR_UNKNOWN);
    MacAddress bssid = {};
    CHECK_TRUE(macAddressFromString(&bssid, bssidStr), SYSTEM_ERROR_UNKNOWN);
    *info = WifiNetworkInfo().ssid(ssid).bssid(bssid).channel(channel).rssi(rssi);
    return 0;
}

int Esp32NcpClient::scan(WifiScanCallback callback, void* data) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CWLAP");
    while (resp.hasNextLine()) {
        char ssid[MAX_SSID_SIZE + 1] = {};
        char bssidStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        int security = 0;
        int channel = 0;
        int rssi = 0;
        const int r = CHECK_PARSER(resp.scanf("+CWLAP: %d,\"%32[^\"]\",%d,\"%17[^\"]\",%d", &security, ssid, &rssi,
                bssidStr, &channel));
        CHECK_TRUE(r == 5, SYSTEM_ERROR_UNKNOWN);
        MacAddress bssid = {};
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
    const int r = CHECK_PARSER(resp.scanf("+GETMAC: \"%32[^\"]\"", addrStr));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(macAddressFromString(addr, addrStr), SYSTEM_ERROR_UNKNOWN);
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
    espReset();
    flushInput(serial_.get(), 1000);
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
    flushInput(serial_.get(), 1000);
    parser_.reset();
    parserError_ = 0;
    LOG(TRACE, "NCP initialized");
    ncpState(NcpState::ON);
    return 0;
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
    LOG(TRACE, "NCP connection state changed: %d", (int)connState_);
    connState_ = state;
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpConnectionStateChangedEvent event = {};
        event.type = NcpEvent::CONNECTION_STATE_CHANGED;
        event.state = connState_;
        handler(event, conf_.eventHandlerData());
    }
}

} // particle
