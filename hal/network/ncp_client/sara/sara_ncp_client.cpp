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

#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.client");

#include "sara_ncp_client.h"

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

#include "stream_util.h"

#include "spark_wiring_interrupts.h"
#include "spark_wiring_vector.h"

#include <algorithm>
#include <limits>
#include <lwip/memp.h>
#include "enumclass.h"

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

const auto UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE = 115200;
const auto UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2 = 921600;
const auto UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4 = 460800;
const auto UBLOX_NCP_R4_APP_FW_VERSION_MEMORY_LEAK_ISSUE = 200;
const auto UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MIN = 200;
const auto UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX = 203;

const auto UBLOX_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto UBLOX_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto UBLOX_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;
const auto UBLOX_NCP_PPP_CHANNEL_RX_BUFFER_SIZE = 256;

const auto UBLOX_NCP_AT_CHANNEL = 1;
const auto UBLOX_NCP_PPP_CHANNEL = 2;

const auto UBLOX_NCP_SIM_SELECT_PIN = 23;

const unsigned REGISTRATION_CHECK_INTERVAL = 15 * 1000;
const unsigned REGISTRATION_INTERVENTION_TIMEOUT = 15 * 1000;
const unsigned REGISTRATION_TIMEOUT = 10 * 60 * 1000;
const unsigned REGISTRATION_TWILIO_HOLDOFF_TIMEOUT = 5 * 60 * 1000;

const unsigned CHECK_IMSI_TIMEOUT = 60 * 1000;

const system_tick_t UBLOX_COPS_TIMEOUT = 5 * 60 * 1000;
const system_tick_t UBLOX_CFUN_TIMEOUT = 3 * 60 * 1000;
const system_tick_t UBLOX_CIMI_TIMEOUT = 10 * 1000; // Should be immediate, but have observed 3 seconds occassionally on u-blox and rarely longer times
const system_tick_t UBLOX_UBANDMASK_TIMEOUT = 10 * 1000;

const auto UBLOX_MUXER_T1 = 2530;
const auto UBLOX_MUXER_T2 = 2540;

using LacType = decltype(CellularGlobalIdentity::location_area_code);
using CidType = decltype(CellularGlobalIdentity::cell_id);

const size_t UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD = 512;
const system_tick_t UBLOX_NCP_R4_WINDOW_SIZE_MS = 50;

