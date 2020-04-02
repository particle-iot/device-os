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
const auto QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE = 115200;

const auto QUECTEL_NCP_MAX_MUXER_FRAME_SIZE = 1509;
const auto QUECTEL_NCP_KEEPALIVE_PERIOD = 5000; // milliseconds
const auto QUECTEL_NCP_KEEPALIVE_MAX_MISSED = 5;

// FIXME: for now using a very large buffer
const auto QUECTEL_NCP_AT_CHANNEL_RX_BUFFER_SIZE = 4096;

const auto QUECTEL_NCP_AT_CHANNEL = 1;
const auto QUECTEL_NCP_PPP_CHANNEL = 2;

const auto QUECTEL_NCP_SIM_SELECT_PIN = 23;

const unsigned REGISTRATION_CHECK_INTERVAL = 15 * 1000;
const unsigned REGISTRATION_TIMEOUT = 10 * 60 * 1000;

// Undefine hardware version
const auto HW_VERSION_UNDEFINED = 0xFF;

// Hardware version
// V003 - 0x00 (disable hwfc)
// V004 - 0x01 (enable hwfc)
const auto HAL_VERSION_B5SOM_V003 = 0x00;

const auto ICCID_MAX_LENGTH = 20;

using LacType = decltype(CellularGlobalIdentity::location_area_code);
using CidType = decltype(CellularGlobalIdentity::cell_id);

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

    uint32_t hwVersion = HW_VERSION_UNDEFINED;
    auto ret = hal_get_device_hw_version(&hwVersion, nullptr);
    if (ret == SYSTEM_ERROR_NONE && hwVersion == HAL_VERSION_B5SOM_V003) {
        sconf = SERIAL_8N1;
        LOG(TRACE, "Disable Hardware Flow control!");
    }

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
    registrationTimeout_ = REGISTRATION_TIMEOUT;
    resetRegistrationState();
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
        // Home network or roaming
        if (val[0] == 1 || val[0] == 5) {
            self->creg_ = RegistrationState::Registered;
        } else {
            self->creg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
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
        // Home network or roaming
        if (val[0] == 1 || val[0] == 5) {
            self->cgreg_ = RegistrationState::Registered;
        } else {
            self->cgreg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
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
        // Home network or roaming
        if (val[0] == 1 || val[0] == 5) {
            self->cereg_ = RegistrationState::Registered;
        } else {
            self->cereg_ = RegistrationState::NotRegistered;
        }
        self->checkRegistrationState();
        // Cellular Global Identity (partial)
        if (r >= 3) {
            auto rat = r >= 4 ? static_cast<CellularAccessTechnology>(val[3]) : self->act_;
            switch (rat) {
                case CellularAccessTechnology::LTE:
                case CellularAccessTechnology::EC_GSM_IOT:
                case CellularAccessTechnology::E_UTRAN: {
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

int QuectelNcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return SYSTEM_ERROR_NONE;
    }
    // Power on the modem
    CHECK(modemPowerOn());
    CHECK(waitReady());
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    muxer_.stop();
    // Power down
    modemPowerOff();
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
        return SYSTEM_ERROR_NONE;
    }
    CHECK(checkParser());
    const int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
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
    int err = muxer_.writeChannel(QUECTEL_NCP_PPP_CHANNEL, data, size);

    if (err) {
        // Make sure we are going into an error state if muxer for some reason fails
        // to write into the data channel.
        disable();
    }

    return err;
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

int QuectelNcpClient::getIccid(char* buf, size_t size) {
    const NcpClientLock lock(this);
    CHECK(checkParser());

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
    CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));

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
        {"CAT-M1", CellularAccessTechnology::EC_GSM_IOT},
        {"CAT-NB1", CellularAccessTechnology::E_UTRAN}
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
                case CellularAccessTechnology::EC_GSM_IOT:
                case CellularAccessTechnology::E_UTRAN: {
                    if (qcsqVals >= 5) {
                        const int min_rsrq_mul_by_100 = -1950;
                        const int max_rsrq_mul_by_100 = -300;

                        auto rsrp = vals[1];
                        auto rsrq_mul_100 = vals[3] * 100;

                        qual->accessTechnology(v.rat);

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

                        if (rsrq_mul_100 < min_rsrq_mul_by_100 && rsrq_mul_100 >= -2000) {
                            qual->quality(0);
                        } else if (rsrq_mul_100 >= max_rsrq_mul_by_100 && rsrq_mul_100 <=0) {
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
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::waitAtResponse(unsigned int timeout, unsigned int period) {
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        const int r = parser_.execCommand(period, "AT");
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

int QuectelNcpClient::waitReady() {
    if (ready_) {
        return SYSTEM_ERROR_NONE;
    }
    muxer_.stop();
    CHECK(serial_->setBaudRate(QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE));
    CHECK(initParser(serial_.get()));
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
        // Hard reset the modem
        modemHardReset(true);
        ncpState(NcpState::OFF);

        return SYSTEM_ERROR_INVALID_STATE;
    }

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::selectSimCard() {
    // Set modem full functionality
    int r = CHECK_PARSER(parser_.execCommand("AT+CFUN=1,0"));
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
    // Enable flow control and change to runtime baudrate
    auto runtimeBaudrate = QUECTEL_NCP_DEFAULT_SERIAL_BAUDRATE;
    uint32_t hwVersion = HW_VERSION_UNDEFINED;
    auto ret = hal_get_device_hw_version(&hwVersion, nullptr);
    if (ret == SYSTEM_ERROR_NONE && hwVersion == HAL_VERSION_B5SOM_V003) {
        CHECK_PARSER_OK(parser_.execCommand("AT+IFC=0,0"));
    } else {
        runtimeBaudrate = QUECTEL_NCP_RUNTIME_SERIAL_BAUDRATE;
        CHECK_PARSER_OK(parser_.execCommand("AT+IFC=2,2"));
    }
    CHECK(changeBaudRate(runtimeBaudrate));
    // Check that the modem is responsive at the new baudrate
    skipAll(serial_.get(), 1000);
    CHECK(waitAtResponse(10000));

    // Select either internal or external SIM card slot depending on the configuration
    CHECK(selectSimCard());

    // Just in case disconnect
    int r = CHECK_PARSER(parser_.execCommand("AT+COPS=2"));
    // CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);

    if (ncpId() == PLATFORM_NCP_QUECTEL_BG96) {
        // FIXME: Force Cat M1-only mode, do we need to do it on Quectel NCP?
        // Scan LTE only, take effect immediately
        CHECK_PARSER(parser_.execCommand("AT+QCFG=\"nwscanmode\",3,1"));
        // Configure Network Category to be Searched under LTE RAT
        // Only use LTE Cat M1, take effect immediately
        CHECK_PARSER(parser_.execCommand("AT+QCFG=\"iotopmode\",0,1"));

        // Force eDRX mode to be disabled.
        CHECK_PARSER(parser_.execCommand("AT+CEDRXS=0"));

        // Disable Power Saving Mode
        CHECK_PARSER(parser_.execCommand("AT+CPSMS=0"));
    }

    // Select (U)SIM card in slot 1, EG91 has two SIM card slots
    if (ncpId() == PLATFORM_NCP_QUECTEL_EG91_E || \
        ncpId() == PLATFORM_NCP_QUECTEL_EG91_NA || \
        ncpId() == PLATFORM_NCP_QUECTEL_EG91_EX) {
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
    r = CHECK_PARSER(parser_.execCommand("AT+CMUX=0,0,%d,1509,,,,,", portspeed));
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

    // Make sure that we receive URCs only on AT channel, ignore response code
    // just in case
    CHECK_PARSER(parser_.execCommand("AT+QCFG=\"cmux/urcport\",1"));

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
        r = parser_.execCommand("AT+CCID");
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        return SYSTEM_ERROR_NONE;
    }
    return SYSTEM_ERROR_UNKNOWN;
}

int QuectelNcpClient::configureApn(const CellularNetworkConfig& conf) {
    netConf_ = conf;
    if (!netConf_.isValid()) {
        // FIXME: CIMI may fail, need delay here
        HAL_Delay_Milliseconds(1000);
        // Look for network settings based on IMSI
        char buf[32] = {};
        auto resp = parser_.sendCommand("AT+CIMI");
        CHECK_PARSER(resp.readLine(buf, sizeof(buf)));
        const int r = CHECK_PARSER(resp.readResult());
        CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
        netConf_ = networkConfigForImsi(buf, strlen(buf));
    }
    // FIXME: for now IPv4 context only
    auto resp = parser_.sendCommand("AT+CGDCONT=1,\"IP\",\"%s\"",
            netConf_.hasApn() ? netConf_.apn() : "");
    const int r = CHECK_PARSER(resp.readResult());
    CHECK_TRUE(r == AtResponse::OK, SYSTEM_ERROR_UNKNOWN);
    return SYSTEM_ERROR_NONE;
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
                                       return SYSTEM_ERROR_NONE;
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
        return SYSTEM_ERROR_NONE;
    }
    SCOPE_GUARD({ regCheckTime_ = millis(); });

    // Check GPRS, LET, NB-IOT network registration status
    CHECK_PARSER_OK(parser_.execCommand("AT+CREG?"));
    CHECK_PARSER_OK(parser_.execCommand("AT+CGREG?"));
    CHECK_PARSER_OK(parser_.execCommand("AT+CEREG?"));

    if (connState_ == NcpConnectionState::CONNECTING && millis() - regStartTime_ >= registrationTimeout_) {
        LOG(WARN, "Resetting the modem due to the network registration timeout");
        muxer_.stop();
        modemHardReset();
        ncpState(NcpState::OFF);
    }
    return SYSTEM_ERROR_NONE;
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

    return SYSTEM_ERROR_NONE;
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

    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemPowerOff() const {
    if (modemPowerState()) {
        LOG(TRACE, "Powering modem off");
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
    return SYSTEM_ERROR_NONE;
}

int QuectelNcpClient::modemHardReset(bool powerOff) const {
    LOG(TRACE, "Hard resetting the modem");

    // BG96 reset, 150ms <= reset pulse <= 460ms
    HAL_GPIO_Write(BGRST, 1);
    HAL_Delay_Milliseconds(300);
    HAL_GPIO_Write(BGRST, 0);

    return SYSTEM_ERROR_NONE;
}

bool QuectelNcpClient::modemPowerState() const {
    // LOG(TRACE, "BGVINT: %d", HAL_GPIO_Read(BGVINT));
    return !HAL_GPIO_Read(BGVINT);
}

// // Use BG96 status pin to enable/disable voltage convert IC Automatically
// int QuectelNcpClient::modemSetUartState(bool state) const {
//     return SYSTEM_ERROR_NONE;
// }

} // namespace particle
