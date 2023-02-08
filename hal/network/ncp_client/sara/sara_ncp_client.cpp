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

#include "system_cache.h"

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
const auto UBLOX_NCP_R4_APP_FW_VERSION_LATEST_02B_01 = 204;
const auto UBLOX_NCP_R4_APP_FW_VERSION_0512 = 219;

const auto UBLOX_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto UBLOX_NCP_KEEPALIVE_PERIOD_DEFAULT = 5000; // milliseconds
const auto UBLOX_NCP_KEEPALIVE_PERIOD_DISABLED = 0; // disables muxer keep alive
const auto UBLOX_NCP_KEEPALIVE_PERIOD_R510 = UBLOX_NCP_KEEPALIVE_PERIOD_DISABLED;
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

const size_t UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD = 500;
const system_tick_t UBLOX_NCP_R4_WINDOW_SIZE_MS = 50;
const int UBLOX_NCP_R4_NO_HW_FLOW_CONTROL_MTU = 990;

const int UBLOX_DEFAULT_CID = 1;
const char UBLOX_DEFAULT_PDP_TYPE[] = "IP";

const int IMSI_MAX_RETRY_CNT = 5;
const int CCID_MAX_RETRY_CNT = 2;

const int UUFWINSTALL_COMPLETE = 128;
const int UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_TIMEOUT = 300000;
const int UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_PERIOD = 10000;

const unsigned CHECK_SIM_CARD_INTERVAL = 1000;
const unsigned CHECK_SIM_CARD_ATTEMPTS = 10;

