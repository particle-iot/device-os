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

#include "logging.h"
LOG_SOURCE_CATEGORY("service.atc");

#include "atclient.h"
#include <cstdio>
#include "str_util.h"
#include "system_error.h"
#include "check.h"
#include "stream.h"
#include "spark_wiring_ticks.h"
#include "enumclass.h"

using namespace particle;

namespace particle {
namespace services {
namespace at {

const char* const stateNames[] = {
    "NOT_READY",
    "READY",
    "COMMAND_SENT",
    "INTERMEDIATE_RESULT_CODE",
    "INFORMATION_TEXT",
    "FINAL_RESULT_CODE",
    "ERROR"
};

constexpr const char* AtClientBase::resultCodes_[];

// 5s
const auto ATCLIENT_DEFAULT_TIMEOUT = 5000;

// 2s
const auto ATCLIENT_READY_TIMEOUT = 2000;

AtClientBase::AtClientBase(Stream* stream)
        : stream_(stream),
          timeout_(ATCLIENT_DEFAULT_TIMEOUT) {
}

AtClientBase::~AtClientBase() {
}

void AtClientBase::setTimeout(unsigned int timeout) {
    timeout_ = timeout;
}

unsigned int AtClientBase::getTimeout() const {
    return timeout_;
}

void AtClientBase::reset() {
    setState(State::NOT_READY);
}

int AtClientBase::waitReady(unsigned int timeout) {
    auto start = millis();
    int r = SYSTEM_ERROR_TIMEOUT;

    TimeoutOverride t(ATCLIENT_READY_TIMEOUT, this);

    while ((millis() - start) < timeout) {
        r = waitState(State::READY);
        if (!r) {
            return 0;
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

void AtClientBase::setStream(Stream* stream) {
    stream_ = stream;
}

Stream* AtClientBase::getStream() {
    return stream_;
}

int AtClientBase::run() {
    switch (state_) {
        case State::NOT_READY: {
            CHECK_TRUE(writeCommand("ATE0") > 0, SYSTEM_ERROR_UNKNOWN);
            memset(buffer_, 0, sizeof(buffer_));
            int r = readLine(buffer_, sizeof(buffer_), true);
            CHECK_TRUE(r > 0, r);
            if (parseResultCode(buffer_, r) == ResultCode::OK) {
                setState(State::READY);
            }
            break;
        }

        case State::READY: {
            // Nothing to do here
            break;
        }

        case State::COMMAND_SENT:
        case State::INTERMEDIATE_RESULT_CODE:
        case State::INFORMATION_TEXT: {
            resetData();
            int r = readLine(buffer_, sizeof(buffer_), true);
            if (r < 0) {
                setState(State::ERROR);
            } else {
                LOG_DEBUG(TRACE, "Line: \"%s\"", buffer_);
                auto code = parseResultCode(buffer_, r);
                if (code != ResultCode::UNKNOWN) {
                    // Got some result code
                    if (isFinalResultCode(code)) {
                        resultCode_ = code;
                        setState(State::FINAL_RESULT_CODE);
                    } else {
                        // Intermediate result code
                        resultCode_ = code;
                        if (!parseIntermediateResultCode(buffer_, r)) {
                            setState(State::INTERMEDIATE_RESULT_CODE);
                        } else {
                            setState(State::ERROR);
                        }
                    }
                } else {
                    // This is information text
                    if (!parseInformationText(buffer_, r)) {
                        setState(State::INFORMATION_TEXT);
                    } else {
                        setState(State::ERROR);
                    }
                }
            }
            break;
        }

        case State::FINAL_RESULT_CODE: {
            setState(State::READY);
            break;
        }
        case State::ERROR: {
            // Go into NOT_READY and attempt to recover
            setState(State::NOT_READY);
            break;
        }
    }

    if (getState() == State::ERROR) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    return SYSTEM_ERROR_NONE;
}

int AtClientBase::readLine(char* buf, size_t size, bool skipEmpty) {
    system_tick_t start = millis();
    size_t read = 0;

    int ret = SYSTEM_ERROR_NOT_FOUND;

    while(read < (size - 1)) {
        CHECK_TRUE((millis() - start) <= timeout_, SYSTEM_ERROR_TIMEOUT);

        // FIXME
        char c;
        int r = stream_->read(&c, 1);
        if (r > 0) {
            buf[read] = c;
            read += r;
            if (endsWith(buf, read, getTerminator(), strlen(getTerminator()))) {
                // We are done
                // Strip non-printable characters just in case
                read = stripNonPrintable(buf, read - strlen(getTerminator()));
                if (read == 0 && skipEmpty) {
                    continue;
                }
                read += strlen(getTerminator());
                ret = read;
                buf[read] = '\0';
                break;
            }
        }
    }

    return ret;
}

AtClientBase::ResultCode AtClientBase::parseResultCode(const char* str, size_t len) {
    ResultCode code = ResultCode::UNKNOWN;

    for (unsigned i = 0; i < sizeof(resultCodes_) / sizeof(resultCodes_[0]); i++) {
        if (startsWith(str, len, resultCodes_[i], strlen(resultCodes_[i]))) {
            code = (ResultCode)i;
            break;
        }
    }

    if (code == ResultCode::UNKNOWN) {
        // Check if this is a custom intermediate result code
        if (startsWith(str, len, "+", 1)) {
            code = ResultCode::INTERMEDIATE;
        }
    }

    if (code != ResultCode::UNKNOWN && code != ResultCode::INTERMEDIATE) {
        LOG_DEBUG(TRACE, "Result code: %s", resultCodes_[to_underlying(code)]);
    }

    return code;
}

bool AtClientBase::isFinalResultCode(ResultCode code) {
    if (code == ResultCode::UNKNOWN ||
        code == ResultCode::CONNECT ||
        code == ResultCode::INTERMEDIATE) {
        return false;
    }

    return true;
}

int AtClientBase::sendCommand(const char* fmt, ...) {
    waitReady(ATCLIENT_DEFAULT_TIMEOUT * 2);
    CHECK_TRUE(state_ == State::READY || state_ == State::FINAL_RESULT_CODE, SYSTEM_ERROR_INVALID_STATE);

    va_list args;
    va_start(args, fmt);
    int r = writeCommand(fmt, args);
    va_end(args);

    CHECK_TRUE(r > 0, r);

    setState(State::COMMAND_SENT);

    return r;
}

int AtClientBase::writeCommand(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int r = writeCommand(fmt, args);
    va_end(args);
    return r;
}

int AtClientBase::writeCommand(const char* fmt, va_list args) {
    char buf[128] = {};
    int n = vsnprintf(buf, sizeof(buf), fmt, args);

    CHECK_TRUE(n > 0, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(n < (int)(sizeof(buf) - strlen(getTerminator()) - 1), SYSTEM_ERROR_TOO_LARGE);

    int r = snprintf(buf + n, sizeof(buf) - n, "%s", getTerminator());
    CHECK_TRUE(r > 0, SYSTEM_ERROR_UNKNOWN);
    n += r;

    int written = stream_->write(buf, n);
    CHECK_TRUE(written > 0, SYSTEM_ERROR_IO);

    LOG_DEBUG(TRACE, "Sending command: \"%s\"", buf);

    return written;
}

int AtClientBase::waitIntermediateResultCode() {
    return waitState(State::INTERMEDIATE_RESULT_CODE);
}

int AtClientBase::waitInformationText() {
    return waitState(State::INFORMATION_TEXT);
}

int AtClientBase::waitFinalResultCode() {
    return waitState(State::FINAL_RESULT_CODE);
}

int AtClientBase::waitState(State state) {
    LOG_DEBUG(TRACE, "Waiting for state %s, current state %s",
            stateNames[to_underlying(state)], stateNames[to_underlying(getState())]);
    run();
    CHECK_TRUE(getState() == state, SYSTEM_ERROR_INVALID_STATE);
    return 0;
}

const char* AtClientBase::getInformationText() const {
    return informationText_;
}

const char* AtClientBase::getIntermediateResultCode() const {
    return intermediateResultCode_;
}

const char* AtClientBase::getIntermediateResultCodeValue() const {
    return intermediateResultCodeValue_;
}

AtClientBase::ResultCode AtClientBase::getFinalResultCode() const {
    return resultCode_;
}

const char* AtClientBase::getTerminator() const {
    return terminator_;
}

AtClientBase::State AtClientBase::getState() const {
    return state_;
}

void AtClientBase::setState(State state) {
    LOG_DEBUG(TRACE, "State %s -> %s", stateNames[to_underlying(getState())], stateNames[to_underlying(state)]);
    switch (state) {
        case State::READY:
        case State::NOT_READY: {
            // FIXME: there should be a flushInput() method
            auto start = millis();
            if (getState() != State::NOT_READY) {
                start = 0;
            }
            while ((millis() - start) < ATCLIENT_READY_TIMEOUT ||
                    stream_->read(buffer_, sizeof(buffer_)) > 0);
            // Reset a bunch of things
            resetData();
            break;
        }
    }
    state_ = state;
}

size_t AtClientBase::stripNonPrintable(char* str, size_t len) {
    char* in = str;
    char* out = str;
    while ((in - str) < (int)len) {
        if (!isPrintable(in, 1)) {
            ++in;
            continue;
        }
        *(out++) = *(in++);
    }

    return out - str;
}

int AtClientBase::parseIntermediateResultCode(char* str, size_t len) {
    char* semiColon = strchr(str, ':');
    if (semiColon) {
        // There is a value
        *semiColon = '\0';
        intermediateResultCode_ = str;
        intermediateResultCodeValue_ = semiColon + 2; // ": "
        str[len - strlen(getTerminator())] = '\0';
        LOG_DEBUG(TRACE, "Intermediate result code: \"%s\", value \"%s\"", intermediateResultCode_, intermediateResultCodeValue_);
    } else {
        intermediateResultCode_ = str;
        intermediateResultCodeValue_ = nullptr;
        str[len - strlen(getTerminator())] = '\0';
        LOG_DEBUG(TRACE, "Intermediate result code: \"%s\"", intermediateResultCode_);
    }

    return 0;
}

int AtClientBase::parseInformationText(char* str, size_t len) {
    informationText_ = str;
    informationText_[len - strlen(getTerminator())] = '\0';
    LOG_DEBUG(TRACE, "Information text: \"%s\"", informationText_);
    return 0;
}

void AtClientBase::resetData() {
    memset(buffer_, 0, sizeof(buffer_));
    intermediateResultCode_ = nullptr;
    intermediateResultCodeValue_ = nullptr;
    informationText_ = nullptr;
    resultCode_ = ResultCode::UNKNOWN;
}

// ArgonNcpAtClient

ArgonNcpAtClient::ArgonNcpAtClient(Stream* stream)
        : AtClientBase(stream) {
}

ArgonNcpAtClient::~ArgonNcpAtClient() {
}

int ArgonNcpAtClient::init() {
    return 0;
}

int ArgonNcpAtClient::destroy() {
    return 0;
}

int ArgonNcpAtClient::getVersion(char* buf, size_t bufSize) {
    int r = sendCommand("AT+CGMR");
    CHECK_TRUE(r > 0, r);

    CHECK(waitInformationText());
    CHECK_TRUE(getInformationText(), SYSTEM_ERROR_BAD_DATA);

    char tmp[16] = {};
    strncpy(tmp, getInformationText(), sizeof(tmp));

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    // FIXME
    char ver[16] = {};
    CHECK_TRUE(sscanf(tmp, "%15s", ver) == 1, SYSTEM_ERROR_BAD_DATA);
    strncpy(buf, ver, bufSize);

    return 0;
}

int ArgonNcpAtClient::getModuleVersion(uint16_t* module) {
    CHECK_TRUE(module, SYSTEM_ERROR_INVALID_ARGUMENT);

    int r = sendCommand("AT+MVER");
    CHECK_TRUE(r > 0, r < 0 ? r : SYSTEM_ERROR_IO);

    CHECK(waitInformationText());
    CHECK_TRUE(getInformationText(), SYSTEM_ERROR_BAD_DATA);

    CHECK_TRUE(sscanf(getInformationText(), "%hu", module) == 1, SYSTEM_ERROR_BAD_DATA);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    return 0;
}

int ArgonNcpAtClient::startUpdate(size_t size) {
    // We could have used %zu, but some newlib configurations don't support it
    int r = sendCommand("AT+FWUPD=%lu", (long unsigned int)size);
    CHECK_TRUE(r > 0, r);

    TimeoutOverride t(10000, this);

    r = waitIntermediateResultCode();
    if (r) {
        if (getState() == State::FINAL_RESULT_CODE && getFinalResultCode() == ResultCode::ERROR) {
            return SYSTEM_ERROR_PROTOCOL;
        } else {
            return r;
        }
    }
    CHECK_TRUE(getIntermediateResultCode(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCode(), "+FWUPD"), SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(getIntermediateResultCodeValue(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCodeValue(), "ONGOING"), SYSTEM_ERROR_UNKNOWN);

    return 0;
}

int ArgonNcpAtClient::finishUpdate() {
    CHECK_TRUE(getState() == State::INTERMEDIATE_RESULT_CODE, SYSTEM_ERROR_INVALID_STATE);
    CHECK(waitFinalResultCode());
    if (getFinalResultCode() == ResultCode::ERROR) {
        return SYSTEM_ERROR_PROTOCOL;
    }
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int ArgonNcpAtClient::getMac(int idx, uint8_t* mac) {
    int r = sendCommand("AT+GETMAC=%d", idx);
    CHECK_TRUE(r > 0, r);

    r = waitIntermediateResultCode();
    if (r) {
        if (getState() == State::FINAL_RESULT_CODE && getFinalResultCode() == ResultCode::ERROR) {
            return SYSTEM_ERROR_PROTOCOL;
        } else {
            return r;
        }
    }
    CHECK_TRUE(getIntermediateResultCode(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCode(), "+GETMAC"), SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(getIntermediateResultCodeValue(), SYSTEM_ERROR_BAD_DATA);

    unsigned int tmp[6] = {};
    CHECK_TRUE(sscanf(getIntermediateResultCodeValue(), "\"%x:%x:%x:%x:%x:%x\"",
            tmp, tmp + 1, tmp + 2, tmp + 3, tmp + 4, tmp + 5) == 6, SYSTEM_ERROR_BAD_DATA);

    for (unsigned i = 0; i < 6; i++) {
        mac[i] = tmp[i];
    }
    CHECK(waitFinalResultCode());
    if (getFinalResultCode() == ResultCode::ERROR) {
        return SYSTEM_ERROR_PROTOCOL;
    }
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    return 0;
}

int ArgonNcpAtClient::startMuxer() {
    int r = sendCommand("AT+CMUX=0");
    CHECK_TRUE(r > 0, r);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int ArgonNcpAtClient::connect() {
    TimeoutOverride t(30000, this);
    int r = sendCommand("AT+CWDHCP=0,3");
    CHECK_TRUE(r > 0, r);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    r = sendCommand("AT+CWJAP=\"Particle\",\"ParticleWifi\"");
    CHECK_TRUE(r > 0, r);

    CHECK(waitInformationText());
    CHECK_TRUE(getInformationText(), SYSTEM_ERROR_BAD_DATA);

    reset();

    return 0;
}

// BoronNcpAtClient
BoronNcpAtClient::BoronNcpAtClient(Stream* stream)
        : AtClientBase(stream) {
}

BoronNcpAtClient::~BoronNcpAtClient() {
}

int BoronNcpAtClient::init() {
    return 0;
}

int BoronNcpAtClient::destroy() {
    return 0;
}

int BoronNcpAtClient::startMuxer() {
    int r = sendCommand("AT+CMUX=0,0,,1509,253,5,254,0,0");
    CHECK_TRUE(r > 0, r);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int BoronNcpAtClient::registerNet() {
    int r = sendCommand("AT+CREG=2");
    CHECK_TRUE(r > 0, r);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    r = sendCommand("AT+CGREG=2");
    CHECK_TRUE(r > 0, r);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    return 0;
}

int BoronNcpAtClient::isRegistered(bool w) {
    int r = sendCommand(w ? "AT+CGREG?" : "AT+CREG?");
    CHECK_TRUE(r > 0, r);

    r = waitIntermediateResultCode();
    if (r) {
        if (getState() == State::FINAL_RESULT_CODE && getFinalResultCode() == ResultCode::ERROR) {
            return SYSTEM_ERROR_PROTOCOL;
        } else {
            return r;
        }
    }
    CHECK_TRUE(getIntermediateResultCode(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCode(), w ? "+CGREG" : "+CREG"), SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(getIntermediateResultCodeValue(), SYSTEM_ERROR_BAD_DATA);

    unsigned int v[2] = {};
    CHECK_TRUE(sscanf(getIntermediateResultCodeValue(), "%u,%u", &v[0], &v[1]) == 2, SYSTEM_ERROR_BAD_DATA);

    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);

    return !(v[1] == 1 || v[1] == 5);
}

int BoronNcpAtClient::connect() {
    int r = sendCommand("AT+CGDATA=\"PPP\",1");
    CHECK_TRUE(r > 0, r);

    r = waitIntermediateResultCode();
    if (r) {
        if (getState() == State::FINAL_RESULT_CODE && getFinalResultCode() == ResultCode::ERROR) {
            return SYSTEM_ERROR_PROTOCOL;
        } else {
            return r;
        }
    }
    CHECK_TRUE(getIntermediateResultCode(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCode(), "CONNECT"), SYSTEM_ERROR_UNKNOWN);

    reset();

    return 0;
}

int BoronNcpAtClient::getImsi() {
    int r = sendCommand("AT+CIMI");
    CHECK_TRUE(r > 0, r);
    CHECK(waitInformationText());
    CHECK_TRUE(getInformationText(), SYSTEM_ERROR_BAD_DATA);
    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int BoronNcpAtClient::getCcid() {
    int r = sendCommand("AT+CCID");
    CHECK_TRUE(r > 0, r);
    r = waitIntermediateResultCode();
    if (r) {
        if (getState() == State::FINAL_RESULT_CODE && getFinalResultCode() == ResultCode::ERROR) {
            return SYSTEM_ERROR_PROTOCOL;
        } else {
            return r;
        }
    }
    CHECK_TRUE(getIntermediateResultCode(), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(!strcmp(getIntermediateResultCode(), "+CCID"), SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(getIntermediateResultCodeValue(), SYSTEM_ERROR_BAD_DATA);
    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int BoronNcpAtClient::selectSim(bool external) {
    int r = sendCommand(external ? "AT+UGPIOC=23,0,0" : "AT+UGPIOC=23,255");
    CHECK_TRUE(r > 0, r);
    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    r = sendCommand("AT+CFUN=16");
    CHECK(waitFinalResultCode());
    CHECK_TRUE(getFinalResultCode() == ResultCode::OK, SYSTEM_ERROR_UNKNOWN);
    reset();
    return 0;
}

} // at
} // services
} // particle
