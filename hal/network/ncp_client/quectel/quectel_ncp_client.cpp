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

#define NO_STATIC_ASSERT
#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.client");

#include "quectel_ncp_client.h"

#include "at_command.h"
#include "at_response.h"
#include "network/ncp/cellular/network_config_db.h"

#include "serial_stream.h"
#include "check.h"
#include "scope_guard.h"
#include "pinmap_hal.h"

#include "gpio_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "core_hal.h"
#include "deviceid_hal.h"

#include "stream_util.h"

#include "spark_wiring_interrupts.h"
#include "spark_wiring_vector.h"

#include <algorithm>
#include <limits>
#include <lwip/memp.h>

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

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
const auto QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE = 460800;

const auto QUECTEL_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto QUECTEL_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto QUECTEL_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto QUECTEL_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;
const auto QUECTEL_NCP_PPP_CHANNEL_RX_BUFFER_SIZE = 256;

const auto QUECTEL_NCP_AT_CHANNEL = 1;
const auto QUECTEL_NCP_PPP_CHANNEL = 2;

const auto QUECTEL_NCP_SIM_SELECT_PIN = 23;

const unsigned REGISTRATION_CHECK_INTERVAL = 15 * 1000;
const unsigned REGISTRATION_TIMEOUT = 10 * 60 * 1000;
const unsigned REGISTRATION_INTERVENTION_TIMEOUT = 15 * 1000;
const unsigned REGISTRATION_TWILIO_HOLDOFF_TIMEOUT = 5 * 60 * 1000;

const system_tick_t QUECTEL_COPS_TIMEOUT = 3 * 60 * 1000;
const system_tick_t QUECTEL_CFUN_TIMEOUT = 3 * 60 * 1000;

// Undefine hardware version
const auto HW_VERSION_UNDEFINED = 0xFF;

// Hardware version
// V003 - 0x00 (disable hwfc)
// V004 - 0x01 (enable hwfc)
const auto HAL_VERSION_B5SOM_V003 = 0x00;

const auto ICCID_MAX_LENGTH = 20;

using LacType = decltype(CellularGlobalIdentity::location_area_code);
using CidType = decltype(CellularGlobalIdentity::cell_id);

const int QUECTEL_DEFAULT_CID = 1;
const char QUECTEL_DEFAULT_PDP_TYPE[] = "IP";

const int IMSI_MAX_RETRY_CNT = 10;
const int CCID_MAX_RETRY_CNT = 2;

const int DATA_MODE_BREAK_ATTEMPTS = 5;
const int PPP_ECHO_REQUEST_ATTEMPTS = 3;
const int CGDCONT_ATTEMPTS = 5;

const int COPS_MAX_RETRY_CNT = 3;

} // anonymous

QuectelNcpClient::QuectelNcpClient() {
}

QuectelNcpClient::~QuectelNcpClient() {
    destroy();
}