const uint32_t SYSTEM_CACHE_OPERATION_MODE_VALID = 0x97c10000;
const uint32_t SYSTEM_CACHE_OPERATION_MODE_MASK = 0x0000ffff;

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
    std::unique_ptr<SerialStream> serial(new (std::nothrow) SerialStream(HAL_PLATFORM_CELLULAR_SERIAL,
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
    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        memoryIssuePresent_ = true; // default to safe state until we determine modem firmware version
        oldFirmwarePresent_ = true; // default to safe state until we determine modem firmware version
    } else {
        memoryIssuePresent_ = false;
        oldFirmwarePresent_ = false;
    }
    parserError_ = 0;
    ready_ = false;
    firmwareUpdateR510_ = false;
    firmwareInstallRespCodeR510_ = -1;
    lastFirmwareInstallRespCodeR510_ = -1;
    waitReadyRetries_ = 0;
    sleepNoPPPWrite_ = false;
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
        unsigned int uint_val[3] = {};
        int int_val = 0;
        char atResponse[64] = {};
        // Take a copy of AT response for multi-pass scanning
        CHECK_PARSER_URC(reader->readLine(atResponse, sizeof(atResponse)));
        // Parse response ignoring mode (replicate URC response)
        int r = ::sscanf(atResponse, "+CEREG: %*u,%u,\"%x\",\"%x\",%d", &uint_val[0], &uint_val[1], &uint_val[2], &int_val);
        // Reparse URC as direct response
        if (0 >= r) {
            r = CHECK_PARSER_URC(
                ::sscanf(atResponse, "+CEREG: %u,\"%x\",\"%x\",%d", &uint_val[0], &uint_val[1], &uint_val[2], &int_val));
        }
        CHECK_TRUE(r >= 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

        bool prevRegStatus = self->eps_.registered();
        self->eps_.status(self->eps_.decodeAtStatus(uint_val[0]));
        // Check IMSI only if registered from a non-registered state, to avoid checking IMSI
        // every time there is a cell tower change in which case also we could see a CEREG: {1 or 5} URC
        // TODO: Do this only for Twilio
        if (!prevRegStatus && self->eps_.registered()) {   // just registered. Check which IMSI worked.
            self->imsiCheckTime_ = 0;
        }
        // self->checkRegistrationState();
        // Cellular Global Identity (partial)
        if (r >= 3) {
            // Check for erroneous R510 AcT value of -1, change to R510 specific Cat-M1 value of 7
            if (r >= 4 && int_val == -1) {
                int_val = 7;
            }
            auto rat = r >= 4 ? static_cast<CellularAccessTechnology>(int_val) : self->act_;
            switch (rat) {
                case CellularAccessTechnology::LTE:
                case CellularAccessTechnology::LTE_CAT_M1:
                case CellularAccessTechnology::LTE_NB_IOT: {
                    self->cgi_.location_area_code = static_cast<LacType>(uint_val[1]);
                    self->cgi_.cell_id = static_cast<CidType>(uint_val[2]);
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
    // XXX: Collect the existing power state as modemPowerOn() updates the NcpState to ON.
    // This is helpful for R5 modems after hard resets, when entering this function.
    auto powerState = ncpPowerState();
    // Power on the modem
    LOG(TRACE, "Powering modem on, ncpId: 0x%02x", ncpId());
    auto r = modemPowerOn();
    if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_ALREADY_EXISTS) {
        return r;
    }

    bool powerOn = (r == SYSTEM_ERROR_NONE);
    if (ncpId() == PLATFORM_NCP_SARA_R510) {
        powerOn |= (powerState == NcpPowerState::TRANSIENT_ON);
    }
    CHECK(waitReady(powerOn));
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

    SCOPE_GUARD({
        resetRegistrationState();
        connectionState(NcpConnectionState::DISCONNECTED);
    });

    // If we disconnect due to the AT interface being dead, a forced check
    // is required because ready_ is true and parserError_ is 0.
    const int r = parser_.execCommand(1000, "AT");
    if (r != AtResponse::OK) {
        parserError_ = r;
    }
    CHECK(checkParser());
    CHECK_PARSER(setModuleFunctionality(CellularFunctionality::MINIMUM));

    return SYSTEM_ERROR_NONE;
}

NcpConnectionState SaraNcpClient::connectionState() {
    return connState_;
}

int SaraNcpClient::getFirmwareVersionString(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());
    auto resp = parser_.sendCommand("ATI9");
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
            const int windowCount = ((HAL_Timer_Get_Milli_Seconds() - lastWindow_) / UBLOX_NCP_R4_WINDOW_SIZE_MS);
            lastWindow_ += UBLOX_NCP_R4_WINDOW_SIZE_MS * windowCount;
            bytesInWindow_ = std::max(0, (int)bytesInWindow_ - (int)UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD * windowCount);
            if (bytesInWindow_ == 0) {
                lastWindow_ = HAL_Timer_Get_Milli_Seconds();
            }
        }

        if (bytesInWindow_ > 0 && (bytesInWindow_ + size) >= UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD) {
            LOG_DEBUG(WARN, "Dropping");
            // Not an error
            return SYSTEM_ERROR_NONE;
        }
    }

    int err = gsm0710::GSM0710_ERROR_NONE;
    if (!sleepNoPPPWrite_) {
        err = muxer_.writeChannel(UBLOX_NCP_PPP_CHANNEL, data, size);
    }
    if (err == gsm0710::GSM0710_ERROR_FLOW_CONTROL) {
        // Not an error
        LOG_DEBUG(WARN, "Remote side flow control");
        err = 0;
    }
    if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
        bytesInWindow_ += size;
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

    // ICCID command errors if CFUN is 0. Run CFUN=4 before reading ICCID.
    int cfunVal = -1;
    cfunVal = getModuleFunctionality();
    if (cfunVal == CellularFunctionality::MINIMUM) {
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::AIRPLANE));
    }

    auto res = getIccidImpl(buf, size);

    // Modify CFUN back to 0 if it was changed previously,
    // as CFUN:0 is needed to prevent long reg problems on certain SIMs
    if (cfunVal == CellularFunctionality::MINIMUM) {
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM));
    }

    return res;
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

    if (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) {
        if (act == particle::to_underlying(CellularAccessTechnology::LTE) || 
            (act == -1 && ncpId() == PLATFORM_NCP_SARA_R510)) {
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
    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
    } else {
        CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
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

    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        // Min and max RSRQ index values multiplied by 100
        // Min: -19.5 and max: -3
        const int min_rsrq_mul_by_100 = -1950;
        const int max_rsrq_mul_by_100 = -300;

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

    } else if (ncpId() == PLATFORM_NCP_SARA_R510) {
        unsigned rsrp = 0;
        int rsrq = 0;
        // Default to 255 in case RSRP/Q are not found
        qual->strength(255);
        qual->quality(255);
        // > AT+UCGED?
        // < +UCGED: 2
        // < 6,2,fff <mcc>,fff <mnc>
        // < 65535,255,255,255,ffff,0000000,65535,00000000,ffff,ff,255 <rsrp>,255 <rsrq>,255,1,255,255,255,255,255,0,255,255,0
        // < OK
        auto resp = parser_.sendCommand("AT+UCGED?");
        while (resp.hasNextLine()) {
            auto r = resp.scanf("%*d,%*d,%*d,%*d,%*x,%*x,%*d,%*x,%*x,%*x,%u,%d,%*[^\n]", &rsrp, &rsrq);
            if (r == 2) {
                if (rsrp <= 255) {
                    qual->strength(rsrp);
                }
                if (rsrq <= 255 && rsrq >= -30) {
                    qual->quality(rsrq);
                }
            }
        }
        CHECK_PARSER_OK(resp.readResult());
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
    if (firmwareUpdateR510_) {
        return SYSTEM_ERROR_NONE;
    }

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
    // If R510 firmware update, force extended timings
    if (firmwareUpdateR510_) {
        timeout = UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_TIMEOUT;
        period = UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_PERIOD;
    }
    auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser.execCommand(period, "AT");
        if (r < 0 && r != SYSTEM_ERROR_TIMEOUT) {
            return r;
        }
        if (r == AtResponse::OK) {
            return SYSTEM_ERROR_NONE;
        }
        // R510 Firmware Update
        if (ncpId() == PLATFORM_NCP_SARA_R510) {
            if (firmwareInstallRespCodeR510_ != lastFirmwareInstallRespCodeR510_) {
                t1 = HAL_Timer_Get_Milli_Seconds(); // If update is progressing, reset AT/OK wait timeout
                lastFirmwareInstallRespCodeR510_ = firmwareInstallRespCodeR510_;
            }
            if (firmwareInstallRespCodeR510_ == UUFWINSTALL_COMPLETE) {
                firmwareUpdateR510_ = false;
                firmwareInstallRespCodeR510_ = -1;
                lastFirmwareInstallRespCodeR510_ = -1;
                break; // Install complete
            }
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
        waitReadyRetries_ = 0;
        LOG(TRACE, "NCP ready to accept AT commands");

        auto r = initReady(modemState);
        if (r != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to perform early initialization");
            ready_ = false;
        }
    } else {
        LOG(ERROR, "No response from NCP");
    }

    if (!ready_ && !firmwareUpdateR510_) {
        // Disable voltage translator
        modemSetUartState(false);
        // Hard reset the modem
        modemHardReset(true);
        ncpState(NcpState::OFF);
        if (++waitReadyRetries_ >= 10) {
            waitReadyRetries_ = 10;
            modemEmergencyHardReset();
        }

        return SYSTEM_ERROR_INVALID_STATE;
    }

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::checkNetConfForImsi() {
    int imsiCount = 0;
    char buf[32] = {};
    do {
        auto resp = parser_.sendCommand(UBLOX_CIMI_TIMEOUT, "AT+CIMI");
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

int SaraNcpClient::selectNetworkProf(ModemState& state) {
    int resetCount = 0;
    bool disableLowPowerModes = false;
    // Note: Not failing on AT error on ICCID/IMSI lookup since SIMs have shown strange edge cases
    // where they error for no reason, and hard resetting the modem or power cycling would not clear it.
    //
    // Check if we are using a Twilio Super SIM based on ICCID
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

    if (ncpId() == PLATFORM_NCP_SARA_R510) {
        // AT+CEMODE? query can take up to 24 seconds to poll without CellularFunctionality::MINIMUM first, and we don't
        // want to be dropping to CellularFunctionality::MINIMUM every boot / connection.
        // Check cached value to see if we need to change CEMODE=0 (PS_ONLY).
        CellularOperationMode cemode = CellularOperationMode::NONE;
        getOperationModeCached(cemode); // We're not checking the result because it will return SYSTEM_ERROR_NOT_FOUND the first time through
        if (cemode != CellularOperationMode::PS_ONLY) {
            CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM, true /* check */));
            CHECK_PARSER_OK(setOperationMode(CellularOperationMode::PS_ONLY, true /* check */, true /* save */));
            CHECK(modemSoftReset()); // reset the SIM
            CHECK(waitAtResponseFromPowerOn(state));
            // NOTE: may need to be in minimum functionality for setting umnoprof, let configureApn() set it back to full
        }
    }

    bool reset = false;
    do {
        // Set UMNOPROF and UBANDMASK as appropriate based on SIM
        auto respUmnoprof = parser_.sendCommand("AT+UMNOPROF?");
        reset = false;
        int curProf = static_cast<int>(UbloxSaraUmnoprof::NONE);
        // auto r = CHECK_PARSER(respUmnoprof.scanf("+UMNOPROF: %d", &curProf));
        auto r = respUmnoprof.scanf("+UMNOPROF: %d", &curProf);
        // CHECK_PARSER_OK(respUmnoprof.readResult());
        // If this command returns an ERROR, we can get stuck in an initialization loop.
        // Let's try to avoid that with a soft reset, before retrying.
        auto result = respUmnoprof.readResult();
        if (result != AtResponse::OK) {
            if (ncpId() == PLATFORM_NCP_SARA_R510) {
                CHECK_PARSER_OK(parser_.execCommand("AT+UFACTORY=2,2"));
            }
            modemSoftPowerOff();
            return SYSTEM_ERROR_AT_NOT_OK;
        }
        // First time setup, or switching between official SIM on wrong profile?
        if (r == 1 && (static_cast<UbloxSaraUmnoprof>(curProf) == UbloxSaraUmnoprof::SW_DEFAULT ||
                (netConf_.netProv() == CellularNetworkProvider::TWILIO && static_cast<UbloxSaraUmnoprof>(curProf) != UbloxSaraUmnoprof::STANDARD_EUROPE) ||
                (ncpId() == PLATFORM_NCP_SARA_R410 && netConf_.netProv() == CellularNetworkProvider::KORE_ATT && static_cast<UbloxSaraUmnoprof>(curProf) != UbloxSaraUmnoprof::ATT) ||
                (ncpId() == PLATFORM_NCP_SARA_R510 && netConf_.netProv() == CellularNetworkProvider::KORE_ATT && static_cast<UbloxSaraUmnoprof>(curProf) != UbloxSaraUmnoprof::ATT)) ) {
            int newProf = static_cast<int>(UbloxSaraUmnoprof::SIM_SELECT);
            // TWILIO Super SIM
            if (netConf_.netProv() == CellularNetworkProvider::TWILIO) {
                // _oldFirmwarePresent: u-blox firmware 05.06* and 05.07* does not have
                // UMNOPROF=100 available. Default to UMNOPROF=0 in that case.
                if (oldFirmwarePresent_) {
                    if (static_cast<UbloxSaraUmnoprof>(curProf) == UbloxSaraUmnoprof::SW_DEFAULT) {
                        break;
                    } else {
                        newProf = static_cast<int>(UbloxSaraUmnoprof::SW_DEFAULT);
                    }
                } else {
                    newProf = static_cast<int>(UbloxSaraUmnoprof::STANDARD_EUROPE);
                }
            }
            // KORE AT&T or 3rd Party SIM
            else {
                // Hard code ATT for R410 05.12 firmware versions or R510 Kore AT&T SIMs
                if (fwVersion_ == UBLOX_NCP_R4_APP_FW_VERSION_0512 || ncpId() == PLATFORM_NCP_SARA_R510) {
                    if (netConf_.netProv() == CellularNetworkProvider::KORE_ATT) {
                        newProf = static_cast<int>(UbloxSaraUmnoprof::ATT);
                    }
                }
                // break out of do-while if we're trying to set SIM_SELECT a third time
                if (resetCount >= 2) {
                    LOG(WARN, "UMNOPROF=1 did not resolve a built-in profile, please check if UMNOPROF=100 is required!");
                    break;
                }
            }

            // Disconnect before making changes to the UMNOPROF
            CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM, true /* check */));
            // This is a persistent setting
            parser_.execCommand(1000, "AT+UMNOPROF=%d", newProf);
            // Not checking for error since we will reset either way
            reset = true;
            disableLowPowerModes = true;
        } else if (r == 1 && static_cast<UbloxSaraUmnoprof>(curProf) == UbloxSaraUmnoprof::STANDARD_EUROPE) {
            auto respBand = parser_.sendCommand(UBLOX_UBANDMASK_TIMEOUT, "AT+UBANDMASK?");
            uint64_t ubandUint64 = 0;
            char ubandStr[24] = {};
            auto retBand = CHECK_PARSER(respBand.scanf("+UBANDMASK: 0,%23[^,]", ubandStr));
            CHECK_PARSER_OK(respBand.readResult());
            if (netConf_.netProv() == CellularNetworkProvider::TWILIO && retBand == 1) {
                char* pEnd = &ubandStr[0];
                ubandUint64 = strtoull(ubandStr, &pEnd, 10);
                // Only update if Twilio Super SIM and not set to correct bands
                if (pEnd - ubandStr > 0 && ubandUint64 != 6170) {
                    // Enable Cat-M1 bands 2,4,5,12 (AT&T), 13 (VZW) = 6170
                    parser_.execCommand(UBLOX_UBANDMASK_TIMEOUT, "AT+UBANDMASK=0,6170");
                    // Not checking for error since we will reset either way
                    reset = true;
                    disableLowPowerModes = false;
                }
            }
        }
        if (reset) {
            CHECK(modemSoftReset());
            CHECK(waitAtResponseFromPowerOn(state));

            // Checking for SIM readiness ensures that other related commands
            // like IFC or PSM/eDRX won't error out
            CHECK(checkSimReadiness());
            // Prevent modem from immediately dropping into PSM/eDRX modes
            // which (on 05.12) may be enabled as soon as the UMNOPROF has taken effect
            if (disableLowPowerModes) {
                CHECK(disablePsmEdrx());
            }
        }
    } while (reset && ++resetCount < 4); // Note: Twilio SIMs could take more than 2 tries in some error cases, others <= 2

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
            const int externalSimMode = 0; // Output mode
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
            if (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) {
                const int internalSimMode = 0; // Output mode
                const int internalSimValue = 1;
                if (mode != internalSimMode || value != internalSimValue) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode, internalSimValue));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                    if (ncpId() == PLATFORM_NCP_SARA_R510) {
                        // save 12 seconds by not resetting if we only have to change the output value LOW to HIGH
                        if (mode != internalSimMode) {
                            reset = true;
                        }
                    } else {
                        reset = true;
                    }
                }
            }
        /* XXX: This mode 23,10 was broken as of R510 v3.15, now resulting in ERROR
            else if (ncpId() == PLATFORM_NCP_SARA_R510) {
                // NOTE: [ch76449] R510S will not retain GPIO's HIGH after a cold boot
                // Workaround: Set pin that needs to be HIGH to mode "Module status indication",
                //             which will be set HIGH when the module is ON, and LOW when it's OFF.
                const int internalSimMode = 10; // Module status indication mode
                if (mode != internalSimMode) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                    reset = true;
                }
            }
        */
            else {
                const int internalSimMode = 255; // disabled
                if (mode != internalSimMode) {
                    const int r = CHECK_PARSER(parser_.execCommand("AT+UGPIOC=%u,%d",
                            UBLOX_NCP_SIM_SELECT_PIN, internalSimMode));
                    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
                    reset = true;
                }
            }
            break;
        }
    }

    if (reset) {
        CHECK(modemSoftReset());
        CHECK(waitAtResponseFromPowerOn(state));
    }

    // Checking for SIM readiness ensures that other related commands
    // like IFC or PSM/eDRX won't error out
    CHECK(checkSimReadiness(true));

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::disablePsmEdrx() {

    // XXX: In theory, it's good to check for SIM readiness as PSM/eDRX commands seem
    // to depend on it's readiness. However, if we have reached this point, it means
    // that sim readiness is already checked
    // CHECK(checkSimReadiness());

    // Force Power Saving mode to be disabled
    //
    // TODO: if we enable this feature in the future add logic to CHECK_PARSER macro(s)
    // to wait longer for device to become active (see MDMParser::_atOk)
    // Format is +CPSMS:0,,,"00010011","00000011"
    auto resp = parser_.sendCommand("AT+CPSMS?");
    unsigned psmState = 1;
    resp.scanf("+CPSMS:%u", &psmState);
    CHECK_PARSER(resp.readResult());
    // Disable PSM even if the above command errors out
    if (psmState == 1) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CPSMS=0"));
    }

    // Force eDRX mode to be disabled. AT+CEDRXS=0 doesn't seem disable eDRX completely, so
    // so we're disabling it for each reported RAT individually
    Vector<unsigned> acts;
    resp = parser_.sendCommand("AT+CEDRXS?");
    while (resp.hasNextLine()) {
        unsigned act = 0;
        unsigned eDRXCycle = 0;
        unsigned pagingTimeWindow = 0;
        // R410 disabled: +CEDRXS:
        // R510 disabled: +CEDRXS: 4,"0000"
        auto r = resp.scanf("+CEDRXS: %u,\"%d\",\"%d\"", &act, &eDRXCycle, &pagingTimeWindow);
        if (r >= 1 && (eDRXCycle != 0 || pagingTimeWindow != 0)) { // Ignore scanf() errors
            CHECK_TRUE(acts.append(act), SYSTEM_ERROR_NO_MEMORY);
        }
    }
    CHECK_PARSER_OK(resp.readResult());
    int lastError = AtResponse::OK;
    for (unsigned act: acts) {
        // This command may fail for unknown reason. eDRX mode is a persistent setting and, eventually,
        // it will get applied for each RAT during subsequent re-initialization attempts
        auto r = CHECK_PARSER(parser_.execCommand("AT+CEDRXS=3,%u", act)); // 3: Disable the use of eDRX
        if (r != AtResponse::OK) {
            lastError = r;
        }
    }
    CHECK_PARSER_OK(lastError);

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::waitAtResponseFromPowerOn(ModemState& modemState) {
    // Make sure we reset back to using hardware serial port @ default baudrate
    muxer_.stop();

    unsigned int attemptedBaudRate = (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) ?
            UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE :
            UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4;

    if (firmwareUpdateR510_) {
        attemptedBaudRate = UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE;
    }

    CHECK(serial_->setBaudRate(attemptedBaudRate));
    CHECK(initParser(serial_.get()));
    skipAll(serial_.get(), 1000);
    parser_.reset();

    int r = SYSTEM_ERROR_TIMEOUT;

    if (firmwareUpdateR510_) {
        r = waitAtResponse(UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_TIMEOUT, UBLOX_WAIT_AT_RESPONSE_WHILE_UUFWINSTALL_PERIOD);
        if (!r) {
            modemState = ModemState::DefaultBaudrate;
        }

        return r;
    }

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
        r = waitAtResponse(20000);
        if (!r) {
            modemState = ModemState::DefaultBaudrate;
        }
    } else {
        LOG_DEBUG(TRACE, "Trying at %d baud rate", attemptedBaudRate);
        r = waitAtResponse(10000);
        if (r) {
            CHECK(serial_->setBaudRate(UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE));
            CHECK(initParser(serial_.get()));
            skipAll(serial_.get());
            parser_.reset();
            LOG_DEBUG(TRACE, "Trying at %d baud rate", UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE);
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
    // v113: "08.90,A01.13" G350 (newer)
    //   v2: "08.70,A00.02" G350 (older)
    // v200: "L0.0.00.00.05.06,A.02.00" (R410 memory issue)
    // v202: "L0.0.00.00.05.07,A.02.02" (R410 demonstrator)
    // v204: "L0.0.00.00.05.08,A.02.04" (R410 maintenance)
    //   v1: "02.06,A00.01" (R510)
    auto resp = parser_.sendCommand("ATI9");
    int ver = 0;
    int major = 0;
    int minor = 0;
    int n = CHECK_PARSER(resp.scanf("%*[^,],%*[A.]%d.%d", &major, &minor));
    int r = resp.readResult();
    if (r == AtResponse::OK && n == 2) {
        ver = major * 100 + minor;
    }
    LOG(TRACE, "App firmware: %d", ver); // will be reported as 0 in case of error
    return ver;
}

int SaraNcpClient::initReady(ModemState state) {
    CHECK(waitAtResponse(5000));
    fwVersion_ = getAppFirmwareVersion();
    // L0.0.00.00.05.06,A.02.00 has a memory issue
    memoryIssuePresent_ = (ncpId() == PLATFORM_NCP_SARA_R410) ? (fwVersion_ == UBLOX_NCP_R4_APP_FW_VERSION_MEMORY_LEAK_ISSUE) : false;
    oldFirmwarePresent_ = (ncpId() == PLATFORM_NCP_SARA_R410) ? (fwVersion_ < UBLOX_NCP_R4_APP_FW_VERSION_LATEST_02B_01) : false;
    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard(state));
    // Make sure flow control is enabled as well
    // NOTE: this should work fine on SARA R4 firmware revisions that don't support it as well
    CHECK_PARSER_OK(parser_.execCommand("AT+IFC=2,2"));
    CHECK(waitAtResponse(10000));

    if (state != ModemState::MuxerAtChannel) {
        // Cold Boot only, Warm Boot will skip the following block...

        if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
            // Change the baudrate to 921600
            CHECK(changeBaudRate(UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2));
        } else {
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

        // Check that the modem is responsive at the new baudrate
        skipAll(serial_.get(), 1000);
        CHECK(waitAtResponse(10000));
    }

    // Select MNO and band profiles depending on the configuration
    if (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) {
        CHECK(selectNetworkProf(state));
    }

    // Reformat the operator string to be numeric
    // (allows the capture of `mcc` and `mnc`)
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=3,2"));

    // Enable packet domain error reporting
    CHECK_PARSER_OK(parser_.execCommand("AT+CGEREP=1,0"));

    if (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) {
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

        // Disable Cat-M1 low power modes
        CHECK(disablePsmEdrx());

        // Allows the R510 to drop into low power mode automatically after ~9s when idle
        if (ncpId() == PLATFORM_NCP_SARA_R510) {
            // XXX: R510 UPSV=1 appears to have issues dropping into low power mode unless we ensure to disable and enable on boot
            CHECK_PARSER_OK(setPowerSavingValue(CellularPowerSavingValue::UPSV_DISABLED, true /* check */));
            CHECK_PARSER_OK(setPowerSavingValue(CellularPowerSavingValue::UPSV_ENABLED_TIMER));
        }

    } else {
        // Force Power Saving mode to be disabled
        //
        // TODO: if we enable this feature in the future add logic to CHECK_PARSER macro(s)
        // to wait longer for device to become active (see MDMParser::_atOk)
        CHECK_PARSER_OK(setPowerSavingValue(CellularPowerSavingValue::UPSV_DISABLED, true /* check */));
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

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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
    unsigned runtimeBaudrate = (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) ?
            UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_R4 :
            UBLOX_NCP_RUNTIME_SERIAL_BAUDRATE_U2;

    // Feeling optimistic, try to see if the muxer is already available
    if (!muxer_.isRunning()) {
        LOG(TRACE, "Muxer is not currently running");
        if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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

    if (firmwareUpdateR510_) {
        runtimeBaudrate = UBLOX_NCP_DEFAULT_SERIAL_BAUDRATE;
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
    if (firmwareUpdateR510_) {
        state = ModemState::Unknown;
        return SYSTEM_ERROR_UNKNOWN;
    }

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
    if (ncpId() == PLATFORM_NCP_SARA_R510 && connState_ == NcpConnectionState::DISCONNECTED) {
        muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_R510);
    } else {
        muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_DEFAULT);
    }
    muxer_.setKeepAliveMaxMissed(UBLOX_NCP_KEEPALIVE_MAX_MISSED);
    muxer_.setMaxRetransmissions(3);
    muxer_.setAckTimeout(UBLOX_MUXER_T1);
    muxer_.setControlResponseTimeout(UBLOX_MUXER_T2);
    if (ncpId() == PLATFORM_NCP_SARA_R410 || ncpId() == PLATFORM_NCP_SARA_R510) {
        muxer_.useMscAsKeepAlive(true);
    }

    // Set channel state handler
    muxer_.setChannelStateHandler(muxChannelStateCb, this);

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::startNcpFwUpdate(bool update) {
    if (ncpId() != PLATFORM_NCP_SARA_R510) {
        return SYSTEM_ERROR_NONE;
    }

    firmwareUpdateR510_ = update;

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::checkSimCard(bool* failure) {
    auto check = [this]() -> int {
        auto resp = parser_.sendCommand("AT+CPIN?");
        char code[33] = {};
        int r = CHECK_PARSER(resp.scanf("+CPIN: %32[^\n]", code));
        CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
        r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
        if (!strcmp(code, "READY")) {
            CHECK_PARSER_OK(parser_.execCommand("AT+CCID"));
            // IFC checks are generally unrelated to the SIM. However, there is a
            // known issue with u-blox R410 that fails IFC and potentially some other
            // commands with `+CME ERROR: SIM failure`
            CHECK_PARSER_OK(parser_.execCommand("AT+IFC?"));
            return SYSTEM_ERROR_NONE;
        }
        return SYSTEM_ERROR_UNKNOWN;
    };

    for (unsigned i = 0; i < CHECK_SIM_CARD_ATTEMPTS; i++) {
        if (!check()) {
            // OK
            return 0;
        }
        if (failure) {
            *failure = true;
        }
        HAL_Delay_Milliseconds(CHECK_SIM_CARD_INTERVAL);
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int SaraNcpClient::checkSimReadiness(bool checkForRfReset) {
    // Using numeric CME ERROR codes
    // int r = CHECK_PARSER(parser_.execCommand("AT+CMEE=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);

    bool encounteredFailure = false;
    int r = checkSimCard(&encounteredFailure);

    if (checkForRfReset && encounteredFailure && ncpId() == PLATFORM_NCP_SARA_R410) {
        // There was an error initializing the SIM
        // This often leads to inability to talk over the data (PPP) muxed channel
        // for some reason. Attempt to cycle the modem through minimal/full functional state.
        // We only do this for R4-based devices, as U2-based modems seem to fail
        // to change baudrate later on for some reason
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM));
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::FULL));
    }

    return r;
}

int SaraNcpClient::getPowerSavingValue() {
    auto respUpsv = parser_.sendCommand("AT+UPSV?");
    int upsvVal = -1;
    // +UPSV: 1,2000,1
    auto upsvValCnt = respUpsv.scanf("+UPSV: %d,%*d,%*d", &upsvVal);
    CHECK_PARSER_OK(respUpsv.readResult());
    CHECK_TRUE(upsvValCnt == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

    return upsvVal;
}

int SaraNcpClient::setPowerSavingValue(CellularPowerSavingValue upsv, bool check) {
    if (check) {
        if ((int)upsv == CHECK(getPowerSavingValue())) {
            // Already in required state
            return SYSTEM_ERROR_NONE;
        }
    }

    int r = SYSTEM_ERROR_UNKNOWN;

    r = parser_.execCommand(1000, "AT+UPSV=%d",(int)upsv);

    CHECK_PARSER_OK(r);

    // AtResponse::Result!
    return r;
}

int SaraNcpClient::getOperationModeCached(CellularOperationMode& cemode) {
    uint32_t systemCacheOperationMode = 0;
    cemode = CellularOperationMode::NONE;
    int scResult = CHECK(particle::services::SystemCache::instance().get(particle::services::SystemCacheKey::CELLULAR_NCP_OPERATION_MODE, &systemCacheOperationMode, sizeof(systemCacheOperationMode)));
    // LOG(INFO, "GET CACHED systemCacheOperationMode %8x, res= %d", systemCacheOperationMode, scResult);
    (void) scResult;
    if ((systemCacheOperationMode & SYSTEM_CACHE_OPERATION_MODE_VALID) == SYSTEM_CACHE_OPERATION_MODE_VALID) {
        // previously checked/set before
        systemCacheOperationMode &= SYSTEM_CACHE_OPERATION_MODE_MASK;
        if (systemCacheOperationMode == particle::to_underlying(CellularOperationMode::PS_ONLY) ||
                systemCacheOperationMode == particle::to_underlying(CellularOperationMode::CS_PS_MODE)) {
            cemode = static_cast<CellularOperationMode>(systemCacheOperationMode);
            return SYSTEM_ERROR_NONE;
        }
    }

    return SYSTEM_ERROR_BAD_DATA;
}

int SaraNcpClient::setOperationModeCached(CellularOperationMode cemode) {
    uint32_t systemCacheOperationMode = SYSTEM_CACHE_OPERATION_MODE_VALID | (uint32_t)cemode;
    int scResult = particle::services::SystemCache::instance().set(particle::services::SystemCacheKey::CELLULAR_NCP_OPERATION_MODE, &systemCacheOperationMode, sizeof(systemCacheOperationMode));
    // LOG(INFO, "SET CACHED systemCacheOperationMode %8x, res= %d", systemCacheOperationMode, scResult);

    return scResult;
}

int SaraNcpClient::getOperationMode() {
    auto respCemode = parser_.sendCommand("AT+CEMODE?");
    int cemodeVal = -1;
    auto cemodeValCnt = respCemode.scanf("+CEMODE: %d", &cemodeVal);
    CHECK_PARSER_OK(respCemode.readResult());
    CHECK_TRUE(cemodeValCnt == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);

    return cemodeVal;
}

int SaraNcpClient::setOperationMode(CellularOperationMode cemode, bool check, bool save) {
    if (ncpId() != PLATFORM_NCP_SARA_R510) {
        return SYSTEM_ERROR_NONE; // only R510 functionality implemented, return without error.
    }

    if (check) {
        if ((int)cemode == CHECK(getOperationMode())) {
            if (save) {
                setOperationModeCached(cemode);
            }
            // Already in required state
            return SYSTEM_ERROR_NONE;
        }
    }

    int r = SYSTEM_ERROR_UNKNOWN;

    r = parser_.execCommand(1000, "AT+CEMODE=%d",(int)cemode);

    CHECK_PARSER_OK(r);

    setOperationModeCached(cemode);

    // AtResponse::Result!
    return r;
}

int SaraNcpClient::getModuleFunctionality() {
    auto resp = parser_.sendCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN?");
    int curVal = -1;
    auto r = resp.scanf("+CFUN: %d", &curVal);
    CHECK_PARSER_OK(resp.readResult());
    CHECK_TRUE(r == 1, SYSTEM_ERROR_AT_RESPONSE_UNEXPECTED);
    return curVal;
}

int SaraNcpClient::setModuleFunctionality(CellularFunctionality cfun, bool check) {
    if (check) {
        if ((int)cfun == CHECK(getModuleFunctionality())) {
            // Already in required state
            return 0;
        }
    }

    int r = SYSTEM_ERROR_UNKNOWN;

    if (ncpId() == PLATFORM_NCP_SARA_R510 ||
            (ncpId() == PLATFORM_NCP_SARA_R410 && cfun >= CellularFunctionality::AIRPLANE)) {
        // NOTE: R510 does not like the explicit CFUN=0,0 (x,0 is default, and we are intending on the default 0,0 but leaving as 0 for maximum compatibility)
        r = parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=%d", (int)cfun);
    } else {
        r = parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CFUN=%d,0", (int)cfun);
    }

    CHECK_PARSER(r);

    if (!r && ncpId() == PLATFORM_NCP_SARA_R410 && (cfun == CellularFunctionality::FULL || cfun == CellularFunctionality::AIRPLANE)) {
        // When switching to full-functionality mode on R410-based devices check the SIM card readiness
        // otherwise some other AT commands unrelated to SIM card will fail with CME ERROR e.g. SIM failure
        CHECK(checkSimCard());
    }
    // AtResponse::Result!
    return r;
}

int SaraNcpClient::configureApn(const CellularNetworkConfig& conf) {
    // IMPORTANT: Set modem full functionality!
    // Otherwise we won't be able to query ICCID/IMSI
    CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::FULL, true /* check */));

    netConf_ = conf;
    if (!netConf_.isValid()) {
        // First look for network settings based on ICCID
        char buf[32] = {};
        auto lenCcid = getIccidImpl(buf, sizeof(buf));
        CHECK_TRUE(lenCcid > 5, SYSTEM_ERROR_BAD_DATA);
        netConf_ = networkConfigForIccid(buf, lenCcid);

        // If failed above i.e., netConf_ is still not valid, look for network settings based on IMSI
        if (!netConf_.isValid()) {
            CHECK(checkNetConfForImsi());
        }
    }

    // Speed up connection times by not setting the APN if already set, this value is saved in NVM.
    //
    char cgdcontApnVal[64] = {}; // APN max length is 63 octets + 1 for \0
    char cgdcontIpVal[12] = {};
    char cgdcontFmt[50] = {};
    int rCgdcont = 0;
    bool setApn = true; // default to setting the APN
    auto apnLength = strlen(netConf_.hasApn() ? netConf_.apn() : "");
    auto respCgdcont = parser_.sendCommand("AT+CGDCONT?");
    if (apnLength) {  // %0s not allowed
        // Create the format string, done this way %ns because some APNs contain '.'
        //      "+CGDCONT: 1,\"%[^\"]\",\"%ns%*[^\n]"
        // Will match "IP", and "apnstr" or ""apn.str" from the following:
        //      "+CGDCONT: 1,"IP","apnstr","0.0.0.0",0,0,0,2,0,0,0,0,0,0,0"
        //      "+CGDCONT: 1,"IP","apnstr.mnc123.mcc456.gprs","100.123.456.789",0,0,0,2,0,0,0,0,0,0,0\r\n"
        //      "+CGDCONT: 1,"IP","apn.str.mnc123.mcc456.gprs","100.123.456.789",0,0,0,2,0,0,0,0,0,0,0\r\n"
        snprintf(cgdcontFmt, sizeof(cgdcontFmt), "+CGDCONT: %d,\"%%[^\"]\",\"%%%us%%*[^\n]", UBLOX_DEFAULT_CID, apnLength);
        while (respCgdcont.hasNextLine()) {
            rCgdcont = respCgdcont.scanf(cgdcontFmt, cgdcontIpVal, cgdcontApnVal);
            if (rCgdcont == 2) { // Ignore scanf() errors
                break;
            }
        }
        // Set the APN if any are true:
        // - We don't match anything from the scanf()
        // - cgdcontApnVal does not match expected APN
        // - No APN is set or explicitly blank "" APN (apnLength == 0)
        // - cgdcontIpVal is not "IP".  u-blox R510 appears to restore "IP" back to "NONIP" in some cases after
        //   being powered down abruptly or cleanly with AT+CPWROFF / PWR_ON pin.
        if (cgdcontApnVal[0] != '\0' &&
                strncmp(cgdcontApnVal, netConf_.hasApn() ? netConf_.apn() : "", sizeof(cgdcontApnVal)) == 0 &&
                strncmp(cgdcontIpVal, "IP", sizeof(cgdcontIpVal)) == 0) {
            setApn = false;
        }
    }
    CHECK_PARSER_OK(respCgdcont.readResult());
    // LOG(INFO,"setApn=%d, IpVal=%s, ApnVal=%s, Fmt=%s, hasApn()=%d, apn()=%s, strncmp=%d",
    //         setApn, cgdcontIpVal, cgdcontApnVal, cgdcontFmt, netConf_.hasApn(),
    //         netConf_.apn(), strncmp(cgdcontApnVal, netConf_.hasApn() ? netConf_.apn() : "", sizeof(cgdcontApnVal)) != 0);
    if (setApn) {
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::AIRPLANE));
        auto resp = parser_.sendCommand("AT+CGDCONT=%d,\"%s\",\"%s%s\"",
                UBLOX_DEFAULT_CID, UBLOX_DEFAULT_PDP_TYPE,
                (netConf_.hasUser() && netConf_.hasPassword()) ? "CHAP:" : "",
                netConf_.hasApn() ? netConf_.apn() : "");
        const int r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_AT_NOT_OK);
    }

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::setRegistrationTimeout(unsigned timeout) {
    registrationTimeout_ = std::max(timeout, REGISTRATION_TIMEOUT);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::registerNet() {
    int r = 0;

    // Set modem full functionality
    CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::FULL, true /* check */));

    resetRegistrationState();

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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

    // There is some kind of a bug in 02.19 R410 modem firmware in where it does not accept
    // any AT commands over the second muxed channel, unless we send something over channel 1 first.
    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        CHECK(waitAtResponse(parser_, 5000));
    }

    // If R510 and CONNECTED already, in the case where we are in low power mode and the keepalives
    // may be diabled/longer, just in case we are trying to resume a broken connection, let's timeout
    // faster with an AT/OK check prior to the following AT+CGATT? call that takes 90s to timeout.
    // This will also skip a follow up AT+CPWROFF call because ready_ will be set to false.
    if (ncpId() == PLATFORM_NCP_SARA_R510 && connState_ == NcpConnectionState::CONNECTED) {
        const int r = parser_.execCommand(1000, "AT");
        if (r != AtResponse::OK) {
            parserError_ = r;
        }
        CHECK(checkParser());
    }

    // CGATT should be enabled before we dial
    auto respCgatt = parser_.sendCommand("AT+CGATT?");
    int cgattState = -1;
    auto ret = respCgatt.scanf("+CGATT: %d", &cgattState);
    CHECK_PARSER(respCgatt.readResult());
    if (ret == 1 && cgattState == 0) {
        CHECK_PARSER_OK(parser_.execCommand("AT+CGATT=1"));
        // Modem could go through quick dereg/reg with this setting
        HAL_Delay_Milliseconds(1000);
    }

    CHECK_TRUE(muxer_.setChannelDataHandler(UBLOX_NCP_PPP_CHANNEL, muxerDataStream_->channelDataCb, muxerDataStream_.get()) == 0, SYSTEM_ERROR_INTERNAL);
    // Send data mode break
    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
        // Ignore response
        // SARA R4 does not support this, see above
        CHECK(dataParser_.execCommand(20000, "ATH"));
    }

    if (ncpId() == PLATFORM_NCP_SARA_R510) {
        CHECK_PARSER_OK(dataParser_.execCommand(10000, "AT+UPORTFWD=100,%d", UBLOX_DEFAULT_CID)); // Enable full cone NAT support
    }

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