const int UBLOX_DEFAULT_CID = 1;
const char UBLOX_DEFAULT_PDP_TYPE[] = "IP";


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
    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_USART_SERIAL2,
            UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS));
    CHECK_TRUE(serial, SYSTEM_ERROR_NO_MEMORY);
    // Initialize muxed channel stream
    decltype(muxerAtStream_) muxStrm(new(std::nothrow) decltype(muxerAtStream_)::element_type(&muxer_, UBLOX_NCP_AT_CHANNEL));
    CHECK_TRUE(muxStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxStrm->init(UBLOX_NCP_AT_CHANNEL_RX_BUFFER_SIZE));
    CHECK(initParser(serial.get()));
    decltype(muxerDataStream_) muxDataStrm(new(std::nothrow) decltype(muxerDataStream_)::element_type(&muxer_, UBLOX_NCP_PPP_CHANNEL));
    CHECK_TRUE(muxDataStrm, SYSTEM_ERROR_NO_MEMORY);
    CHECK(muxDataStrm->init(UBLOX_NCP_PPP_CHANNEL_RX_BUFFER_SIZE));
    serial_ = std::move(serial);
    muxerAtStream_ = std::move(muxStrm);
    muxerDataStream_ = std::move(muxDataStrm);
    ncpState_ = NcpState::OFF;
    prevNcpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    regStartTime_ = 0;
    regCheckTime_ = 0;
    imsiCheckTime_ = 0;
    powerOnTime_ = 0;
    registeredTime_ = 0;
    iccidChecked_ = 0;
    isTwilioSuperSIM_ = 0;
    memoryIssuePresent_ = false;
    parserError_ = 0;
    ready_ = false;
    registrationTimeout_ = REGISTRATION_TIMEOUT;
    resetRegistrationState();
    ncpPowerState(modemPowerState() ? NcpPowerState::ON : NcpPowerState::OFF);
    return SYSTEM_ERROR_NONE;
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

    // NOTE: These URC handlers need to take care of both the URCs and direct responses to the commands.
    // See CH28408

    // +CREG: <stat>[,<lac>,<ci>[,<AcTStatus>]]
    CHECK(parser_.addUrcHandler("+CREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
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
        // every time there is a cell tower change in which case also we could see a CREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->csd_.registered()) {   // just registered. Check which IMSI worked.
            self->imsiCheckTime_ = 0;
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
    // n={0,1} +CGREG: <stat>
    // n=2     +CGREG: <stat>[,<lac>,<ci>[,<AcT>,<rac>]]
    CHECK(parser_.addUrcHandler("+CGREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
        unsigned int val[4] = {};
        char atResponse[64] = {};
        // Take a copy of AT response for multi-pass scanning
        CHECK_PARSER_URC(reader->readLine(atResponse, sizeof(atResponse)));
        // Parse response ignoring mode (replicate URC response)
        int r = ::sscanf(atResponse, "+CGREG: %*u,%u,\"%x\",\"%x\",%u,\"%*x\"", &val[0], &val[1], &val[2], &val[3]);
        // Reparse URC as direct response
        if (0 >= r) {
            r = CHECK_PARSER_URC(
                ::sscanf(atResponse, "+CGREG: %u,\"%x\",\"%x\",%u,\"%*x\"", &val[0], &val[1], &val[2], &val[3]));
        }
        CHECK_TRUE(r >= 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

        bool prevRegStatus = self->psd_.registered();
        self->psd_.status(self->psd_.decodeAtStatus(val[0]));
        // Check IMSI only if registered from a non-registered state, to avoid checking IMSI
        // every time there is a cell tower change in which case also we could see a CGREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->psd_.registered()) {   // just registered. Check which IMSI worked.
            self->imsiCheckTime_ = 0;
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
    // +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>[,[<Active_Time>],[<Periodic_TAU>]]]]
    CHECK(parser_.addUrcHandler("+CEREG", [](AtResponseReader* reader, const char* prefix, void* data) -> int {
        const auto self = (SaraNcpClient*)data;
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
            self->imsiCheckTime_ = 0;
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
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return SYSTEM_ERROR_NONE;
    }
    // Power on the modem
    auto r = modemPowerOn();
    if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_ALREADY_EXISTS) {
        return r;
    }
    CHECK(waitReady(r == SYSTEM_ERROR_NONE /* powerOn */));
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::off() {
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

    // Disable voltage translator
    modemSetUartState(false);

    if (!r) {
        LOG(TRACE, "Soft power off modem successfully");
        // WARN: We assume that the modem can turn off itself reliably.
    } else {
        // Power down using hardware
        modemPowerOff();
        // FIXME: There is power leakage still if powering off the modem failed.
    }

    // Disable the UART interface.
    LOG(TRACE, "Deinit modem serial.");
    serial_->on(false);

    ready_ = false;
    ncpState(NcpState::OFF);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::enable() {
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
    muxerDataStream_->enabled(false);
}

NcpState SaraNcpClient::ncpState() {
    return ncpState_;
}

NcpPowerState SaraNcpClient::ncpPowerState() {
    return pwrState_;
}

int SaraNcpClient::disconnect() {
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
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    resetRegistrationState();

    connectionState(NcpConnectionState::DISCONNECTED);
    return SYSTEM_ERROR_NONE;
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
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraNcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

/*
* This is a callback that writes data into muxer channel 2 (data PPP channel)
* Whenever we encounter a large packet, we enforce a certain number of ms to pass before
* transmitting anything else on this channel. After we send large packet, we drop messages(bytes)
* for a certain amount of time defined by UBLOX_NCP_R4_WINDOW_SIZE_MS
*/
int SaraNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    // Just in case perform some state checks to ensure that LwIP PPP implementation
    // does not write into the data channel when it's not supposed do
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_FALSE(muxerDataStream_->enabled(), SYSTEM_ERROR_INVALID_STATE);

    if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
        if ((HAL_Timer_Get_Milli_Seconds() - lastWindow_) >= UBLOX_NCP_R4_WINDOW_SIZE_MS) {
            lastWindow_ = HAL_Timer_Get_Milli_Seconds();
            bytesInWindow_ = 0;
        }

        if (bytesInWindow_ >= UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD) {
            LOG_DEBUG(WARN, "Dropping");
            // Not an error
            return SYSTEM_ERROR_NONE;
        }
    }

    int err = muxer_.writeChannel(UBLOX_NCP_PPP_CHANNEL, data, size);
    if (err == gsm0710::GSM0710_ERROR_FLOW_CONTROL) {
        // Not an error
        LOG_DEBUG(WARN, "Remote side flow control");
        err = 0;
    }
    if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
        bytesInWindow_ += size;
        if (bytesInWindow_ >= UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD) {
            lastWindow_ = HAL_Timer_Get_Milli_Seconds();
        }
    }
    if (err) {
        // Make sure we are going into an error state if muxer for some reason fails
        // to write into the data channel.
        LOG(ERROR, "Failed to write into data channel %d", err);
        disable();
    }
    return err;
}

int SaraNcpClient::dataChannelFlowControl(bool state) {
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    // Just in case
    CHECK_FALSE(muxerDataStream_->enabled(), SYSTEM_ERROR_INVALID_STATE);

    if (state && !inFlowControl_) {
        inFlowControl_ = true;
        CHECK_TRUE(muxer_.suspendChannel(UBLOX_NCP_PPP_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    } else if (!state && inFlowControl_) {
        inFlowControl_ = false;
        lastWindow_ = 0;
        bytesInWindow_ = 0;
        CHECK_TRUE(muxer_.resumeChannel(UBLOX_NCP_PPP_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
    }
    return SYSTEM_ERROR_NONE;
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

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::getIccidImpl(char* buf, size_t size) {
    auto resp = parser_.sendCommand("AT+CCID");
    char iccid[32] = {};
    int r = CHECK_PARSER(resp.scanf("+CCID: %31s", iccid));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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

int SaraNcpClient::getIccid(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    return getIccidImpl(buf, size);
}

int SaraNcpClient::getImei(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("AT+CGSN");
    const size_t n = CHECK_PARSER(resp.readLine(buf, size));
    CHECK_PARSER_OK(resp.readResult());
    return n;
}

int SaraNcpClient::getTxDelayInDataChannel() {
    if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
        return UBLOX_NCP_R4_WINDOW_SIZE_MS * 2;
    }
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::queryAndParseAtCops(CellularSignalQuality* qual) {
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

    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        if (act == particle::to_underlying(CellularAccessTechnology::LTE)) {
            act = particle::to_underlying(CellularAccessTechnology::LTE_CAT_M1);
        }
    }

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

int SaraNcpClient::getCellularGlobalIdentity(CellularGlobalIdentity* cgi) {
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
    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
    } else {
        CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
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

int SaraNcpClient::getSignalQuality(CellularSignalQuality* qual) {
    const NcpClientLock lock(this);
    CHECK_TRUE(connState_ != NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(qual, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(checkParser());
    CHECK(queryAndParseAtCops(qual));

    // Min and max RSRQ index values multiplied by 100
    // Min: -19.5 and max: -3
    const int min_rsrq_mul_by_100 = -1950;
    const int max_rsrq_mul_by_100 = -300;

    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        int rsrp;
        int rsrq_n;
        unsigned long rsrq_f;

        // Default to 255 in case RSRP/Q are not found
        qual->strength(255);
        qual->quality(255);

        // Set UCGED to mode 5 for RSRP/RSRQ values on R410M
        CHECK_PARSER_OK(parser_.execCommand("AT+UCGED=5"));
        auto resp = parser_.sendCommand("AT+UCGED?");

        int val;
        unsigned long val2;
        while (resp.hasNextLine()) {
            char type = 0;
            const int r = CHECK_PARSER(resp.scanf("+RSR%c: %*d,%*d,\"%d.%lu\"", &type, &val, &val2));
            if (r >= 2) {
                if (type == 'P') {
                    rsrp = val;
                    if (rsrp < -140 && rsrp >= -200) {
                        qual->strength(0);
                    } else if (rsrp >= -44 && rsrp <= 0) {
                        qual->strength(97);
                    } else if (rsrp >= -140 && rsrp < -44) {
                        qual->strength(rsrp + 141);
                    } else {
                        // If RSRP is not in the expected range
                        qual->strength(255);
                    }
                } else if (type == 'Q' && r == 3) {
                    rsrq_n = val;
                    rsrq_f = val2;
                    int rsrq_mul_100 = rsrq_n * 100 - rsrq_f;
                    if (rsrq_mul_100 < min_rsrq_mul_by_100 && rsrq_mul_100 >= -2000) {
                        qual->quality(0);
                    } else if (rsrq_mul_100 >= max_rsrq_mul_by_100 && rsrq_mul_100 <=0) {
                        qual->quality(34);
                    } else if (rsrq_mul_100 >= min_rsrq_mul_by_100 && rsrq_mul_100 < max_rsrq_mul_by_100) {
                        qual->quality((rsrq_mul_100 + 2000)/50);
                    } else {
                        // If RSRQ is not in the expected range
                        qual->quality(255);
                    }
                }
            }
        }

        const int r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    } else {
        int rxlev, rxqual;
        auto resp = parser_.sendCommand("AT+CSQ");
        int r = CHECK_PARSER(resp.scanf("+CSQ: %d,%d", &rxlev, &rxqual));
        CHECK_TRUE(r == 2, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

        // Fixup values
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

        switch (qual->strengthUnits()) {
            case CellularStrengthUnits::RXLEV: {
                qual->strength((rxlev != 99) ? (2 * rxlev) : rxlev);
                break;
            }
            case CellularStrengthUnits::RSCP: {
                if (qual->quality() != 255) {
                    // Convert to Ec/Io in dB * 100
                    auto ecio100 = qual->quality() * 50 - 2450;
                    // RSCP = RSSI + Ec/Io
                    // Based on Table 4: Mapping between <signal_power> reported from UE and the RSSI when the P-CPICH= -2 dB (UBX-13002752 - R65)
                    if (rxlev != 99) {
                        auto rssi100 = -11250 + 500 * rxlev / 2;
                        auto rscp = (rssi100 + ecio100) / 100;
                        // Convert from dBm [-121, -25] to RSCP_LEV number, see 3GPP TS 25.133 9.1.1.3
                        if (rscp < -120) {
                            rscp = 0;
                        } else if (rscp >= -25) {
                            rscp = 96;
                        } else if (rscp >= -120 && rscp < -25) {
                            rscp = rscp + 121;
                        } else {
                            rscp = 255;
                        }
                        qual->strength(rscp);
                    } else {
                        qual->strength(255);
                    }
                } else {
                    // Naively map to CESQ range (which is wrong)
                    qual->strength((rxlev != 99) ? (3 + 2 * rxlev) : 255);
                }
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
    }

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::checkParser() {
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

int SaraNcpClient::waitAtResponse(unsigned int timeout, unsigned int period) {
    return waitAtResponse(parser_, timeout, period);
}

int SaraNcpClient::waitAtResponse(AtParser& parser, unsigned int timeout, unsigned int period) {
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

int SaraNcpClient::waitReady(bool powerOn) {
    if (ready_) {
        return SYSTEM_ERROR_NONE;
    }

    ModemState modemState = ModemState::Unknown;

    // Just in case make sure that the voltage translator is on
    CHECK(modemSetUartState(true));
    HAL_Delay_Milliseconds(100);
    CHECK(modemSetUartState(false));
    HAL_Delay_Milliseconds(100);
    CHECK(modemSetUartState(true));

    if (powerOn) {
        LOG_DEBUG(TRACE, "Waiting for modem to be ready from power on");
        ready_ = waitAtResponseFromPowerOn(modemState) == 0;
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
        // start power on timer for memory issue power off delays, assume not registered
        if (powerOn) {
            powerOnTime_ = millis();
            registeredTime_ = 0;
        }
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
    }

    if (!ready_) {
        // Disable voltage translator
        modemSetUartState(false);
        // Hard reset the modem
        modemHardReset(true);
        ncpState(NcpState::OFF);

        return SYSTEM_ERROR_INVALID_STATE;
    }

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::selectSimCard(ModemState& state) {
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
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    }

    if (mode == 0) {
        int p, v;
        auto resp = parser_.sendCommand("AT+UGPIOR=%u", UBLOX_NCP_SIM_SELECT_PIN);
        int r = CHECK_PARSER(resp.scanf("+UGPIO%*[R:] %d%*[ ,]%d", &p, &v));
        if (r == 2 && p == UBLOX_NCP_SIM_SELECT_PIN) {
            value = v;
        }
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
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
                CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                reset = true;
            }
            break;
        }
        case SimType::INTERNAL:
        default: {
            LOG(INFO, "Using internal SIM card");
            if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
                const int internalSimMode = 255;
                if (mode != internalSimMode) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                    reset = true;
                }
            } else {
                const int internalSimMode = 0;
                const int internalSimValue = 1;
                if (mode != internalSimMode || value != internalSimValue) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode, internalSimValue));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                    reset = true;
                }
            }
            break;
        }
    }

    if (reset) {
        if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
            // U201
            const int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=16"));
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
            HAL_Delay_Milliseconds(1000);
        } else {
            // R410
            const int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=15"));
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
            HAL_Delay_Milliseconds(10000);
        }

        CHECK(waitAtResponseFromPowerOn(state));
    }

    // Using numeric CME ERROR codes
    // int r = CHECK_PARSER(parser_.execCommand("AT+CMEE=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    int simState = 0;
    unsigned attempts = 0;
    for (attempts = 0; attempts < 10; attempts++) {
        simState = checkSimCard();
        if (!simState) {
            break;
        }
        HAL_Delay_Milliseconds(1000);
    }

    if (simState != SYSTEM_ERROR_NONE) {
        return simState;
    }

    if (attempts != 0 && ncpId() == PLATFORM_NCP_SARA_R410) {
        // There was an error initializing the SIM
        // This often leads to inability to talk over the data (PPP) muxed channel
        // for some reason. Attempt to cycle the modem through minimal/full functional state.
        // We only do this for R4-based devices, as U2-based modems seem to fail
        // to change baudrate later on for some reason
        CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=0"));
        CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=1"));
    }

    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        int resetCount = 0;
        do {
            // Set UMNOPROF = SIM_SELECT
            auto resp = parser_.sendCommand("AT+UMNOPROF?");
            reset = false;
            int umnoprof = static_cast<int>(UbloxSaraUmnoprof::NONE);
            auto r = CHECK_PARSER(resp.scanf("+UMNOPROF: %d", &umnoprof));
            CHECK_PARSER_OK(resp.readResult());
            if (r == 1 && static_cast<UbloxSaraUmnoprof>(umnoprof) == UbloxSaraUmnoprof::SW_DEFAULT) {
                // Check if we are using a Twilio Super SIM based on ICCID
                char buf[32] = {};
                auto lenCcid = getIccidImpl(buf, sizeof(buf));
                CellularNetworkConfig netConfig;
                CHECK_TRUE(lenCcid > 5, SYSTEM_ERROR_AT_NOT_OK);
                netConfig = networkConfigForIccid(buf, lenCcid);

                // Disconnect before making changes to the UMNOPROF
                auto respCfun = parser_.sendCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN?");
                int cfunVal = -1;
                auto retCfun = CHECK_PARSER(respCfun.scanf("+CFUN: %d", &cfunVal));
                CHECK_PARSER_OK(respCfun.readResult());
                if (retCfun == 1 && cfunVal != 0) {
                    CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=0"));
                }

                // This is a persistent setting
                int umnoprof = static_cast<int>(UbloxSaraUmnoprof::SIM_SELECT);
                if (!strcmp(netConfig.apn(), "super")) { // if Twilio Super SIM
                    umnoprof = static_cast<int>(UbloxSaraUmnoprof::STANDARD_EUROPE);
                }
                parser_.execCommand(1000, "AT+UMNOPROF=%d", umnoprof);
                // Not checking for error since we will reset either way
                reset = true;
            } else if (r == 1 && static_cast<UbloxSaraUmnoprof>(umnoprof) == UbloxSaraUmnoprof::STANDARD_EUROPE) {
                auto respBand = parser_.sendCommand(UBLOX_UBANDMASK_TIMEOUT, "AT+UBANDMASK?");
                uint64_t ubandCatm1 = 0;
                auto retBand = CHECK_PARSER(respBand.scanf("+UBANDMASK: 0,%llu", &ubandCatm1));
                CHECK_PARSER_OK(respBand.readResult());
                if (retBand == 1 && static_cast<uint32_t>(ubandCatm1 & 0xffffffff) != 6170) {
                    // Enable Cat-M1 bands 2,4,5,12 (AT&T), 13 (VZW) = 6170
                    parser_.execCommand(UBLOX_UBANDMASK_TIMEOUT, "AT+UBANDMASK=0,6170");
                    // Not checking for error since we will reset either way
                    reset = true;
                }
            }
            if (reset) {
                CHECK_PARSER_OK(parser_.execCommand("AT+CFUN=15"));
                HAL_Delay_Milliseconds(10000);
                CHECK(waitAtResponseFromPowerOn(state));
            }
        } while (reset && resetCount++ < 4);

        if (resetCount >= 4) {
            return SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED; // we shouldn't have been resetting this many times
        }
    }

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::waitAtResponseFromPowerOn(ModemState& modemState) {
    // Make sure we reset back to using hardware serial port @ default baudrate
    muxer_.stop();

    unsigned int defaultBaudrate = ncpId() != PLATFORM_NCP_SARA_R410 ?
            UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE :
            UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4;

    CHECK(serial_->setBaudRate(defaultBaudrate));
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get(), 1000);
    parser_.reset();

    int r = SYSTEM_ERROR_TIMEOUT;

    if (ncpId() != PLATFORM_NCP_SARA_R410) {
        r = waitAtResponse(20000);
        if (!r) {
            modemState = ModemState::DefaultBaudrate;
        }
    } else {
        r = waitAtResponse(10000);
        if (r) {
            CHECK(serial_->setBaudRate(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE));
            CHECK(initParser(serial_.get()));
            skipAll(serial_.get());
            parser_.reset();
            r = waitAtResponse(5000);
            if (!r) {
                modemState = ModemState::DefaultBaudrate;
            }
        } else {
            modemState = ModemState::RuntimeBaudrate;
        }
    }

    return r;
}

int SaraNcpClient::changeBaudRate(unsigned int baud) {
    auto resp = parser_.sendCommand("AT+IPR=%u", baud);
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    return serial_->setBaudRate(baud);
}

int SaraNcpClient::getAppFirmwareVersion() {
    // ATI9 (get version and app version)
    // example output
    // "08.90,A01.13" G350 (newer)
    // "08.70,A00.02" G350 (older)
    // "L0.0.00.00.05.06,A.02.00" (memory issue)
    // "L0.0.00.00.05.07,A.02.02" (demonstrator)
    // "L0.0.00.00.05.08,A.02.04" (maintenance)
    auto resp = parser_.sendCommand("ATI9");
    int major = 0;
    int minor = 0;
    int r = CHECK_PARSER(resp.scanf("%*[^,],%*[A.]%d.%d", &major, &minor));
    CHECK_TRUE(r == 2, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    CHECK_PARSER_OK(resp.readResult());
    LOG(TRACE, "App firmware: %d", major * 100 + minor);
    return major * 100 + minor;
}

int SaraNcpClient::initReady(ModemState state) {
    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard(state));

    // Make sure flow control is enabled as well
    // NOTE: this should work fine on SARA R4 firmware revisions that don't support it as well
    CHECK_PARSER_OK(parser_.execCommand("AT+IFC=2,2"));
    CHECK(waitAtResponse(10000));

    // Reformat the operator string to be numeric
    // (allows the capture of `mcc` and `mnc`)
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=3,2"));

    // Enable packet domain error reporting
    CHECK_PARSER_OK(parser_.execCommand("AT+CGEREP=1,0"));

    if (state != ModemState::MuxerAtChannel) {
        if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
            // Change the baudrate to 921600
            CHECK(changeBaudRate(UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2));
        } else {
            fwVersion_ = getAppFirmwareVersion();
            if (fwVersion_ > 0) {
                // L0.0.00.00.05.06,A.02.00 has a memory issue
                memoryIssuePresent_ = (fwVersion_ == UBLOX_NCP_R4_APP_FW_VERSION_MEMORY_LEAK_ISSUE);
                // There is a set of other revisions which do not have hardware flow control
                if (!(fwVersion_ >= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MIN &&
                        fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX)) {
                    // Change the baudrate to 460800
                    // NOTE: ignoring AT errors just in case to accommodate for some revisions
                    // potentially not supporting anything other than 115200

                    // XXX: AT+IPR setting is persistent on SARA R4!
                    int r = changeBaudRate(UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4);
                    if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_AT_NOT_OK) {
                        return r;
                    }
                }
            }
        }

        // Check that the modem is responsive at the new baudrate
        skipAll(serial_.get(), 1000);
        CHECK(waitAtResponse(10000));
    }

    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        // Force Cat M1-only mode
        // We may encounter a CME ERROR response with u-blox firmware 05.08,A.02.04 and in that case Cat-M1 mode is
        // already enforced properly based on the UMNOPROF setting.
        auto resp = parser_.sendCommand("AT+URAT?");
        unsigned selectAct = 0, preferAct1 = 0, preferAct2 = 0;
        auto r = resp.scanf("+URAT: %u,%u,%u", &selectAct, &preferAct1, &preferAct2);
        resp.readResult();
        if (r > 0) {
            if (selectAct != 7 || (r >= 2 && preferAct1 != 7) || (r >= 3 && preferAct2 != 7)) { // 7: LTE Cat M1
                // Disconnect before making changes to URAT
                r = CHECK_PARSER(parser_.execCommand("AT+COPS=2,2"));
                if (r == AtResponse::OK) {
                    // This is a persistent setting
                    CHECK_PARSER_OK(parser_.execCommand("AT+URAT=7"));
                }
            }
        }

        // Force eDRX mode to be disabled. AT+CEDRXS=0 doesn't seem disable eDRX completely, so
        // so we're disabling it for each reported RAT individually
        Vector<unsigned> acts;
        resp = parser_.sendCommand("AT+CEDRXS?");
        while (resp.hasNextLine()) {
            unsigned act = 0;
            r = resp.scanf("+CEDRXS: %u", &act);
            if (r == 1) { // Ignore scanf() errors
                CHECK_TRUE(acts.append(act), SYSTEM_ERROR_NO_MEMORY);
            }
        }
        CHECK_PARSER_OK(resp.readResult());
        int lastError = AtResponse::OK;
        for (unsigned act: acts) {
            // This command may fail for unknown reason. eDRX mode is a persistent setting and, eventually,
            // it will get applied for each RAT during subsequent re-initialization attempts
            r = CHECK_PARSER(parser_.execCommand("AT+CEDRXS=3,%u", act)); // 3: Disable the use of eDRX
            if (r != AtResponse::OK) {
                lastError = r;
            }
        }
        CHECK_PARSER_OK(lastError);
        // Force Power Saving mode to be disabled
        //
        // TODO: if we enable this feature in the future add logic to CHECK_PARSER macro(s)
        // to wait longer for device to become active (see MDMParser::_atOk)
        CHECK_PARSER_OK(parser_.execCommand("AT+CPSMS=0"));
    } else {
        // Force Power Saving mode to be disabled
        //
        // TODO: if we enable this feature in the future add logic to CHECK_PARSER macro(s)
        // to wait longer for device to become active (see MDMParser::_atOk)
        CHECK_PARSER_OK(parser_.execCommand("AT+UPSV=0"));
    }

    if (state != ModemState::MuxerAtChannel) {
        // Send AT+CMUX and initialize multiplexer
        r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,,%u,,,,,", UBLOX_NCP_MAX_MUXER_FRAME_SIZE));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

        // Initialize muxer
        CHECK(initMuxer());

        // Start muxer (blocking call)
        CHECK_TRUE(muxer_.start(true) == 0, SYSTEM_ERROR_UNKNOWN);
    }

    NAMED_SCOPE_GUARD(muxerSg, {
        muxer_.stop();
    });

    // Open AT channel and connect it to AT channel stream
    if (muxer_.openChannel(UBLOX_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
        // Failed to open AT channel
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Just in case resume AT channel
    muxer_.resumeChannel(UBLOX_NCP_AT_CHANNEL);

    // Reinitialize parser with a muxer-based stream
    CHECK(initParser(muxerAtStream_.get()));

    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        CHECK(waitAtResponse(10000));
    } else {
        CHECK(waitAtResponse(20000, 5000));
    }
    ncpState(NcpState::ON);
    LOG_DEBUG(TRACE, "Muxer AT channel live");

    muxerSg.dismiss();

    return SYSTEM_ERROR_NONE;
}

bool SaraNcpClient::checkRuntimeStateMuxer(unsigned int baudrate) {
    // Assume we are running at the given baudrate
    CHECK(serial_->setBaudRate(baudrate));

    // Attempt to check whether we are already in muxer state
    // This call will automatically attempt to open/reopen channel 0 (main)
    // It should timeout within T2 * N2, which is 300ms * 3 ~= 1s
    bool stop = true;
    SCOPE_GUARD({
        if (stop) {
            muxer_.stop();
        }
    });
    if (!initMuxer()) {
        muxer_.setAckTimeout(gsm0710::proto::DEFAULT_T1);
        muxer_.setControlResponseTimeout(gsm0710::proto::DEFAULT_T2);
        LOG(TRACE, "Initialized muxer @ %u baudrate", baudrate);
        if (!muxer_.start(true /* initiator */, false /* don't open channel 0 */)) {
            if (!muxer_.forceOpenChannel(0) && !muxer_.ping()) {
                LOG(TRACE, "Resumed muxed session");
                stop = false;
                return true;
            }
        }
    }

    // Remove any potentially received garbage data in the serial stream
    skipAll(serial_.get());
    return false;
}

int SaraNcpClient::checkRuntimeState(ModemState& state) {

    // Assume we are running at the runtime baudrate
    unsigned runtimeBaudrate = ncpId() == PLATFORM_NCP_SARA_R410 ? UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4 :
            UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2;

    // Feeling optimistic, try to see if the muxer is already available
    if (!muxer_.isRunning()) {
        LOG(TRACE, "Muxer is not currently running");
        if (ncpId() != PLATFORM_NCP_SARA_R410) {
            checkRuntimeStateMuxer(runtimeBaudrate);
        } else {
            LOG_DEBUG(TRACE, "Attempt to start/resume muxer at %u baud", runtimeBaudrate);
            bool ret = checkRuntimeStateMuxer(runtimeBaudrate);
            if (!ret) {
                LOG_DEBUG(TRACE, "Attempt to start/resume muxer at %u baud", UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE);
                checkRuntimeStateMuxer(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE);
            }
        }
    }

    if (muxer_.isRunning()) {
        // Muxer is running and channel 0 is already open
        // Open AT channel and connect it to AT channel stream
        if (muxer_.openChannel(UBLOX_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get())) {
            // Failed to open AT channel
            // Force open
            if (muxer_.forceOpenChannel(UBLOX_NCP_AT_CHANNEL)) {
                LOG(TRACE, "Failed to open AT channel");
                return SYSTEM_ERROR_UNKNOWN;
            } else {
                muxer_.forceOpenChannel(UBLOX_NCP_PPP_CHANNEL);
                muxer_.closeChannel(UBLOX_NCP_PPP_CHANNEL);
            }
            muxer_.setChannelDataHandler(UBLOX_NCP_AT_CHANNEL, muxerAtStream_->channelDataCb, muxerAtStream_.get());
        }
        // Attempt to resume AT channel
        CHECK_TRUE(muxer_.resumeChannel(UBLOX_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_UNKNOWN);

        CHECK(initParser(muxerAtStream_.get()));
        skipAll(muxerAtStream_.get());
        parser_.reset();
        if (!waitAtResponse(10000)) {
            // We are in muxed mode already with AT channel open
            state = ModemState::MuxerAtChannel;

            // Restore defaults
            muxer_.setAckTimeout(UBLOX_MUXER_T1);
            muxer_.setControlResponseTimeout(UBLOX_MUXER_T2);

            return SYSTEM_ERROR_NONE;
        }

        // Something went wrong, we are supposed to be in multiplexed mode, however we are not receiving
        // responses from the modem.
        // Stop the muxer and error out
        muxer_.stop();
        LOG(TRACE, "Modem failed to respond on a muxed AT channel after resuming muxed session");
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // We are not in the mulitplexed mode yet
    // Check if the modem is responsive at the runtime baudrate
    CHECK(serial_->setBaudRate(runtimeBaudrate));
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get());
    parser_.reset();
    if (!waitAtResponse(2000)) {
        state = ModemState::RuntimeBaudrate;
        return SYSTEM_ERROR_NONE;
    }

    LOG(TRACE, "Modem is not responsive @ %u baudrate", runtimeBaudrate);

    // The modem is not responsive at the runtime baudrate, check default
    CHECK(serial_->setBaudRate(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get());
    parser_.reset();
    if (!waitAtResponse(5000)) {
        state = ModemState::DefaultBaudrate;
        return SYSTEM_ERROR_NONE;
    }

    LOG(TRACE, "Modem is not responsive @ %u baudrate", UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE);

    state = ModemState::Unknown;
    return SYSTEM_ERROR_UNKNOWN;
}

int SaraNcpClient::initMuxer() {
    // Initialize muxer
    muxer_.setStream(serial_.get());
    muxer_.setMaxFrameSize(UBLOX_NCP_MAX_MUXER_FRAME_SIZE);
    muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD);
    muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED);
    muxer_.setMaxRetransmissions(3);
    muxer_.setAckTimeout(UBLOX_MUXER_T1);
    muxer_.setControlResponseTimeout(UBLOX_MUXER_T2);
    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        muxer_.useMscAsKeepAlive(true);
    }

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::checkSimCard() {
    auto resp = parser_.sendCommand("AT+CPIN?");
    char code[33] = {};
    int r = CHECK_PARSER(resp.scanf("+CPIN: %32[^\n]", code));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    if (!strcmp(code, "READY")) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CCID"));
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int SaraNcpClient::configureApn(const CellularNetworkConfig& conf) {
    netConf_ = conf;
    if (!netConf_.isValid()) {
        // First look for network settings based on ICCID
        char buf[32] = {};
        auto lenCcid = getIccidImpl(buf, sizeof(buf));
        CHECK_TRUE(lenCcid > 5, SYSTEM_ERROR_AT_NOT_OK);
        netConf_ = networkConfigForIccid(buf, lenCcid);

        // If failed above i.e., netConf_ is still not valid, look for network settings based on IMSI
        if (!netConf_.isValid()) {
            memset(buf, 0, sizeof(buf));
            auto resp = parser_.sendCommand(UBLOX_CIMI_TIMEOUT, "AT+CIMI");
            CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
            const int r = CHECK_PARSER(resp.readResult());
            CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
            netConf_ = networkConfigForImsi(buf, strlen(buf));
        }
    }

    auto resp = parser_.sendCommand("AT+CGDCONT=%d,\"%s\",\"%s%s\"",
            UBLOX_DEFAULT_CID, UBLOX_DEFAULT_PDP_TYPE,
            (netConf_.hasUser() && netConf_.hasPassword()) ? "CHAP:" : "",
            netConf_.hasApn() ? netConf_.apn() : "");
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::setRegistrationTimeout(unsigned timeout) {
    registrationTimeout_ = std::max(timeout, REGISTRATION_TIMEOUT);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::registerNet() {
    int r = 0;

    // Set modem full functionality
    r = CHECK_PARSER(parser_.execCommand("AT+CFUN=1,0"));
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    resetRegistrationState();

    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        r = CHECK_PARSER(parser_.execCommand("AT+CREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
        r = CHECK_PARSER(parser_.execCommand("AT+CGREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    } else {
        r = CHECK_PARSER(parser_.execCommand("AT+CEREG=2"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    }

    connectionState(NcpConnectionState::CONNECTING);
    registeredTime_ = 0;

    auto resp = parser_.sendCommand("AT+COPS?");
    int copsState = 2;
    r = CHECK_PARSER(resp.scanf("+COPS: %d", &copsState));
    CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    // NOTE: up to 3 mins (FIXME: there seems to be a bug where this timeout of 3 minutes
    //       is not being respected by u-blox modems.  Setting to 5 for now.)
    if (copsState != 0 && copsState != 1) {
        // Only run AT+COPS=0 if currently de-registered, to avoid PLMN reselection
        r = CHECK_PARSER(parser_.execCommand(UBLOX_COPS_TIMEOUT, "AT+COPS=0,2"));
        // Ignore response code here
        // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    }

    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        r = CHECK_PARSER(parser_.execCommand("AT+CREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
        r = CHECK_PARSER(parser_.execCommand("AT+CGREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    } else {
        r = CHECK_PARSER(parser_.execCommand("AT+CEREG?"));
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    }

    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;
    iccidChecked_ = 0;      // re-check every time we start registration
    isTwilioSuperSIM_ = 0;  //  |
    imsiCheckTime_ = (imsiCheckTime_ == 0) ? 0 : regStartTime_;     // if it is 0, it means the radio registered in the last query

    return SYSTEM_ERROR_NONE;
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

void SaraNcpClient::ncpPowerState(NcpPowerState state) {
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

int SaraNcpClient::enterDataMode() {
    CHECK_TRUE(connectionState() == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
    const NcpClientLock lock(this);
    bool ok = false;
    SCOPE_GUARD({
        muxerDataStream_->enabled(false);
        if (!ok) {
            LOG(ERROR, "Failed to enter data mode");
            muxer_.setChannelDataHandler(UBLOX_NCP_PPP_CHANNEL, nullptr, nullptr);
            // Go into an error state
            disable();
        }
    });

    skipAll(muxerDataStream_.get());
    muxerDataStream_->enabled(true);

    CHECK_TRUE(muxer_.setChannelDataHandler(UBLOX_NCP_PPP_CHANNEL, muxerDataStream_->channelDataCb, muxerDataStream_.get()) == 0, SYSTEM_ERROR_INTERNAL);
    // Send data mode break
    if (ncpId() != PLATFORM_NCP_SARA_R410) {
        const char breakCmd[] = "~+++";
        muxerDataStream_->write(breakCmd, sizeof(breakCmd) - 1);
    } else {
        // SARA R4 does not support ~+++ escape sequence and with +++ and ATH
        // it doesn't actually interrupt an ongoing PPP session an enters some
        // kind of a weird state where it interleaves both PPP and command mode.
        //
        // What we do here instead is send a PPP LCP Termination Request
        const char breakCmd[] = "~\xff}#\xc0!}%}\"} }0User requestS3~";
        muxerDataStream_->write(breakCmd, sizeof(breakCmd) - 1);
    }
    skipAll(muxerDataStream_.get(), 1000);

    // Initialize AT parser
    auto parserConf = AtParserConfig()
            .stream(muxerDataStream_.get())
            .commandTerminator(AtCommandTerminator::CRLF);
    dataParser_.destroy();
    CHECK(dataParser_.init(std::move(parserConf)));

    CHECK(waitAtResponse(dataParser_, 5000));

    // Ignore response
    CHECK(dataParser_.execCommand(20000, "ATH"));

    auto resp = dataParser_.sendCommand(3 * 60 * 1000, "ATD*99***1#");
    if (resp.hasNextLine()) {
        char buf[64] = {};
        CHECK(resp.readLine(buf, sizeof(buf)));
        const char connectResponse[] = "CONNECT";
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


    int r = muxer_.setChannelDataHandler(UBLOX_NCP_PPP_CHANNEL, [](const uint8_t* data, size_t size, void* ctx) -> int {
        auto self = (SaraNcpClient*)ctx;
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
        // Open data channel and resume it just in case
        int r = muxer_.openChannel(UBLOX_NCP_PPP_CHANNEL);
        if (r) {
            LOG(ERROR, "Failed to open data channel");
            ready_ = false;
            state = connState_ = NcpConnectionState::DISCONNECTED;
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
            case UBLOX_NCP_PPP_CHANNEL: {
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

void SaraNcpClient::resetRegistrationState() {
    csd_.reset();
    psd_.reset();
    eps_.reset();
    regStartTime_ = millis();
    regCheckTime_ = regStartTime_;
    imsiCheckTime_ = regStartTime_;
    registrationInterventions_ = 0;
    iccidChecked_ = 0;
    isTwilioSuperSIM_ = 0;
}

void SaraNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if ((csd_.registered() && psd_.registered()) || eps_.registered()) {
            if (memoryIssuePresent_ && connState_ != NcpConnectionState::CONNECTED) {
                registeredTime_ = millis(); // start registered timer for memory issue power off delays
            }
            connectionState(NcpConnectionState::CONNECTED);
        } else if (connState_ == NcpConnectionState::CONNECTED) {
            // FIXME: potentially go back into connecting state only when getting into
            // a 'sticky' non-registered state
            resetRegistrationState();
            connectionState(NcpConnectionState::CONNECTING);
        }
    }
}

int SaraNcpClient::interveneRegistration() {
    CHECK_TRUE(connState_ == NcpConnectionState::CONNECTING, SYSTEM_ERROR_NONE);

    // Check and store if we are using a Twilio SIM based on ICCID
    if (!iccidChecked_) {
        char buf[32] = {};
        auto lenCcid = getIccidImpl(buf, sizeof(buf));
        CellularNetworkConfig netConfig;
        CHECK_TRUE(lenCcid > 5, SYSTEM_ERROR_AT_NOT_OK);
        netConfig = networkConfigForIccid(buf, lenCcid);
        if (!strcmp(netConfig.apn(), "super")) { // if Twilio Super SIM
            isTwilioSuperSIM_ = 1;
        }
        iccidChecked_ = 1; // cache
    }

    if (isTwilioSuperSIM_ && millis() - regStartTime_ <= REGISTRATION_TWILIO_HOLDOFF_TIMEOUT) {
        return 0;
    }

    auto timeout = (registrationInterventions_ + 1) * REGISTRATION_INTERVENTION_TIMEOUT;

    // Intervention to speed up registration or recover in case of failure
    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        // Only attempt intervention when in a sticky state
        // (over intervention interval and multiple URCs with the same state)
        if (csd_.sticky() && csd_.duration() >= timeout) {
            if (csd_.status() == CellularRegistrationStatus::NOT_REGISTERING) {
                LOG(TRACE, "Sticky not registering CSD state for %lu s, PLMN reselection", csd_.duration() / 1000);
                csd_.reset();
                psd_.reset();
                registrationInterventions_++;
                CHECK_PARSER(parser_.execCommand(UBLOX_COPS_TIMEOUT, "AT+COPS=0,2"));
                return 0;
            } else if (csd_.status() == CellularRegistrationStatus::DENIED && psd_.status() == csd_.status()) {
                LOG(TRACE, "Sticky CSD and PSD denied state for %lu s, RF reset", csd_.duration() / 1000);
                csd_.reset();
                psd_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=0"));
                CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=1"));
                return 0;
            }
        }

        if (csd_.registered() && psd_.sticky() && psd_.duration() >= timeout) {
            if (psd_.status() == CellularRegistrationStatus::NOT_REGISTERING) {
                LOG(TRACE, "Sticky not registering PSD state for %lu s, force GPRS attach", psd_.duration() / 1000);
                psd_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand("AT+CGACT?"));
                int r = CHECK_PARSER(parser_.execCommand(3 * 60 * 1000, "AT+CGACT=1"));
                if (r != AtResponse::OK) {
                    csd_.reset();
                    psd_.reset();
                    LOG(TRACE, "GPRS attach failed, try PLMN reselection");
                    CHECK_PARSER(parser_.execCommand(UBLOX_COPS_TIMEOUT, "AT+COPS=0,2"));
                }
            }
        }
    } else {
        if (eps_.sticky() && eps_.duration() >= timeout) {
            if (eps_.status() == CellularRegistrationStatus::NOT_REGISTERING) {
                LOG(TRACE, "Sticky not registering EPS state for %lu s, PLMN reselection", eps_.duration() / 1000);
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER(parser_.execCommand(UBLOX_COPS_TIMEOUT, "AT+COPS=0,2"));
            } else if (eps_.status() == CellularRegistrationStatus::DENIED) {
                LOG(TRACE, "Sticky EPS denied state for %lu s, RF reset", eps_.duration() / 1000);
                eps_.reset();
                registrationInterventions_++;
                CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=0"));
                CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=1"));
            }
        }
    }
    return 0;
}

int SaraNcpClient::checkRunningImsi() {
    // Check current IMSI if registered successfully in which case imsiCheckTime_ will be 0,
    // Else, if not registered, check after CHECK_IMSI_TIMEOUT is expired
    if ((imsiCheckTime_ == 0) || ((connState_ != NcpConnectionState::CONNECTED) &&
            (millis() - imsiCheckTime_ >= CHECK_IMSI_TIMEOUT))) {
        CHECK_PARSER(parser_.execCommand(UBLOX_CIMI_TIMEOUT, "AT+CIMI"));
        imsiCheckTime_ = millis();
    }
    return 0;
}

int SaraNcpClient::processEventsImpl() {
    CHECK_TRUE(ncpState_ == NcpState::ON, SYSTEM_ERROR_INVALID_STATE);
    parser_.processUrc(); // Ignore errors
    checkRegistrationState();
    interveneRegistration();
    checkRunningImsi();
    if (connState_ != NcpConnectionState::CONNECTING ||
            millis() - regCheckTime_ < REGISTRATION_CHECK_INTERVAL) {
        return SYSTEM_ERROR_NONE;
    }
    SCOPE_GUARD({
        regCheckTime_ = millis();
    });

    if (conf_.ncpIdentifier() != PLATFORM_NCP_SARA_R410) {
        CHECK_PARSER(parser_.execCommand("AT+CEER"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    } else {
        CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
    }

    if (connState_ == NcpConnectionState::CONNECTING &&
            millis() - regStartTime_ >= registrationTimeout_) {
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

int SaraNcpClient::modemInit() const {
    hal_gpio_config_t conf = {
        .size = sizeof(conf),
        .version = HAL_GPIO_VERSION,
        .mode = OUTPUT,
        .set_value = true,
        .value = 1
    };

    // Configure PWR_ON and RESET_N pins as OUTPUT and set to high by default
    CHECK(HAL_Pin_Configure(UBPWR, &conf, nullptr));
    CHECK(HAL_Pin_Configure(UBRST, &conf, nullptr));

    // Configure BUFEN as Push-Pull Output and default to 1 (disabled)
    CHECK(HAL_Pin_Configure(BUFEN, &conf, nullptr));

    // Configure VINT as Input for modem power state monitoring
    conf.mode = INPUT;
    CHECK(HAL_Pin_Configure(UBVINT, &conf, nullptr));

    LOG(TRACE, "Modem low level initialization OK");

    return SYSTEM_ERROR_NONE;
}

bool SaraNcpClient::waitModemPowerState(bool onOff, system_tick_t timeout) {
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

int SaraNcpClient::modemPowerOn() {
    if (!serial_->on()) {
        CHECK(serial_->on(true));
    }
    if (!modemPowerState()) {
        ncpPowerState(NcpPowerState::TRANSIENT_ON);

        LOG(TRACE, "Powering modem on");
        // Perform power-on sequence depending on the NCP type
        if (ncpId() != PLATFORM_NCP_SARA_R410) {
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

        // Verify that the module was powered up by checking the VINT pin up to 1 sec
        if (waitModemPowerState(1, 1000)) {
            LOG(TRACE, "Modem powered on");
        } else {
            LOG(ERROR, "Failed to power on modem");
        }
    } else {
        LOG(TRACE, "Modem already on");
        // FIXME:
        return SYSTEM_ERROR_ALREADY_EXISTS;
    }
    CHECK_TRUE(modemPowerState(), SYSTEM_ERROR_TIMEOUT);

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::modemPowerOff() {
    static std::once_flag f;
    std::call_once(f, [this]() {
        if (ncpId() != PLATFORM_NCP_SARA_R410 && modemPowerState()) {
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
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);

        LOG(TRACE, "Powering modem off using hardware control");
        // Important! We need to disable voltage translator here
        // otherwise V_INT will never go low
        modemSetUartState(false);
        // Perform power-off sequence depending on the NCP type
        if (ncpId() != PLATFORM_NCP_SARA_R410) {
            // U201
            // Low pulse 1s+
            HAL_GPIO_Write(UBPWR, 0);
            HAL_Delay_Milliseconds(1500);
            HAL_GPIO_Write(UBPWR, 1);
        } else {
            // If memory issue is present, ensure we don't force a power off too soon
            // to avoid hitting the 124 day memory housekeeping issue
            // TODO: Add ATOK check and AT+CPWROFF command attempt first?
            if (memoryIssuePresent_) {
                waitForPowerOff();
            }
            // R410
            // Low pulse 1.5s+
            HAL_GPIO_Write(UBPWR, 0);
            HAL_Delay_Milliseconds(1600);
            HAL_GPIO_Write(UBPWR, 1);
        }

        // Verify that the module was powered down by checking the VINT pin up to 10 sec
        if (waitModemPowerState(0, 10000)) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem");
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::modemSoftPowerOff() {
    if (modemPowerState()) {
        LOG(TRACE, "Try powering modem off using AT command");
        if (!ready_) {
            LOG(ERROR, "NCP client is not ready");
            return SYSTEM_ERROR_INVALID_STATE;
        }
        int r = CHECK_PARSER(parser_.execCommand("AT+CPWROFF"));
        if (r != AtResponse::OK) {
            LOG(ERROR, "AT+CPWROFF command is not responding");
            return SYSTEM_ERROR_AT_NOT_OK;
        }
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);
        system_tick_t now = HAL_Timer_Get_Milli_Seconds();
        LOG(TRACE, "Waiting the modem to be turned off...");
        // Verify that the module was powered down by checking the VINT pin up to 10 sec
        if (waitModemPowerState(0, 10000)) {
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

int SaraNcpClient::modemHardReset(bool powerOff) {
    const auto pwrState = modemPowerState();
    // We can only reset the modem in the powered state
    if (!pwrState) {
        LOG(ERROR, "Cannot hard reset the modem, it's not on");
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(TRACE, "Hard resetting the modem");
    if (ncpId() != PLATFORM_NCP_SARA_R410) {
        // U201
        // Low pulse for 50ms
        HAL_GPIO_Write(UBRST, 0);
        HAL_Delay_Milliseconds(50);
        HAL_GPIO_Write(UBRST, 1);
        HAL_Delay_Milliseconds(1000);

        // NOTE: powerOff argument is ignored, modem will restart automatically
        // in all cases
    } else {
        // If memory issue is present, ensure we don't force a power off too soon
        // to avoid hitting the 124 day memory housekeeping issue
        if (memoryIssuePresent_) {
            waitForPowerOff();
        }
        // R410
        // Low pulse for 10s
        HAL_GPIO_Write(UBRST, 0);
        HAL_Delay_Milliseconds(10000);
        HAL_GPIO_Write(UBRST, 1);
        // Just in case waiting here for one more second,
        // won't hurt, we've already waited for 10
        HAL_Delay_Milliseconds(1000);
        // IMPORTANT: R4 is powered-off after applying RESET!
        if (!powerOff) {
            LOG(TRACE, "Powering on the modem after the hard reset");
            return modemPowerOn();
        } else {
            ncpPowerState(NcpPowerState::OFF);
            // Disable the UART interface.
            LOG(TRACE, "Deinit modem serial.");
            serial_->on(false);
        }
    }
    return SYSTEM_ERROR_NONE;
}

bool SaraNcpClient::modemPowerState() const {
    return HAL_GPIO_Read(UBVINT);
}

int SaraNcpClient::modemSetUartState(bool state) const {
    LOG(TRACE, "Setting UART voltage translator state %d", state);
    HAL_GPIO_Write(BUFEN, state ? 0 : 1);
    return SYSTEM_ERROR_NONE;
}

void SaraNcpClient::waitForPowerOff() {
    LOG(TRACE, "Modem waiting up to 30s to power off with PWR_UC...");
    system_tick_t now = millis();
    if (powerOnTime_ == 0) {
        powerOnTime_ = now; // fallback to max timeout of 30s to be safe
    }
    do { // check for timeout (VINT == LOW, Powered on 30s ago, Registered 20s ago)
        now = millis();
        // prefer to timeout 20s after registration if we are registered
        if (registeredTime_) {
            if (now - registeredTime_ >= 20000UL) {
                break;
            }
        } else if (powerOnTime_ && now - powerOnTime_ >= 30000UL) {
            break;
        }
        HAL_Delay_Milliseconds(100); // just wait
    } while (modemPowerState());
    registeredTime_ = 0; // reset timers
    powerOnTime_ = 0;
}

} // particle