int QuectelNcpClient::init(const NcpClientConfig& conf) {
    modemInit();
    conf_ = static_cast<const CellularNcpClientConfig&>(conf);


    // Initialize serial stream
    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_PLATFORM_CELLULAR_SERIAL, QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE, getDefaultSerialConfig()));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);

    // Initialize muxed channel stream
    decltype(muxerAtStream_) muxStrm(new (std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, QUECTEL_NCP_AT_CHANNEL));
    CHECK_TRUE(muxStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxStrm->init(QUECTEL_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    CHECK(initParser(serial.get()));
    decltype(muxerDataStream_) muxDataStrm(new(std::nothrow) decltype(muxerDataStream_)::element_type(&muxer_, QUECTEL_NCP_PPP_CHANNEL));
    CHECK_TRUE(muxDataStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxDataStrm->init(QUECTEL_NCP_PPP_CHANNEL_RX_BUFFER_SIZE));
    serial_ = std::move(serial);
    muxerAtStream_ = std::move(muxStrm);
    muxerDataStream_ = std::move(muxDataStrm);
    ncpState_ = NcpState::OFF;
    prevNcpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    regStartTime_ = 0;
    regCheckTime_ = 0;
    parserError_ = 0;
    ready_ = false;
    registrationTimeout_ = REGISTRATION_TIMEOUT;
    resetRegistrationState();
    if (modemPowerState()) {
        serial_->on(true);
        ncpPowerState(NcpPowerState::ON);
    } else {
        serial_->on(false);
        ncpPowerState(NcpPowerState::OFF);
    }
    return SYSTEM_ERROR_NONE;
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

    // NOTE: These URC handlers need to take care of both the URCs and direct responses to the commands.
    // See CH28408

    //+CREG: <n>,<stat>[,<lac>,<ci>[,<Act>]]
    //+CREG: <stat>[,<lac>,<ci>[,<Act>]]
    CHECK(parser_.addUrcHandler("+CREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (QuectelNcpClient*)data;
        unsigned int val[4] = {};
        char atResponse[64] = {};
        // Take a copy of AT response for multi-pass scanning
        CHECK_PARSER_URC(reader->readLine(atResponse, sizeof(atResponse)));
        // Parse response ignoring mode (replicate URC response)
        int r = ::sscanf(atResponse, "+CREG: %*u,%u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]);
        // Reparse URC as direct response
        if (0 >= r) {
            r = CHECK_PARSER_URC(
                ::sscanf(atResponse, "+CREG: %u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]));
        }
        CHECK_TRUE(r >= 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

        bool prevRegStatus = self->csd_.registered();
        self->csd_.status(self->csd_.decodeAtStatus(val[0]));
        // Check IMSI only if registered from a non-registered state, to avoid checking IMSI
        // every time there is a cell tower change in which case also we could see a CEREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->csd_.registered()) {   // just registered. Check which IMSI worked.
            self->checkImsi_ = true;
        }
        // self->checkRegistrationState();
        // Cellular Global Identity (partial)
        // Only update if unset
        if (r >= 3) {
            if (self->cgi_.location_area_code == std::numeric_limits<LacType>::max() &&
                    self->cgi_.cell_id == std::numeric_limits<CidType>::max()) {
                self->cgi_.location_area_code = static_cast<LacType>(val[1]);
                self->cgi_.cell_id = static_cast<CidType>(val[2]);
            }
        }
        return SYSTEM_ERROR_NONE;
    }, this));
    //+CGREG: <n>,<stat>[,<lac>,<ci>[,<Act>]]
    //+CGREG: <stat>[,<lac>,<ci>[,<Act>]]
    CHECK(parser_.addUrcHandler("+CGREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (QuectelNcpClient*)data;
        unsigned int val[4] = {};
        char atResponse[64] = {};
        // Take a copy of AT response for multi-pass scanning
        CHECK_PARSER_URC(reader->readLine(atResponse, sizeof(atResponse)));
        // Parse response ignoring mode (replicate URC response)
        int r = ::sscanf(atResponse, "+CGREG: %*u,%u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]);
        // Reparse URC as direct response
        if (0 >= r) {
            r = CHECK_PARSER_URC(
                ::sscanf(atResponse, "+CGREG: %u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]));
        }
        CHECK_TRUE(r >= 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

        bool prevRegStatus = self->psd_.registered();
        self->psd_.status(self->psd_.decodeAtStatus(val[0]));
        // Check IMSI only if registered from a non-registered state, to avoid checking IMSI
        // every time there is a cell tower change in which case also we could see a CGREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->psd_.registered()) {   // just registered. Check which IMSI worked.
            self->checkImsi_ = true;
        }
        // self->checkRegistrationState();
        // Cellular Global Identity (partial)
        if (r >= 3) {
            auto rat = r >= 4 ? static_cast<CellularAccessTechnology>(val[3]) : self->act_;
            switch (rat) {
                case CellularAccessTechnology::GSM:
                case CellularAccessTechnology::GSM_COMPACT:
                case CellularAccessTechnology::UTRAN:
                case CellularAccessTechnology::GSM_EDGE:
                case CellularAccessTechnology::UTRAN_HSDPA:
                case CellularAccessTechnology::UTRAN_HSUPA:
                case CellularAccessTechnology::UTRAN_HSDPA_HSUPA: {
                    self->cgi_.location_area_code = static_cast<LacType>(val[1]);
                    self->cgi_.cell_id = static_cast<CidType>(val[2]);
                    break;
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }, this));
    //+CEREG: <n>,<stat>[,<tac>,<ci>[,<Act>]]
    //+CEREG: <stat>[,<tac>,<ci>[,<Act>]]
    CHECK(parser_.addUrcHandler("+CEREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (QuectelNcpClient*)data;
        unsigned int val[4] = {};
        char atResponse[64] = {};
        // Take a copy of AT response for multi-pass scanning
        CHECK_PARSER_URC(reader->readLine(atResponse, sizeof(atResponse)));
        // Parse response ignoring mode (replicate URC response)
        int r = ::sscanf(atResponse, "+CEREG: %*u,%u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]);
        // Reparse URC as direct response
        if (0 >= r) {
            r = CHECK_PARSER_URC(
                ::sscanf(atResponse, "+CEREG: %u,\"%x\",\"%x\",%u", &val[0], &val[1], &val[2], &val[3]));
        }
        CHECK_TRUE(r >= 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

        bool prevRegStatus = self->eps_.registered();
        self->eps_.status(self->eps_.decodeAtStatus(val[0]));
        // Check IMSI only if registered from a non-registered state, to avoid checking IMSI
        // every time there is a cell tower change in which case also we could see a CEREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->eps_.registered()) {   // just registered. Check which IMSI worked.
            self->checkImsi_ = true;
        }
        // self->checkRegistrationState();
        // Cellular Global Identity (partial)
        if (r >= 3) {
            auto rat = r >= 4 ? static_cast<CellularAccessTechnology>(val[3]) : self->act_;
            switch (rat) {
                case CellularAccessTechnology::LTE:
                case CellularAccessTechnology::LTE_CAT_M1:
                case CellularAccessTechnology::LTE_NB_IOT: {
                    self->cgi_.location_area_code = static_cast<LacType>(val[1]);
                    self->cgi_.cell_id = static_cast<CidType>(val[2]);
                    break;
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }, this));
	// "+QUSIM: 1" URC is seen with USIM update / availability
    CHECK(parser_.addUrcHandler("+QUSIM: 1", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (QuectelNcpClient*)data;
        self->checkImsi_ = true;
        return SYSTEM_ERROR_NONE;
    }, this));
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return SYSTEM_ERROR_NONE;
    }
    // Power on the modem
    LOG(TRACE, "Powering modem on, ncpId: 0x%02x", ncpId());
    auto r = modemPowerOn();
    if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_ALREADY_EXISTS) {
        return r;
    }
    CHECK(waitReady(r == SYSTEM_ERROR_NONE /* powerOn */));
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Try using AT command to turn off the modem first.
    int r = modemSoftPowerOff();

    // Disable ourselves/channel, so that the muxer can potentially stop faster non-gracefully
    serial_->enabled(false);
    muxer_.stop();
    serial_->enabled(true);

    if (!r) {
        LOG(TRACE, "Soft power off modem successfully");
        // WARN: We assume that the modem can turn off itself reliably.
    } else {
        // Power down using hardware
        if (modemPowerOff() != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to turn off the modem.");
        }
        // FIXME: There is power leakage still if powering off the modem failed.
    }

    // Disable the UART interface.
    LOG(TRACE, "Deinit modem serial.");
    serial_->on(false);

    ready_ = false;
    ncpState(NcpState::OFF);
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::enable() {
    const NcpClientLock lock(this);
    if (ncpState_ != NcpState::DISABLED) {
        return SYSTEM_ERROR_NONE;
    }
    serial_->enabled(true);
    muxerAtStream_->enabled(true);
    ncpState_ = prevNcpState_;
    off();
    return SYSTEM_ERROR_NONE;
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
    muxerDataStream_->enabled(false);
}

NcpState QuectelNcpClient::ncpState() {
    return ncpState_;
}

NcpPowerState QuectelNcpClient::ncpPowerState() {
    return pwrState_;
}

int QuectelNcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return SYSTEM_ERROR_NONE;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=0,0"));
    (void)r;
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    resetRegistrationState();

    connectionState(NcpConnectionState::DISCONNECTED);
    return SYSTEM_ERROR_NONE;
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
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int QuectelNcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int QuectelNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    // Just in case perform some state checks to ensure that LwIP PPP implementation
    // does not write into the data channel when it's not supposed do
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(muxerDataStream_->enabled(), SYSTEM_ERROR_INVALID_STATE);

    int err = muxer_.writeChannel(QUECTEL_NCP_PPP_CHANNEL, data, size);
    if (err == gsm0710::GSM0710_ERROR_FLOW_CONTROL) {
        LOG_DEBUG(WARN, "Remote side flow control");
        // Not an error
        err = 0;
    }

    if (err) {
        // Make sure we are going into an error state if muxer for some reason fails
        // to write into the data channel.
        LOG(ERROR, "Failed to write into data channel %d", err);
        disable();
    }

    return err;
}

int QuectelNcpClient::dataChannelFlowControl(bool state) {
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    // Just in case
    CHECK_FALSE(muxerDataStream_->enabled(), SYSTEM_ERROR_INVALID_STATE);

    if (state && !inFlowControl_) {
        inFlowControl_ = true;
        CHECK_TRUE(muxer_.suspendChannel(QUECTEL_NCP_PPP_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    } else if (!state && inFlowControl_) {
        inFlowControl_ = false;
        CHECK_TRUE(muxer_.resumeChannel(QUECTEL_NCP_PPP_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    }
    return SYSTEM_ERROR_NONE;
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

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getIccidImpl(char* buf, size_t size) {
    auto resp = parser_.sendCommand("AT+CCID");
    char iccid[32] = {};
    int r = CHECK_PARSER(resp.scanf("+CCID: %31s", iccid));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    auto iccidLen = strlen(iccid);
    // Strip padding F, as for certain SIMs Quectel's AT+CCID does not strip it on its own
    if (iccidLen == ICCID_MAX_LENGTH && (iccid[iccidLen - 1] == 'F' || iccid[iccidLen - 1] == 'f')) {
        iccid[iccidLen - 1] = '\0';
        --iccidLen;
    }
    size_t n = std::min(iccidLen, size);
    memcpy(buf, iccid, n);
    if (size > 0) {
        if (n == size) {
            --n;
        }
        buf[n] = '\0';
    }
    return n;
}

int QuectelNcpClient::getIccid(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());

    // ICCID command errors if CFUN is 0. Run CFUN=4 before reading ICCID.
    auto respCfun = parser_.sendCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN?");
    int cfunVal = -1;
    auto retCfun = CHECK_PARSER(respCfun.scanf("+CFUN: %d", &cfunVal));
    CHECK_PARSER_OK(respCfun.readResult());
    if (retCfun == 1 && cfunVal == 0) {
        CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=4,0"));
    }

    auto res = getIccidImpl(buf, size);

    // Modify CFUN back to 0 if it was changed previously,
    // as CFUN:0 is needed to prevent long reg problems on certain SIMs
    if (cfunVal == 0) {
        CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=0,0"));
    }

    return res;
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

    // Preserve digit format data
    const int mnc_digits = ::strnlen(mobileNetworkCode, sizeof(mobileNetworkCode));
    CHECK_TRUE((2 == mnc_digits || 3 == mnc_digits), SYSTEM_ERROR_BAD_DATA);
    if (2 == mnc_digits) {
        cgi_.cgi_flags |= CGI_FLAG_TWO_DIGIT_MNC;
    } else {
        cgi_.cgi_flags &= ~CGI_FLAG_TWO_DIGIT_MNC;
    }

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
        case CellularAccessTechnology::LTE_CAT_M1:
        case CellularAccessTechnology::LTE_NB_IOT: {
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

    // FIXME: this is a workaround for CH28408
    CellularSignalQuality qual;
    CHECK(queryAndParseAtCops(&qual));
    CHECK_TRUE(qual.accessTechnology() != CellularAccessTechnology::NONE, SYSTEM_ERROR_INVALID_STATE);
    // Update current RAT
    act_ = qual.accessTechnology();
    // Invalidate LAC and Cell ID
    cgi_.location_area_code = std::numeric_limits<LacType>::max();
    cgi_.cell_id = std::numeric_limits<CidType>::max();
    // Fill in LAC and Cell ID based on current RAT, prefer PSD and EPS
    // fallback to CSD
    CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
    if (isQuecCat1Device()) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
    }

    switch (cgi->version)
    {
    case CGI_VERSION_1:
    default:
    {
        // Confirm user is expecting the correct amount of data
        CHECK_TRUE((cgi->size >= sizeof(cgi_)), SYSTEM_ERROR_INVALID_ARGUMENT);

        *cgi = cgi_;
        cgi->size = sizeof(cgi_);
        cgi->version = CGI_VERSION_1;
        break;
    }
    }

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getSignalQuality(CellularSignalQuality* qual) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ != NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(qual, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(checkParser());
    CHECK(queryAndParseAtCops(qual));

    // Using AT+QCSQ first
    struct RatMap {
        const char* name;
        CellularAccessTechnology rat;
    };

    static const RatMap ratMap[] = {
        {"NOSERVICE", CellularAccessTechnology::NONE},
        {"WCDMA", CellularAccessTechnology::UTRAN},
        {"TDSCDMA", CellularAccessTechnology::UTRAN},
        {"LTE", CellularAccessTechnology::LTE},
        {"CAT-M1", CellularAccessTechnology::LTE_CAT_M1},
        {"eMTC", CellularAccessTechnology::LTE_CAT_M1},
        {"CAT-NB1", CellularAccessTechnology::LTE_NB_IOT},
        {"NBIoT", CellularAccessTechnology::LTE_NB_IOT},
    };

    int vals[5] = {};
    char sysmode[32] = {};

    auto resp = parser_.sendCommand("AT+QCSQ");
    int r = CHECK_PARSER(resp.scanf("+QCSQ: \"%31[^\"]\",%d,%d,%d,%d,%d", sysmode, &vals[0], &vals[1], &vals[2], &vals[3], &vals[4]));
    CHECK_TRUE(r >= 2, SYSTEM_ERROR_BAD_DATA);
    int qcsqVals = r;
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    bool qcsqOk = false;
    for (const auto& v: ratMap) {
        if (!strcmp(sysmode, v.name)) {
            switch (v.rat) {
                case CellularAccessTechnology::NONE: {
                    qcsqOk = true;
                    break;
                }
                case CellularAccessTechnology::UTRAN:
                case CellularAccessTechnology::UTRAN_HSDPA:
                case CellularAccessTechnology::UTRAN_HSUPA:
                case CellularAccessTechnology::UTRAN_HSDPA_HSUPA: {
                    if (qcsqVals >= 4) {
                        auto rscp = vals[1];
                        auto ecno = vals[2];
                        if (rscp < -120) {
                            rscp = 0;
                        } else if (rscp >= -25) {
                            rscp = 96;
                        } else if (rscp >= -120 && rscp < -25) {
                            rscp = rscp + 121;
                        } else {
                            rscp = 255;
                        }

                        if (ecno < -24) {
                            ecno = 0;
                        } else if (ecno >= 0) {
                            ecno = 49;
                        } else if (ecno >= -24 && ecno < 0) {
                            ecno = (ecno * 100 + 2450) / 50;
                        } else {
                            ecno = 255;
                        }

                        qual->accessTechnology(v.rat);
                        qual->strength(rscp);
                        qual->quality(ecno);
                        qcsqOk = true;
                    }
                    break;
                }
                case CellularAccessTechnology::LTE:
                case CellularAccessTechnology::LTE_CAT_M1:
                case CellularAccessTechnology::LTE_NB_IOT: {
                    if (qcsqVals >= 5) {
                        const int min_rsrq_mul_by_100 = -1950;
                        const int max_rsrq_mul_by_100 = -300;

                        auto rsrp = vals[1];
                        auto rsrq_mul_100 = vals[3] * 100;

                        qual->accessTechnology(v.rat);

                        if (rsrp < -140 && rsrp >= -200) {
                            qual->strength(0);
                        } else if (rsrp >= -44 && rsrp < 0) {
                            qual->strength(97);
                        } else if (rsrp >= -140 && rsrp < -44) {
                            qual->strength(rsrp + 141);
                        } else {
                            // If RSRP is not in the expected range
                            qual->strength(255);
                        }

                        if (rsrq_mul_100 < min_rsrq_mul_by_100 && rsrq_mul_100 >= -2000) {
                            qual->quality(0);
                        } else if (rsrq_mul_100 >= max_rsrq_mul_by_100 && rsrq_mul_100 < 0) {
                            qual->quality(34);
                        } else if (rsrq_mul_100 >= min_rsrq_mul_by_100 && rsrq_mul_100 < max_rsrq_mul_by_100) {
                            qual->quality((rsrq_mul_100 + 2000) / 50);
                        } else {
                            // If RSRQ is not in the expected range
                            qual->quality(255);
                        }
                        qcsqOk = true;
                    }
                    break;
                }
            }

            break;
        }
    }

    if (qcsqOk) {
        return SYSTEM_ERROR_NONE;
    }

    // Fall-back to AT+CSQ on errors or 2G as AT+QCSQ does not provide quality for GSM
    int rxlev, rxqual;
    resp = parser_.sendCommand("AT+CSQ");
    r = CHECK_PARSER(resp.scanf("+CSQ: %d,%d", &rxlev, &rxqual));
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

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::setRegistrationTimeout(unsigned timeout) {
    registrationTimeout_ = std::max(timeout, REGISTRATION_TIMEOUT);
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::getTxDelayInDataChannel() {
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::checkParser() {
    CHECK_TRUE(pwrState_ == NcpPowerState::ON, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(ncpState_ == NcpState::ON, SYSTEM_ERROR_INVALID_STATE);
    if (ready_ && parserError_ != 0) {
        const int r = parser_.execCommand(1000, "AT");
        if (r == AtResponse::OK) {
            parserError_ = 0;
        } else {
            ready_ = false;
        }
    }
    CHECK(waitReady());
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::waitAtResponse(unsigned int timeout, unsigned int period) {
    return waitAtResponse(parser_, timeout, period);
}

int QuectelNcpClient::waitAtResponse(AtParser& parser, unsigned int timeout, unsigned int period) {
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser.execCommand(period, "AT");
        if (r < 0 && r != SYSTEM_ERROR_TIMEOUT) {
            return r;
        }
        if (r == AtResponse::OK) {
            return SYSTEM_ERROR_NONE;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= timeout) {
            break;
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

int QuectelNcpClient::waitReady(bool powerOn) {
    if (ready_) {
        return SYSTEM_ERROR_NONE;
    }

    ModemState modemState = ModemState::Unknown;

    if (powerOn) {
        LOG_DEBUG(TRACE, "Waiting for modem to be ready from power on");
        muxer_.stop();
        CHECK(serial_->setBaudRate(QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE));
        CHECK(initParser(serial_.get()));
        skipAll(serial_.get(), 1000);
        parser_.reset();
        ready_ = waitAtResponse(20000) == 0;
        modemState = ModemState::DefaultBaudrate;
    } else if (ncpState() == NcpState::OFF) {
        LOG_DEBUG(TRACE, "Waiting for modem to be ready from current unknown state");
        ready_ = checkRuntimeState(modemState) == 0;
        if (ready_) {
            LOG_DEBUG(TRACE, "Runtime state %d", (int)modemState);
        }
    } else {
        // Most likely we just had a parser error, try to get a response from the modem as-is
        LOG_DEBUG(TRACE, "Waiting for modem to be ready after parser error");
        auto stream = parser_.config().stream();
        if (stream) {
            skipAll(stream, 1000);
            parser_.reset();
            ready_ = waitAtResponse(2000) == 0;
            if (muxer_.isRunning()) {
                modemState = ModemState::MuxerAtChannel;
            } else {
                // FIXME: we need to be able to tell which baudrate we are currently running at
                modemState = ModemState::DefaultBaudrate;
            }
        }
    }

    if (ready_) {
        skipAll(serial_.get());
        parser_.reset();
        parserError_ = 0;
        LOG(TRACE, "NCP ready to accept AT commands");

        auto r = initReady(modemState);
        if (r != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to perform early initialization");
            ready_ = false;
        }
    } else {
        LOG(ERROR, "No response from NCP");
        // Make sure the muxer is stopped
        muxer_.stop();
    }

    if (!ready_) {
        // Hard reset the modem
        modemHardReset(true);
        ncpState(NcpState::OFF);

        return SYSTEM_ERROR_INVALID_STATE;
    }

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::checkNetConfForImsi() {
    int imsiCount = 0;
    char buf[32] = {};
    do {
        auto resp = parser_.sendCommand("AT+CIMI");
        memset(buf, 0, sizeof(buf));
        int imsiLength = 0;
        if (resp.hasNextLine()) {
            imsiLength = CHECK(resp.readLine(buf, sizeof(buf)));
        }
        const int r = CHECK_PARSER(resp.readResult());
        if (r == AtResponse::OK && imsiLength > 0) {
            netConf_ = networkConfigForImsi(buf, imsiLength);
            return SYSTEM_ERROR_NONE;
        } else if (imsiCount >= IMSI_MAX_RETRY_CNT) {
            // if max retries are exhausted
            return SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED;
        }
        ++imsiCount;
        HAL_Delay_Milliseconds(100*imsiCount);
    } while (imsiCount < IMSI_MAX_RETRY_CNT);
    return SYSTEM_ERROR_TIMEOUT;
}

int QuectelNcpClient::selectSimCard() {
    // Using numeric CME ERROR codes
    // int r = CHECK_PARSER(parser_.execCommand("AT+CMEE=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    int simState = 0;
    unsigned attempts = 0;
    for (attempts = 0; attempts < 10; attempts++) {
        simState = checkSimCard();
        if (!simState) {
            break;
        }
        HAL_Delay_Milliseconds(1000);
    }

    if (attempts != 0) {
        // There was an error initializing the SIM
        // We've seen issues on uBlox-based devices, as a precation, we'll cycle
        // the modem here through minimal/full functional state.
        CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=0,0"));
        CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=1,0"));
    }
    return simState;
}

int QuectelNcpClient::changeBaudRate(unsigned int baud) {
    auto resp = parser_.sendCommand("AT+IPR=%u", baud);
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return serial_->setBaudRate(baud);
}

bool QuectelNcpClient::isQuecCatM1Device() {
    int ncp_id = ncpId();
    return (ncp_id == PLATFORM_NCP_QUECTEL_BG96 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M5 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M6 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M1 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_MF ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG77) ;
}

bool QuectelNcpClient::isQuecCat1Device() {
    int ncp_id = ncpId();
    return (ncp_id == PLATFORM_NCP_QUECTEL_EG91_E ||
            ncp_id == PLATFORM_NCP_QUECTEL_EG91_NA ||
            ncp_id == PLATFORM_NCP_QUECTEL_EG91_EX ||
            ncp_id == PLATFORM_NCP_QUECTEL_EG91_NAX);
}

bool QuectelNcpClient::isQuecCatNBxDevice() {
    int ncp_id = ncpId();
    return (ncp_id == PLATFORM_NCP_QUECTEL_BG95_M5 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M6 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_MF ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG77) ;
}

bool QuectelNcpClient::isQuecBG95xDevice() {
    int ncp_id = ncpId();
    return (ncp_id == PLATFORM_NCP_QUECTEL_BG95_M5 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M6 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_M1 ||
            ncp_id == PLATFORM_NCP_QUECTEL_BG95_MF) ;
}

int QuectelNcpClient::initReady(ModemState state) {
    // Set modem full functionality
    int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=1,0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    if (state != ModemState::MuxerAtChannel) {
        // Enable flow control and change to runtime baudrate
#if PLATFORM_ID == PLATFORM_B5SOM
        uint32_t hwVersion = HW_VERSION_UNDEFINED;
        auto ret = hal_get_device_hw_version(&hwVersion, nullptr);
        if (ret == SYSTEM_ERROR_NONE && hwVersion == HAL_VERSION_B5SOM_V003) {
            CHECK_PARSER_OK(parser_.execCommand("AT+IFC=0,0"));
        } else
#endif // PLATFORM_ID == PLATFORM_B5SOM
        {
            CHECK_PARSER_OK(parser_.execCommand("AT+IFC=2,2"));
            CHECK(waitAtResponse(10000));
        }
        auto runtimeBaudrate = QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE;
        CHECK(changeBaudRate(runtimeBaudrate));
        // Check that the modem is responsive at the new baudrate
        skipAll(serial_.get(), 1000);
        CHECK(waitAtResponse(10000));

        // Select either internal or external SIM card slot depending on the configuration
        CHECK(selectSimCard());

        // Just in case disconnect
        // int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
        // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

        if (isQuecCatM1Device()) {
            // Force eDRX mode to be disabled.
            CHECK_PARSER(parser_.execCommand("AT+CEDRXS=0"));

            // Disable Power Saving Mode
            CHECK_PARSER(parser_.execCommand("AT+CPSMS=0"));
        }

        // Select (U)SIM card in slot 1, EG91 has two SIM card slots
        if (isQuecCat1Device()) {
            CHECK_PARSER(parser_.execCommand("AT+QDSIM=0"));
        }

        // Send AT+CMUX and initialize multiplexer
        int portspeed;
        switch (runtimeBaudrate) {
            case 9600: portspeed = 1; break;
            case 19200: portspeed = 2; break;
            case 38400: portspeed = 3; break;
            case 57600: portspeed = 4; break;
            case 115200: portspeed = 5; break;
            case 230400: portspeed = 6; break;
            case 460800: portspeed = 7; break;
            default:
                return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,%d,%u,,,,,", portspeed, QUECTEL_NCP_MAX_MUXER_FRAME_SIZE));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

        // Initialize muxer
        CHECK(initMuxer());
    }

    NAMED_SCOPE_GUARD(muxerSg, {
        muxer_.stop();
    });

    // Open AT channel and connect it to AT channel stream
    if (muxer_.openChannel(QUECTEL_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
        // Failed to open AT channel
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Just in case resume AT channel
    CHECK_TRUE(muxer_.resumeChannel(QUECTEL_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_UNKNOWN);

    // Reinitialize parser with a muxer-based stream
    CHECK(initParser(muxerAtStream_.get()));
    CHECK(waitAtResponse(20000, 5000));

    ncpState(NcpState::ON);
    LOG_DEBUG(TRACE, "Muxer AT channel live");

    muxerSg.dismiss();

    // Make sure that we receive URCs only on AT channel, ignore response code
    // just in case
    CHECK_PARSER(parser_.execCommand("AT+QCFG=\"cmux/urcport\",1"));

    // Enable packet domain error reporting
    // Ignore error responses, this command is known to fail sometimes
    CHECK_PARSER(parser_.execCommand("AT+CGEREP=1,0"));

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::checkRuntimeState(ModemState& state) {
    // Assume we are running at the runtime baudrate
    // NOTE: disabling hardware flow control here as BG96-based Trackers are known
    // to latch CTS sometimes on warm boot
    // Sending some data without flow control allows us to get out of that state
    CHECK(serial_->setConfig(getDefaultSerialConfig() & ~(SERIAL_FLOW_CONTROL_RTS_CTS), QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE));

    // Essentially we are generating empty 07.10 frames here
    // This is done so that we can complete an ongoing frame transfer that was aborted e.g.
    // due to a reset.
    const char basicFlag = static_cast<char>(gsm0710::proto::BASIC_FLAG);
    for (int i = 0; i < QUECTEL_NCP_MAX_MUXER_FRAME_SIZE; i++) {
        CHECK(serial_->waitEvent(Stream::WRITABLE, 1000));
        CHECK(serial_->write(&basicFlag, sizeof(basicFlag)));
        skipAll(serial_.get());
    }

    // Reinitialize with flow control enabled (if needed for a particular hw revision)
    CHECK(serial_->setConfig(getDefaultSerialConfig()));

    // Feeling optimistic, try to see if the muxer is already available
    if (!muxer_.isRunning()) {
        LOG_DEBUG(TRACE, "Muxer is not currently running");
        // Attempt to check whether we are already in muxer state
        // This call will automatically attempt to open/reopen channel 0 (main)
        // It should timeout within T2 * N2, which is 300ms * 3 ~= 1s for Quectel devices
        if (initMuxer() != SYSTEM_ERROR_NONE) {
            muxer_.stop();
        } else {
            LOG_DEBUG(TRACE, "Resumed muxed session");
        }
    }

    if (muxer_.isRunning()) {
        // Muxer is running and channel 0 is already open
        // Open AT channel and connect it to AT channel stream
        if (muxer_.openChannel(QUECTEL_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
            // Failed to open AT channel
            LOG_DEBUG(TRACE, "Failed to open AT channel");
            return SYSTEM_ERROR_UNKNOWN;
        }
        // Just in case resume AT channel
        CHECK_TRUE(muxer_.resumeChannel(QUECTEL_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_UNKNOWN);

        CHECK(initParser(muxerAtStream_.get()));
        skipAll(muxerAtStream_.get());
        parser_.reset();
        if (!waitAtResponse(10000)) {
            // We are in muxed mode already with AT channel open
            state = ModemState::MuxerAtChannel;
            return SYSTEM_ERROR_NONE;
        }

        // Something went wrong, we are supposed to be in multiplexed mode, however we are not receiving
        // responses from the modem.
        // Stop the muxer and error out
        muxer_.stop();
        LOG_DEBUG(TRACE, "Modem failed to respond on a muxed AT channel after resuming muxed session");
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // We are not in the mulitplexed mode yet
    // Check if the modem is responsive at the runtime baudrate
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get());
    parser_.reset();
    if (!waitAtResponse(1000)) {
        state = ModemState::RuntimeBaudrate;
        return SYSTEM_ERROR_NONE;
    }

    LOG_DEBUG(TRACE, "Modem is not responsive @ %u baudrate", QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE);

    // The modem is not responsive at the runtime baudrate, check default
    CHECK(serial_->setBaudRate(QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get());
    parser_.reset();
    if (!waitAtResponse(20000)) {
        state = ModemState::DefaultBaudrate;
        return SYSTEM_ERROR_NONE;
    }

    LOG_DEBUG(TRACE, "Modem is not responsive @ %u baudrate", QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE);

    state = ModemState::Unknown;
    return SYSTEM_ERROR_UNKNOWN;
}

int QuectelNcpClient::initMuxer() {
    muxer_.setStream(serial_.get());
    // [TS 27.010] N1 in the AT+CMUX command defines the maximum number of octets that that may be contained in
    // an information field. It does not include octets added for transparency purposes.
    // XXX: We are not using any transparency (error-correction) mechanisms, so the maximum frame size should
    // be limited to N1, however on some modems (e.g. BG95) this is not respected and the modem sends frames a
    // bit larger than N1. To account for that we are initializing the muxer with a larger internal frame buffer,
    // but negotiating a smaller maximum frame size (N1) with the modem in AT+CMUX
    muxer_.setMaxFrameSize(QUECTEL_NCP_MAX_MUXER_FRAME_SIZE + 64);
    muxer_.setKeepAlivePeriod(QUECTEL_NCP_KEEPALIVE_PERIOD);
    muxer_.setKeepAliveMaxMissed(QUECTEL_NCP_KEEPALIVE_MAX_MISSED);
    muxer_.setMaxRetransmissions(gsm0710::proto::DEFAULT_N2);
    muxer_.setAckTimeout(gsm0710::proto::DEFAULT_T1);
    muxer_.setControlResponseTimeout(gsm0710::proto::DEFAULT_T2);

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    // Start muxer (blocking call)
    CHECK_TRUE(muxer_.start(true) == 0, SYSTEM_ERROR_UNKNOWN);

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::checkSimCard() {
    auto resp = parser_.sendCommand("AT+CPIN?");
    char code[33] = {};
    int r = CHECK_PARSER(resp.scanf("+CPIN: %32[^\n]", code));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    if (!strcmp(code, "READY")) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CCID"));
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int QuectelNcpClient::configureApn(const CellularNetworkConfig& conf) {
    // IMPORTANT: Set modem full functionality!
    // Otherwise we won't be able to query ICCID/IMSI
    CHECK_PARSER_OK(parser_.execCommand("AT+CFUN=1,0"));

    netConf_ = conf;
    if (!netConf_.isValid()) {
        // First look for network settings based on ICCID
        char buf[32] = {};
        int ccidCount = 0;
        do {
            memset(buf, 0, sizeof(buf));
            auto lenCcid = getIccidImpl(buf, sizeof(buf));
            if (lenCcid > 5) {
                netConf_ = networkConfigForIccid(buf, lenCcid);
                break;
            }
        } while (++ccidCount < CCID_MAX_RETRY_CNT);

        // If failed above i.e., netConf_ is still not valid, look for network settings based on IMSI
        if (!netConf_.isValid()) {
            CHECK(checkNetConfForImsi());
        }
    }
    // XXX: we've seen CGDCONT fail on cold boot, retrying here a few times
    for (int i = 0; i < CGDCONT_ATTEMPTS; i++) {
        // FIXME: for now IPv4 context only
        auto resp = parser_.sendCommand("AT+CGDCONT=%d,\"%s\",\"%s\"",
                QUECTEL_DEFAULT_CID, QUECTEL_DEFAULT_PDP_TYPE,
                netConf_.hasApn() ? netConf_.apn() : "");
        const int r = CHECK_PARSER(resp.readResult());
        if (r == AtResponse::OK) {
            return SYSTEM_ERROR_NONE;
        }
        HAL_Delay_Milliseconds(200);
    }
    return SYSTEM_ERROR_AT_NOT_OK;
}

int QuectelNcpClient::registerNet() {
    int r = 0;
    // Set modem full functionality
    r = CHECK_PARSER(parser_.execCommand("AT+CFUN=1,0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    resetRegistrationState();

    if (isQuecCat1Device() || ncpId() == PLATFORM_NCP_QUECTEL_BG95_M5) {
        // Register GPRS, LET, NB-IOT network
        r = CHECK_PARSER(parser_.execCommand("AT+CREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        r = CHECK_PARSER(parser_.execCommand("AT+CGREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }
    r = CHECK_PARSER(parser_.execCommand("AT+CEREG=2"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    connectionState(NcpConnectionState::CONNECTING);

    // EG91NA can get stuck in an COPS? ERROR init loop, retry 2 times.
    int copsCount = 0;
    int copsState = 2;
    char copsResponse[64] = {};
    do {
        auto resp = parser_.sendCommand("AT+COPS?");
        if (resp.hasNextLine()) {
            CHECK_PARSER(resp.readLine(copsResponse, sizeof(copsResponse)));
            CHECK_PARSER(::sscanf(copsResponse, "+COPS: %d", &copsState));
        }
        r = CHECK_PARSER(resp.readResult());
        if (r == AtResponse::OK) {
            break;
        } else if (copsCount >= COPS_MAX_RETRY_CNT) {
            // if max retries are exhausted
            return SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED;
        }
        ++copsCount;
        HAL_Delay_Milliseconds(2000 * copsCount);
    } while (copsCount < COPS_MAX_RETRY_CNT);

    if (copsState != 0 && copsState != 1) {
        // Only run AT+COPS=0 if currently de-registered, to avoid PLMN reselection
        // NOTE: up to 3 mins
        r = CHECK_PARSER(parser_.execCommand(QUECTEL_COPS_TIMEOUT, "AT+COPS=0,2"));
        // Ignore response code here
        // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    }

    if (isQuecCatM1Device()) {
        if (ncpId() == PLATFORM_NCP_QUECTEL_BG96 || ncpId() == PLATFORM_NCP_QUECTEL_BG95_M5) {
            // NOTE: BG96 supports 2G fallback which we disable explicitly so that a 10W power supply is not required
            // Configure RATs to be searched
            // Set to scan LTE only if not already set, take effect immediately
            auto respNwMode = parser_.sendCommand("AT+QCFG=\"nwscanmode\"");
            int nwScanMode = -1;
            r = CHECK_PARSER(respNwMode.scanf("+QCFG: \"nwscanmode\",%d", &nwScanMode));
            CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
            r = CHECK_PARSER(respNwMode.readResult());
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        #if PLATFORM_ID == PLATFORM_MSOM
            if (nwScanMode != 0) {
                CHECK_PARSER(parser_.execCommand("AT+QCFG=\"nwscanmode\",0,1")); // AUTO
            }

            if (ncpId() == PLATFORM_NCP_QUECTEL_BG95_M5) {
                auto respNwScanSeq = parser_.sendCommand("AT+QCFG=\"nwscanseq\"");
                int nwScanSeq = -1;
                r = CHECK_PARSER(respNwScanSeq.scanf("+QCFG: \"nwscanseq\",%d", &nwScanSeq));
                CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
                r = CHECK_PARSER(respNwScanSeq.readResult());
                CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
                if (nwScanSeq != 201) { // i.e. 0201
                    CHECK_PARSER(parser_.execCommand("AT+QCFG=\"nwscanseq\",0201,1")); // LTE 02, then GSM 01
                }
            }
        #else 
            if (nwScanMode != 3) {
                CHECK_PARSER(parser_.execCommand("AT+QCFG=\"nwscanmode\",3,1"));
            }
        #endif
        }

        if (isQuecCatNBxDevice()) {
            // Configure Network Category to be searched
            // Set to use LTE Cat-M1 ONLY if not already set, take effect immediately
            auto respOpMode = parser_.sendCommand("AT+QCFG=\"iotopmode\"") ;

            int iotOpMode = -1;
            r = CHECK_PARSER(respOpMode.scanf("+QCFG: \"iotopmode\",%d", &iotOpMode));

            CHECK_TRUE(r == 1, SYSTEM_ERROR_UNKNOWN);
            r = CHECK_PARSER(respOpMode.readResult());
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
            if (iotOpMode != 0) {
                CHECK_PARSER(parser_.execCommand("AT+QCFG=\"iotopmode\",0,1"));
            }
        }
    }
    // Check GSM, GPRS, and LTE network registration status
    CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
    if (isQuecCat1Device() || ncpId() == PLATFORM_NCP_QUECTEL_BG95_M5) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    }

    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;

    return SYSTEM_ERROR_NONE;
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

void QuectelNcpClient::ncpPowerState(NcpPowerState state) {
    if (pwrState_ == state) {
        return;
    }
    pwrState_ = state;
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpPowerStateChangedEvent event = {};
        event.type = NcpEvent::POWER_STATE_CHANGED;
        event.state = pwrState_;
        handler(event, conf_.eventHandlerData());
    }
}

int QuectelNcpClient::enterDataMode() {
    CHECK_TRUE(connectionState() == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    const NcpClientLock lock(this);
    bool ok = false;
    SCOPE_GUARD({
        muxerDataStream_->enabled(false);
        if (!ok) {
            LOG(ERROR, "Failed to enter data mode");
            muxer_.setChannelDataHandler(QUECTEL_NCP_PPP_CHANNEL, nullptr, nullptr);
            // Go into an error state
            disable();
        }
    });

    skipAll(muxerDataStream_.get());
    muxerDataStream_->enabled(true);

    CHECK_TRUE(muxer_.setChannelDataHandler(QUECTEL_NCP_PPP_CHANNEL, muxerDataStream_->channelDataCb, muxerDataStream_.get()) == 0, SYSTEM_ERROR_INTERNAL);
    // From Quectel AT commands manual:
    // To prevent the +++ escape sequence from being misinterpreted as data, the following sequence should be followed:
    // 1) Do not input any character within 1s before inputting +++.
    // 2) Input +++ within 1s, and no other characters can be inputted during the time.
    // 3) Do not input any character within 1s after +++ has been inputted.
    // 4) Switch to command mode; otherwise return to step 1)
    // NOTE: we are ignoring step 1 and are acting more optimistic on the first attempt
    // Subsequent attempts to exit data mode follow this step.
    bool responsive = false;

    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(muxerDataStream_.get())
            .commandTerminator(AtCommandTerminator::CRLF);
    dataParser_.destroy();
    CHECK(dataParser_.init(std::move(parserConf)));

    for (int i = 0; i < DATA_MODE_BREAK_ATTEMPTS; i++) {
        // Also try toggling DTR as +++ may sometimes fail.
        exitDataModeWithDtr();
        // Send data mode break
        const char breakCmd[] = "+++";
        muxerDataStream_->write(breakCmd, sizeof(breakCmd) - 1);
        skipAll(muxerDataStream_.get(), 1000);

        dataParser_.reset();
        responsive = waitAtResponse(dataParser_, 1000, 500) == 0;
        if (responsive) {
            break;
        }
    }
    CHECK_TRUE(responsive, SYSTEM_ERROR_TIMEOUT);

    const char connectResponse[] = "CONNECT";

    char buf[64] = {};
    auto resp = dataParser_.sendCommand(2000, "ATO");
    if (resp.hasNextLine()) {
        CHECK(resp.readLine(buf, sizeof(buf)));
        if (!strncmp(buf, connectResponse, sizeof(connectResponse) - 1)) {
            // IMPORTANT: ATO does not work all the time and we resume into
            // some broken state where the modem does not reply to our PPP ConfReqs.
            // What's worse is that ATH doesn't seem to work either, so we cannot
            // disconnect an ongoing PPP session. Toggling DTR doesn't work either.
            // As a workaround send a PPP LCP Echo Request, wait until we get a reply
            // or at least something similar to it.
            // If we don't get a reply, consider we are in a broken state. To get
            // out of it we are going to close and re-open the data muxer channel.
            dataParser_.reset();
            skipAll(muxerDataStream_.get());
            int flagsSeen = 0;
            for (int i = 0; i < PPP_ECHO_REQUEST_ATTEMPTS; i++) {
                // I wish this was done in PPP layer
                const char lcpEchoRequest[] = "~\xff\x03\xc0!\t\x00\x00\x08\x00\x00\x00\x00\xbbn~";
                muxerDataStream_->write(lcpEchoRequest, sizeof(lcpEchoRequest) - 1);
                const int r = muxerDataStream_->waitEvent(Stream::READABLE, 1000);
                if (r == Stream::READABLE) {
                    // Do a quick check that we've seen something similar to a PPP frame: ~<...>~
                    const auto size = CHECK(muxerDataStream_->read(buf, sizeof(buf)));
                    for (int i = 0; i < size; i++) {
                        if (buf[i] == lcpEchoRequest[0]) {
                            flagsSeen++;
                        }
                    }
                    if (flagsSeen >= 2) {
                        ok = true;
                        break;
                    }
                } else if (r != SYSTEM_ERROR_TIMEOUT) {
                    return r;
                }
            }
            if (!ok) {
                LOG(WARN, "Resumed into a broken PPP session");
                // FIXME: Previously we tried closing and re-opening the data channel,
                // which does help in some cases, but fails in some others.
                // To safe, trigger a modem reset
                return SYSTEM_ERROR_INVALID_STATE;
            }
        }
    } else {
        const int r = CHECK(resp.readResult());
        if (r != AtResponse::NO_CARRIER) {
            return SYSTEM_ERROR_UNKNOWN;
        }
    }

    if (!ok) {
        auto resp = dataParser_.sendCommand(3 * 60 * 1000, "ATD*99***1#");
        if (resp.hasNextLine()) {
            memset(buf, 0, sizeof(buf));
            CHECK(resp.readLine(buf, sizeof(buf)));
            if (strncmp(buf, connectResponse, sizeof(connectResponse) - 1)) {
                return SYSTEM_ERROR_UNKNOWN;
            }
        } else {
            // We've got a final response code
            CHECK(resp.readResult());
            // This is not a critical failure
            ok = true;
            return SYSTEM_ERROR_NOT_ALLOWED;
        }
    }

    int r = muxer_.setChannelDataHandler(QUECTEL_NCP_PPP_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
        auto self = (QuectelNcpClient*)ctx;
        const auto handler = self->conf_.dataHandler();
        if (handler) {
            handler(0, data, size, self->conf_.dataHandlerData());
        }
        return SYSTEM_ERROR_NONE;
    }, this);

    if (!r) {
        muxerDataStream_->enabled(false);
        // Just in case resume
        inFlowControl_ = true;
        r = dataChannelFlowControl(false);
    }
    if (!r) {
        ok = true;
    }
    return r;
}

int QuectelNcpClient::dataModeError(int error) {
    return 0;
}

int QuectelNcpClient::getMtu() {
    return 0;
}

int QuectelNcpClient::urcs(bool enable) {
    if (enable) {
        CHECK_TRUE(muxer_.resumeChannel(QUECTEL_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    } else {
        CHECK_TRUE(muxer_.suspendChannel(QUECTEL_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    }
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::startNcpFwUpdate(bool update) {
    return 0;
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
        inFlowControl_ = false;
        // Open data channel and resume it just in case
        int r = muxer_.openChannel(QUECTEL_NCP_PPP_CHANNEL);
        if (r) {
            LOG(ERROR, "Failed to open data channel");
            ready_ = false;
            state = connState_ = NcpConnectionState::DISCONNECTED;
            return;
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

    // Ignore state changes unless we are in an ON state
    if (self->ncpState() != NcpState::ON) {
        return SYSTEM_ERROR_NONE;
    }
    // This callback is executed from the multiplexer thread, not safe to use the lock here
    // because it might get called while blocked inside some muxer function

    // Also please note that connectionState() should never be called with the CONNECTED state
    // from this callback.

    // We are only interested in Closed state
    if (newState == decltype(muxer_)::ChannelState::Closed) {
        switch (channel) {
            case 0: {
                // Muxer stopped
                self->disable();
                break;
            }
            case QUECTEL_NCP_PPP_CHANNEL: {
                // PPP channel closed
                if (self->connState_ != NcpConnectionState::DISCONNECTED) {
                    // It should be safe to notify the PPP netif/client about a change of state
                    // here exactly because the muxer channel is closed and there is no
                    // chance for a deadlock.
                    self->connectionState(NcpConnectionState::CONNECTING);
                }
                break;
            }
        }
    }

    return SYSTEM_ERROR_NONE;
}

void QuectelNcpClient::resetRegistrationState() {
    csd_.reset();
    psd_.reset();
    eps_.reset();
    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;
    registrationInterventions_ = 0;
}

void QuectelNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if (psd_.registered() || eps_.registered()) {
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            // FIXME: potentially go back into connecting state only when getting into
            // a 'sticky' non-registered state
            resetRegistrationState();
            connectionState(NcpConnectionState::CONNECTING);
        }
    }
}

int QuectelNcpClient::interveneRegistration() {
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTING, SYSTEM_ERROR_NONE);

    if (netConf_.netProv() == CellularNetworkProvider::TWILIO && millis() - regStartTime_ <= REGISTRATION_TWILIO_HOLDOFF_TIMEOUT) {
        return 0;
    }
    auto timeout = (registrationInterventions_ + 1) * REGISTRATION_INTERVENTION_TIMEOUT;

    // Intervention to speed up registration or recover in case of failure
    if (!isQuecCatM1Device()) {
        if (eps_.sticky() && eps_.duration() >= timeout) {
            if (eps_.status() == CellularRegistrationStatus::NOT_REGISTERING && csd_.status() == eps_.status()) {
                LOG(TRACE, "Sticky not registering state for %lu s, PLMN reselection", eps_.duration() / 1000);
                csd_.reset();
                psd_.reset();
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER(parser_.execCommand(QUECTEL_COPS_TIMEOUT, "AT+COPS=0,2"));
                return 0;
            } else if (eps_.status() == CellularRegistrationStatus::DENIED && csd_.status() == eps_.status()) {
                LOG(TRACE, "Sticky denied state for %lu s, RF reset", eps_.duration() / 1000);
                csd_.reset();
                psd_.reset();
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=0,0"));
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=1,0"));
                return 0;
            }
        }

        if (csd_.sticky() && csd_.duration() >= timeout ) {
            if (csd_.status() == CellularRegistrationStatus::DENIED && psd_.status() == csd_.status()) {
                LOG(TRACE, "Sticky CSD and PSD denied state for %lu s, RF reset", csd_.duration() / 1000);
                csd_.reset();
                psd_.reset();
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=0,0"));
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=1,0"));
                return 0;
            }
        }

        if (csd_.registered() && psd_.sticky() && psd_.duration() >= timeout) {
            if (psd_.status() == CellularRegistrationStatus::NOT_REGISTERING && eps_.status() == psd_.status()) {
                LOG(TRACE, "Sticky not registering PSD state for %lu s, force GPRS attach", psd_.duration() / 1000);
                psd_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand("AT+CGACT?"));
                int r = CHECK_PARSER(parser_.execCommand(3 * 60 * 1000, "AT+CGACT=1"));
                if (r != AtResponse::OK) {
                    csd_.reset();
                    psd_.reset();
                    eps_.reset();
                    LOG(TRACE, "GPRS attach failed, try PLMN reselection");
                    CHECK_PARSER(parser_.execCommand(QUECTEL_COPS_TIMEOUT, "AT+COPS=0,2"));
                }
            }
        }
    } else {
        if (eps_.sticky() && eps_.duration() >= timeout) {
            if (eps_.status() == CellularRegistrationStatus::NOT_REGISTERING) {
                LOG(TRACE, "Sticky not registering EPS state for %lu s, PLMN reselection", eps_.duration() / 1000);
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER(parser_.execCommand(QUECTEL_COPS_TIMEOUT, "AT+COPS=0,2"));
                CHECK_PARSER(parser_.execCommand("AT+QCFG=\"nwscanmode\",3,1"));
                CHECK_PARSER(parser_.execCommand("AT+QCFG=\"iotopmode\",0,1"));
            } else if (eps_.status() == CellularRegistrationStatus::DENIED) {
                LOG(TRACE, "Sticky EPS denied state for %lu s, RF reset", eps_.duration() / 1000);
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=0,0"));
                CHECK_PARSER_OK(parser_.execCommand(QUECTEL_CFUN_TIMEOUT, "AT+CFUN=1,0"));
            }
        }
    }
    return 0;
}


int QuectelNcpClient::checkRunningImsi() {
    // Check current IMSI
    if (checkImsi_) {
        checkImsi_ = false;
        CHECK_PARSER(parser_.execCommand("AT+CIMI"));
    }
    return 0;
}

int QuectelNcpClient::processEventsImpl() {
    CHECK_TRUE(ncpState_ == NcpState::ON, SYSTEM_ERROR_INVALID_STATE);
    parser_.processUrc(); // Ignore errors
    checkRegistrationState();
    interveneRegistration();
    checkRunningImsi();
    if (connState_ != NcpConnectionState::CONNECTING || millis() - regCheckTime_ < REGISTRATION_CHECK_INTERVAL) {
        return SYSTEM_ERROR_NONE;
    }
    SCOPE_GUARD({ regCheckTime_ = millis(); });

    // Log extended errors for modems that support AT+CEER
    if (isQuecCat1Device()) {
        CHECK_PARSER(parser_.execCommand("AT+CEER"));
    }

    // Log error level (will be factory default)
    // CHECK_PARSER(parser_.execCommand("AT+CMEE?"));

    // Check GSM, GPRS, and LTE network registration status
    CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
    if (isQuecCat1Device() || ncpId() == PLATFORM_NCP_QUECTEL_BG95_M5) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    }

    // Check the signal seen by the module while trying to register
    // Do not need to check for an OK, as this is just for debugging purpose
    CHECK_PARSER(parser_.execCommand("AT+QCSQ"));

    if (connState_ == NcpConnectionState::CONNECTING && millis() - regStartTime_ >= registrationTimeout_) {
        LOG(WARN, "Resetting the modem due to the network registration timeout");
        // We are going into an OFF state immediately before stopping the muxer
        // otherwise the muxer channel state callback will disable us unnecessarily.
        ncpState(NcpState::OFF);
        muxer_.stop();
        int rv = modemPowerOff();
        if (rv != 0) {
            modemHardReset(true);
        }
        return SYSTEM_ERROR_TIMEOUT;
    }

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemInit() const {
    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = HAL_GPIO_VERSION,
        .mode = INPUT_PULLUP,
        .set_value = false,
        .value = 0,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };
    // Configure VINT as Input for modem power state monitoring
    // NOTE: The BGVINT pin is inverted
    conf.mode = INPUT_PULLUP;
    CHECK(hal_gpio_configure(BGVINT, &conf, nullptr));

    if (modemPowerState() == 1) {
        LOG(TRACE, "Startup: Modem is on");
    } else {
        LOG(TRACE, "Startup: Modem is off");
    }

    // Configure PWR_ON and RESET_N pins as OUTPUT and set to high by default
    conf.mode = OUTPUT;
    conf.set_value = true;
    // NOTE: The BGPWR/BGRST pins are inverted
    conf.value = 0;
    CHECK(hal_gpio_configure(BGPWR, &conf, nullptr));
    CHECK(hal_gpio_configure(BGRST, &conf, nullptr));

    // DTR=0: normal mode, DTR=1: sleep mode
    // NOTE: The BGDTR pins is inverted
    conf.value = 1;
    CHECK(hal_gpio_configure(BGDTR, &conf, nullptr));

    LOG(TRACE, "Modem low level initialization OK");

    return SYSTEM_ERROR_NONE;
}

bool QuectelNcpClient::waitModemPowerState(bool onOff, system_tick_t timeout) {
    system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    while (HAL_Timer_Get_Milli_Seconds() - now < timeout) {
        if (modemPowerState() == onOff) {
            if (onOff) {
                ncpPowerState(NcpPowerState::ON);
            } else {
                ncpPowerState(NcpPowerState::OFF);
            }
            return true;
        }
        HAL_Delay_Milliseconds(5);
    }
    return false;
}

int QuectelNcpClient::modemPowerOn() {
    if (!serial_->on()) {
        CHECK(serial_->on(true));
    }
    if (!modemPowerState()) {
        ncpPowerState(NcpPowerState::TRANSIENT_ON);

        // EG91, BG96, BG95, BG77: Power on, power on pulse 500ms - 1000ms
        // XXX: Keeping it halfway between 500ms and 650ms, to avoid the power OFF timing of >=650ms
        // NOTE: The BGPWR pin is inverted
        hal_gpio_write(BGPWR, 1);
        HAL_Delay_Milliseconds(575);
        hal_gpio_write(BGPWR, 0);

        // After power on the device, we can't assume the device is ready for operation:
        // BG95/BG77: status pin ready requires >= 2.1s, uart ready requires >= 2.5s
        // BG96: status pin ready requires >= 4.8s, uart ready requires >= 4.9s
        // EG91: status pin ready requires >= 10s, uart ready requires >= 12s
        // Wait 3 seconds longer than minimum spec
        system_tick_t powerOffWaitMs = 15000; // EG91
        if (isQuecBG95xDevice() || ncpId() == PLATFORM_NCP_QUECTEL_BG77) {
            powerOffWaitMs = 5500; // BG95/BG77
        } else if (ncpId() == PLATFORM_NCP_QUECTEL_BG96) {
            powerOffWaitMs = 7900; // BG96
        }
        if (waitModemPowerState(1, powerOffWaitMs)) {
            LOG(TRACE, "Modem powered on");
        } else {
            LOG(ERROR, "Failed to power on modem, try hard reset");
            modemHardReset(false);
        }
    } else {
        LOG(TRACE, "Modem already on");
        // FIXME:
        return SYSTEM_ERROR_ALREADY_EXISTS;
    }
    CHECK_TRUE(modemPowerState(), SYSTEM_ERROR_TIMEOUT);

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemPowerOff() {
    if (modemPowerState()) {
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);

        LOG(TRACE, "Powering modem off");

        // Power off, power off pulse >= 650ms
        // NOTE: The BGRST pin is inverted
        hal_gpio_write(BGPWR, 1);
        HAL_Delay_Milliseconds(650);
        hal_gpio_write(BGPWR, 0);

        // Verify that the module was powered down by checking the status pin (BGVINT)
        // BG95/BG77: >=1.3s
        // BG96: >=2s
        // EG91: >=30s
        // Wait 3 seconds longer than minimum spec
        system_tick_t powerOnWaitMs = 33000; // EG91
        if (isQuecBG95xDevice() || ncpId() == PLATFORM_NCP_QUECTEL_BG77) {
            powerOnWaitMs = 4300; // BG95/BG77
        } else if (ncpId() == PLATFORM_NCP_QUECTEL_BG96) {
            powerOnWaitMs = 5000; // BG96
        }
        if (waitModemPowerState(0, powerOnWaitMs)) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem, try hard reset");
            modemHardReset(true);
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemSoftPowerOff() {
    if (modemPowerState()) {
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);

        LOG(TRACE, "Try powering modem off using AT command");
        if (!ready_) {
            LOG(ERROR, "NCP client is not ready");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        // Re-try in case that the modem is just powered up and refuse to execute the power down command.
        int r = AtResponse::ERROR;
        for (uint8_t i = 0; i < 6; i++) {
            r = CHECK_PARSER(parser_.execCommand("AT+QPOWD"));
            if (r == AtResponse::OK) {
                break;
            }
            HAL_Delay_Milliseconds(500);
        }
        if (r != AtResponse::OK) {
            LOG(ERROR, "AT+QPOWD command is not responding");
            return SYSTEM_ERROR_AT_NOT_OK;
        }
        system_tick_t now = HAL_Timer_Get_Milli_Seconds();
        LOG(TRACE, "Waiting the modem to be turned off...");
        // Verify that the module was powered down by checking the VINT pin up to 30 sec
        if (waitModemPowerState(0, 30000)) {
            LOG(TRACE, "It takes %d ms to power off the modem.", HAL_Timer_Get_Milli_Seconds() - now);
        } else {
            LOG(ERROR, "Failed to power off modem using AT command");
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemHardReset(bool powerOff) {
    LOG(TRACE, "Hard resetting the modem");

    if (isQuecBG95xDevice() || ncpId() == PLATFORM_NCP_QUECTEL_BG77) {
        // BG95/BG77 reset, 2s <= reset pulse <= 3.8s
        // NOTE: The BGRST pin is inverted
        hal_gpio_write(BGRST, 1);
        HAL_Delay_Milliseconds(2900);
        hal_gpio_write(BGRST, 0);
    } else {
        // BG96/EG91 reset, 150ms <= reset pulse <= 460ms
        // NOTE: The BGRST pin is inverted
        hal_gpio_write(BGRST, 1);
        HAL_Delay_Milliseconds(400);
        hal_gpio_write(BGRST, 0);
    }

    LOG(TRACE, "Waiting the modem to restart.");
    if (waitModemPowerState(1, 30000)) {
        LOG(TRACE, "Successfully reset the modem.");
    } else {
        LOG(ERROR, "Failed to reset the modem.");
    }

    // The modem restarts automatically after hard reset.
    if (powerOff) {
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);

        // Need to delay at least 5s, otherwise, the modem cannot be powered off.
        HAL_Delay_Milliseconds(5000);
        LOG(TRACE, "Powering modem off");
        // Power off, power off pulse >= 650ms
        // NOTE: The BGPWR pin is inverted
        hal_gpio_write(BGPWR, 1);
        HAL_Delay_Milliseconds(650);
        hal_gpio_write(BGPWR, 0);

        // Verify that the module was powered down by checking the status pin (BGVINT)
        // BG96: >=2s
        // EG91: >=30s
        if (waitModemPowerState(0, 30000)) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem");
        }
    }

    return SYSTEM_ERROR_NONE;
}

bool QuectelNcpClient::modemPowerState() const {
    // LOG(TRACE, "BGVINT: %d", hal_gpio_read(BGVINT));
    // NOTE: The BGVINT pin is inverted
    return !hal_gpio_read(BGVINT);
}

uint32_t QuectelNcpClient::getDefaultSerialConfig() const {
    uint32_t sconf = SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS;

    // Our first board reversed RTS and CTS pin, we gave them the hwVersion 0x00,
    // Disabling hwfc should not only depend on hwVersion but also platform
#if PLATFORM_ID == PLATFORM_B5SOM
    uint32_t hwVersion = HW_VERSION_UNDEFINED;
    auto ret = hal_get_device_hw_version(&hwVersion, nullptr);
    if (ret == SYSTEM_ERROR_NONE && hwVersion == HAL_VERSION_B5SOM_V003) {
        sconf = SERIAL_8N1;
        LOG(TRACE, "Disable Hardware Flow control!");
    }
#endif // PLATFORM_ID == PLATFORM_B5SOM

    return sconf;
}

void QuectelNcpClient::exitDataModeWithDtr() const {
    hal_gpio_write(BGDTR, 0);
    HAL_Delay_Milliseconds(1);
    hal_gpio_write(BGDTR, 1);
    HAL_Delay_Milliseconds(1);
}

} // namespace particle