int SaraNcpClient::dataModeError(int error) {
    if (ncpId() == PLATFORM_NCP_SARA_R410 && error == SYSTEM_ERROR_PPP_NO_CARRIER_IN_NETWORK_PHASE) {
        CHECK_TRUE(connectionState() == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
        // FIXME: this is a workaround for some R410 firmware versions where the PPP session suddenly dies
        // in network phase after the first IPCP ConfReq. For some reason CGATT=0/1 helps.
        const NcpClientLock lock(this);
        CHECK_FALSE(cgattWorkaroundApplied_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CGATT=0"));
        HAL_Delay_Milliseconds(1000);
        CHECK_PARSER_OK(parser_.execCommand(UBLOX_CFUN_TIMEOUT, "AT+CGATT=1"));
        cgattWorkaroundApplied_ = true;
    }
    return 0;
}

int SaraNcpClient::getMtu() {
    if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
        return UBLOX_NCP_R4_NO_HW_FLOW_CONTROL_MTU;
    }
    return 0;
}

int SaraNcpClient::urcs(bool enable) {
    const NcpClientLock lock(this);
    if (enable) {
        sleepNoPPPWrite_ = false;
        CHECK_TRUE(muxer_.resumeChannel(UBLOX_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);
        if ((ncpId() != PLATFORM_NCP_SARA_R510) ||
                ((ncpId() == PLATFORM_NCP_SARA_R510) &&
                !(connState_ == NcpConnectionState::CONNECTED || connState_ == NcpConnectionState::DISCONNECTED))) {
            muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_DEFAULT);
        }
        if (ncpId() == PLATFORM_NCP_SARA_U201) {
            // Make sure the modem is responsive again. U201 modems do take a while to
            // go back into functioning state
            CHECK(waitAtResponse(5000, gsm0710::proto::DEFAULT_T2));
        }
    } else {
        sleepNoPPPWrite_ = true;

        // R510 may be in low power mode, wake it up so we can get our muxer data channel suspend
        // echo, before we get into sleep and potentially prematurely wake by "network activity" (RX data)
        if (ncpId() == PLATFORM_NCP_SARA_R510) {
            CHECK(waitAtResponse(5000, 2000));
        }
        CHECK_TRUE(muxer_.suspendChannel(UBLOX_NCP_AT_CHANNEL) == 0, SYSTEM_ERROR_INTERNAL);

        // muxer_.suspendChannel(UBLOX_NCP_AT_CHANNEL) does not block muxer AT channel keepalives. All modems will
        // require this to be disabled to wake on network activity, and not prematurely wake on the echo of the keepalive.
        muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_DISABLED);
        HAL_Delay_Milliseconds(100); // allow a bit of time for muxer echo response
    }
    return SYSTEM_ERROR_NONE;
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

    if (ncpId() == PLATFORM_NCP_SARA_R510) {
        if (connState_ == NcpConnectionState::CONNECTED || connState_ == NcpConnectionState::DISCONNECTED) {
            muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_R510);
        } else {
            muxer_.setKeepAlivePeriod(UBLOX_NCP_KEEPALIVE_PERIOD_DEFAULT);
        }
    }

    if (connState_ == NcpConnectionState::CONNECTED) {
        // Reset CGATT workaround flag
        cgattWorkaroundApplied_ = false;

        if (firmwareUpdateR510_) {
            LOG(WARN, "Skipping PPP data connection to download NCP firmware");
            return;
        }

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
}

void SaraNcpClient::checkRegistrationState() {
    if (connState_ != NcpConnectionState::DISCONNECTED) {
        if (psd_.registered() || eps_.registered()) {
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

    if (netConf_.netProv() == CellularNetworkProvider::TWILIO && millis() - regStartTime_ <= REGISTRATION_TWILIO_HOLDOFF_TIMEOUT) {
        return 0;
    }

    auto timeout = (registrationInterventions_ + 1) * REGISTRATION_INTERVENTION_TIMEOUT;

    // Intervention to speed up registration or recover in case of failure
    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
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
                CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM));
                CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::FULL));
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
                CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::MINIMUM));
                CHECK_PARSER_OK(setOperationMode(CellularOperationMode::PS_ONLY, true /* check */));
                CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::FULL));
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
        // NOTE: The CIMI command has been known to not have an immediate response on u-blox modems
        // and currently has a 10 second timeout.  This command is also only for logging purposes
        // to monitor the currently selected IMSI on EtherSIM during registration.  For these reasons
        // we are intentionally not registering a parserError_ when this command does not return
        // AtResponse::OK.  Instead, in the case of a non-OK response, we will follow up the command
        // with an AT/OK check and subsequent checkParser() call to catch/address any modem parsing issues.
        auto respCimi = parser_.execCommand(UBLOX_CIMI_TIMEOUT, "AT+CIMI");
        if (respCimi != AtResponse::OK) {
            const int r = parser_.execCommand(1000, "AT");
            if (r != AtResponse::OK) {
                parserError_ = r;
            }
            CHECK(checkParser());
        }
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

    if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
        CHECK_PARSER(parser_.execCommand("AT+CEER"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
        CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
        // Check the signal seen by the module while trying to register
        // Do not need to check for an OK, as this is just for debugging purpose
        CHECK_PARSER(parser_.execCommand("AT+CSQ"));
    } else {
        CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));
        // Check the signal seen by the module while trying to register
        // Do not need to check for an OK, as this is just for debugging purpose,
        // and UCGED may sometimes return CME ERROR with low signal
        if (ncpId() == PLATFORM_NCP_SARA_R410) {
            CHECK_PARSER(parser_.execCommand("AT+UCGED=5"));
        }
        CHECK_PARSER(parser_.execCommand("AT+UCGED?"));
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
        .value = 1,
        .drive_strength = HAL_GPIO_DRIVE_DEFAULT
    };

    // Configure PWR_ON and RESET_N pins as OUTPUT and set to high by default
    CHECK(hal_gpio_configure(UBPWR, &conf, nullptr));
    CHECK(hal_gpio_configure(UBRST, &conf, nullptr));

#if HAL_PLATFORM_CELLULAR_MODEM_VOLTAGE_TRANSLATOR
    // Configure BUFEN as Push-Pull Output and default to 1 (disabled)
    CHECK(hal_gpio_configure(BUFEN, &conf, nullptr));
#endif // HAL_PLATFORM_CELLULAR_MODEM_VOLTAGE_TRANSLATOR

    // Configure VINT as Input for modem power state monitoring
    conf.mode = INPUT;
    CHECK(hal_gpio_configure(UBVINT, &conf, nullptr));

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

        // Perform power-on sequence depending on the NCP type
        if (ncpId() == PLATFORM_NCP_SARA_R410) {
            // R410
            // Low pulse 150-3200ms
            hal_gpio_write(UBPWR, 0);
            HAL_Delay_Milliseconds(150);
            hal_gpio_write(UBPWR, 1);
        } else if (ncpId() == PLATFORM_NCP_SARA_R510) {
            // R510
            // Low pulse 1000-2000ms
            hal_gpio_write(UBPWR, 0);
            HAL_Delay_Milliseconds(1500);
            hal_gpio_write(UBPWR, 1);
        } else {
            // U201
            // Low pulse 50-80us
            ATOMIC_BLOCK() {
                hal_gpio_write(UBPWR, 0);
                HAL_Delay_Microseconds(50);
                hal_gpio_write(UBPWR, 1);
            }
        }

        // Verify that the module was powered up by checking the VINT pin up to 1 sec
        if (waitModemPowerState(1, 1000)) {
            LOG(TRACE, "Modem powered on");
        } else {
            LOG(ERROR, "Failed to power on modem");
        }
    } else {
        LOG(TRACE, "Modem already on");
        ncpPowerState(NcpPowerState::ON);
        // FIXME:
        return SYSTEM_ERROR_ALREADY_EXISTS;
    }
    CHECK_TRUE(modemPowerState(), SYSTEM_ERROR_TIMEOUT);

    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::modemPowerOff() {
    if (firmwareUpdateR510_) {
        CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
        return SYSTEM_ERROR_NONE;
    }

    static std::once_flag f;
    std::call_once(f, [this]() {
        if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510 && modemPowerState()) {
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
        if (ncpId() != PLATFORM_NCP_SARA_R410 && ncpId() != PLATFORM_NCP_SARA_R510) {
            // U201
            // Low pulse 1s+
            hal_gpio_write(UBPWR, 0);
            HAL_Delay_Milliseconds(1500);
            hal_gpio_write(UBPWR, 1);
        } else {
            // If memory issue is present, ensure we don't force a power off too soon
            // to avoid hitting the 124 day memory housekeeping issue
            // TODO: Add ATOK check and AT+CPWROFF command attempt first?
            if (memoryIssuePresent_) {
                waitForPowerOff();
            }
            // R410
            // Low pulse 1.5s+
            hal_gpio_write(UBPWR, 0);
            HAL_Delay_Milliseconds(1600);
            hal_gpio_write(UBPWR, 1);
        }

        // Verify that the module was powered down by checking the VINT pin up to 10 sec
        if (waitModemPowerState(0, 10000)) {
            LOG(TRACE, "Modem powered off");
        } else {
            LOG(ERROR, "Failed to power off modem");
            if (ncpId() == PLATFORM_NCP_SARA_R510) {
                // XXX: modemHardReset() does not recover the modem
                modemEmergencyHardReset();
                // Modem will be OFF after emergency hard reset, but we set the state based on V_INT
                if (modemPowerState()) {
                    ncpPowerState(NcpPowerState::ON);
                } else {
                    ncpPowerState(NcpPowerState::OFF);
                    LOG(TRACE, "Modem off after emergency hard reset");
                }
            }
        }
    } else {
        LOG(TRACE, "Modem already off");
    }

    CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
    return SYSTEM_ERROR_NONE;
}

int SaraNcpClient::modemSoftReset() {
    if (ncpId() == PLATFORM_NCP_SARA_R410) {
        // R410
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::RESET_NO_SIM));
        HAL_Delay_Milliseconds(10000);
    } else if (ncpId() == PLATFORM_NCP_SARA_R510) {
        // R510
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::RESET_WITH_SIM));
        HAL_Delay_Milliseconds(10000);
    } else {
        // U201
        CHECK_PARSER_OK(setModuleFunctionality(CellularFunctionality::RESET_WITH_SIM));
        HAL_Delay_Milliseconds(1000);
    }
    return 0;
}

int SaraNcpClient::modemSoftPowerOff() {
    if (firmwareUpdateR510_) {
        CHECK_TRUE(!modemPowerState(), SYSTEM_ERROR_INVALID_STATE);
        return SYSTEM_ERROR_NONE;
    }

    if (modemPowerState()) {
        ncpPowerState(NcpPowerState::TRANSIENT_OFF);
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
    if (ncpId() == PLATFORM_NCP_SARA_U201) {
        // Low pulse for 50ms
        hal_gpio_write(UBRST, 0);
        HAL_Delay_Milliseconds(50);
        hal_gpio_write(UBRST, 1);
        HAL_Delay_Milliseconds(1000);   // just in case
        ncpPowerState(NcpPowerState::TRANSIENT_ON); // TODO: Test this
        // NOTE: powerOff argument is ignored, modem will restart automatically
        // in all cases
    } else if (ncpId() == PLATFORM_NCP_SARA_R510) {
        hal_gpio_write(UBRST, 0);
        HAL_Delay_Milliseconds(200);
        hal_gpio_write(UBRST, 1);
        ncpPowerState(NcpPowerState::TRANSIENT_ON);
        // Note: No need to apply just-in-case delays, plus the radio seems to
        // reset a few times rapidly and is unresponsive to AT for a few runs.
        // Still recoverable by using the TRANSIENT_ON state as above, which
        // recognizes to wait longer during this transition.
    } else {
        // If memory issue is present, ensure we don't force a power off too soon
        // to avoid hitting the 124 day memory housekeeping issue
        if (memoryIssuePresent_) {
            waitForPowerOff();
        }
        hal_gpio_write(UBRST, 0);
        HAL_Delay_Milliseconds(10000);
        hal_gpio_write(UBRST, 1);
        HAL_Delay_Milliseconds(1000);   // just in case
        // IMPORTANT: R4 is powered-off after applying RESET!
        if (!powerOff) {
            LOG(TRACE, "Powering modem on after hard reset, ncpId: 0x%02x", ncpId());
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

int SaraNcpClient::modemEmergencyHardReset() {
    if (ncpId() != PLATFORM_NCP_SARA_R510) {
        return SYSTEM_ERROR_NONE;
    }

    LOG(TRACE, "Emergency hardware shutdown the modem");
    const auto pwrState = modemPowerState();
    // We can only reset the modem in the powered state
    if (!pwrState) {
        LOG(ERROR, "Modem is not powered on!");
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // Low held on power pin
    hal_gpio_write(UBPWR, 0);
    HAL_Delay_Milliseconds(500);
    // Low held on reset pin
    hal_gpio_write(UBRST, 0);
    // Release power pin after 23s (23.5)
    HAL_Delay_Milliseconds(23000);
    hal_gpio_write(UBPWR, 1);
    // Release reset pin after 1.5s (2s)
    HAL_Delay_Milliseconds(2000);
    hal_gpio_write(UBRST, 1);

    ncpPowerState(NcpPowerState::TRANSIENT_ON);
    return SYSTEM_ERROR_NONE;
}

bool SaraNcpClient::modemPowerState() const {
    return hal_gpio_read(UBVINT);
}

int SaraNcpClient::modemSetUartState(bool state) const {
#if HAL_PLATFORM_CELLULAR_MODEM_VOLTAGE_TRANSLATOR
    LOG(TRACE, "Setting UART voltage translator state %d", state);
    hal_gpio_write(BUFEN, state ? 0 : 1);
#endif // HAL_PLATFORM_CELLULAR_MODEM_VOLTAGE_TRANSLATOR
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
