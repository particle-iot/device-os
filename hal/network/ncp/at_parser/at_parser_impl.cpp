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

#include "at_parser_impl.h"

#include "at_response.h"

#include "stream.h"
#include "logging.h"
#include "scope_guard.h"
#include "check.h"
#include "debug.h"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <cassert>

LOG_SOURCE_CATEGORY("ncp.at")

namespace particle {

namespace detail {

namespace {

const size_t LOG_LINE_PREFIX_SIZE = 13; // strlen("0000000000 > ")

static_assert(LOG_LINE_PREFIX_SIZE < LOG_LINE_BUF_SIZE, "Log buffer is too small"); 

const char* cmdTermString(AtCommandTerminator term) {
    switch (term) {
    case AtCommandTerminator::LF:
        return "\n";
    case AtCommandTerminator::CRLF:
        return "\r\n";
    default:
        return "\r"; // CR
    }
}

inline bool isLineBreak(char c) {
    return (c == '\r' || c == '\n');
}
/*
bool findLineBreak(const char* data, size_t size, size_t* pos) {
    for (size_t i = 0; i < size; ++i) {
        if (isLineBreak(data[i])) {
            *pos = i;
            return true;
        }
    }
    return false;
}
*/
inline system_tick_t millis() {
    return HAL_Timer_Get_Milli_Seconds();
}

} // unnamed

// Final result codes recognized by the parser
const AtParserImpl::ResultCode AtParserImpl::RESULT_CODES[] = {
    { AtResponse::Result::OK, "OK", 2 },
    { AtResponse::Result::ERROR, "ERROR", 5 },
    { AtResponse::Result::BUSY, "BUSY", 4 },
    { AtResponse::Result::NO_ANSWER, "NO ANSWER", 9 },
    { AtResponse::Result::NO_CARRIER, "NO CARRIER", 10 },
    { AtResponse::Result::NO_DIALTONE, "NO DIALTONE", 11 },
    { AtResponse::Result::CME_ERROR, "+CME ERROR", 9 },
    { AtResponse::Result::CMS_ERROR, "+CMS ERROR", 9 }
};

const size_t AtParserImpl::RESULT_CODE_COUNT = sizeof(RESULT_CODES) / sizeof(RESULT_CODES[0]);

AtParserImpl::AtParserImpl(AtParserConfig conf) :
        conf_(std::move(conf)) {
    cmdTerm_ = cmdTermString(conf_.commandTerminator());
    cmdTermSize_ = strlen(cmdTerm_);
#if AT_COMMAND_LOG_ENABLED
    memset(logLine_, ' ', LOG_LINE_PREFIX_SIZE);
#endif
    reset();
}

AtParserImpl::~AtParserImpl() {
}

int AtParserImpl::newCommand() {
    if (!checkStatus(Status::READY)) {
        return SYSTEM_ERROR_BUSY; // Do not cancel current command
    }
    cmdTimeout_ = conf_.commandTimeout(); // Default command timeout
    clearStatus(Status::READY);
    setStatus(Status::WRITE_CMD_DATA);
    return 0;
}

int AtParserImpl::sendCommand() {
    if (!checkStatus(Status::WRITE_CMD_DATA)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (!checkStatus(Status::CMD_NOT_EMPTY)) {
        return error(SYSTEM_ERROR_NOT_ENOUGH_DATA);
    }
    setStatus(Status::WRITE_CMD_TERM);
    cmdTermOffs_ = 0;
    PARSER_CHECK(writeCmdTerm(&cmdTimeout_));
    return 0;
}

void AtParserImpl::cancelCommand() {
}

void AtParserImpl::commandTimeout(unsigned timeout) {
    cmdTimeout_ = timeout;
}

int AtParserImpl::write(const char* data, size_t size) {
    if (!checkStatus(Status::WRITE_CMD_DATA)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (!checkStatus(Status::CMD_NOT_EMPTY)) {
        if (!checkStatus(Status::END_OF_LINE)) {
            PARSER_CHECK(processInput(nullptr, 0, ReadFlag::STOP_AT_LINE_END, nullptr, &cmdTimeout_));
        }
        newLogLine(true);
    }
    PARSER_CHECK(write(data, &size, &cmdTimeout_));
    if (size > 0) {
        setStatus(Status::CMD_NOT_EMPTY);
        const size_t n = std::min(size, CMD_PREFIX_BUF_SIZE - cmdPrefixSize_);
        memcpy(cmdPrefix_ + cmdPrefixSize_, data, n);
        cmdPrefixSize_ += n;
        writeLogLine(data, size);
    }
    return size;
}

int AtParserImpl::read(char* data, size_t size) {
    return 0;
}

int AtParserImpl::readLine(char* data, size_t size) {
    return 0;
}

int AtParserImpl::readResult(int* errorCode) {
    return 0;
}

int AtParserImpl::addUrcHandler(const char* prefix, UrcHandlerFn handler, void* data) {
    const size_t prefixSize = strlen(prefix);
    if (prefixSize == 0 || prefixSize > INPUT_BUF_SIZE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    removeUrcHandler(prefix);
    UrcHandler h = {};
    h.prefix = prefix;
    h.prefixSize = prefixSize;
    h.callback = handler;
    h.data = data;
    if (!urcHandlers_.append(std::move(h))) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return 0;
}

void AtParserImpl::removeUrcHandler(const char* prefix) {
    for (int i = 0; i < urcHandlers_.size(); ++i) {
        if (strcmp(urcHandlers_.at(i).prefix, prefix) == 0) {
            urcHandlers_.removeAt(i);
            break;
        }
    }
}

int AtParserImpl::processUrc(unsigned timeout) {
    if (!checkStatus(Status::READY)) {
        return SYSTEM_ERROR_BUSY; // Do not cancel current command
    }
    if (checkStatus(Status::WRITE_CMD_TERM)) {
        PARSER_CHECK(writeCmdTerm(&timeout));
        if (checkStatus(Status::WRITE_CMD_TERM)) {
            return 0; // Non-blocking write did not complete
        }
    }
    unsigned urcCount = 0;
    PARSER_CHECK(processInput(nullptr, 0, 0, &urcCount, &timeout));
    return urcCount;
}

bool AtParserImpl::endOfLine() {
    return checkStatus(Status::END_OF_LINE);
}

void AtParserImpl::echoEnabled(bool enabled) {
    // The echo setting can't be changed if there's a command being processed
    if (checkStatus(Status::READY)) {
        conf_.echoEnabled(enabled);
    }
}

void AtParserImpl::reset() {
    bufPos_ = 0;
    cmdPrefixSize_ = 0;
    cmdTimeout_ = 0;
    cmdTermOffs_ = 0;
#if AT_COMMAND_LOG_ENABLED
    logLineSize_ = 0;
#endif
    status_ = Status::READY;
}

int AtParserImpl::processInput(char* data, size_t size, unsigned flags, unsigned* urcCount, unsigned* timeout) {
    bool readMore = false;
    for (;;) {
        if (readMore) {
            PARSER_CHECK(this->readMore(timeout));
            readMore = false;
        }
        if (checkStatus(Status::CHECK_RESULT)) {
            const auto result = findResultCode(buf_, bufPos_);
            if (result) {
                if (bufPos_ <= result->strSize) {
                    readMore = true;
                    continue;
                }
                const char c = buf_[result->strSize];
                if (result->val == Result::CME_ERROR || result->val == Result::CMS_ERROR) {
                    if (c != ':') {
                        result_ = result->val;
                        setStatus(Status::HAS_RESULT);
                    }
                } else {
                    if (isLineBreak(c)) {
                        result_ = result->val;
                        setStatus(Status::HAS_RESULT);
                    }
                }
            } else {
                clearStatus(Status::CHECK_RESULT);
            }
        }
        if (checkStatus(Status::CHECK_URC)) {
            const auto handler = findUrcHandler(buf_, bufPos_);
            if (handler) {
                if (bufPos_ < handler->prefixSize) {
                    readMore = true;
                    continue;
                }
                AtResponseReader reader(this);
                setStatus(Status::URC_HANDLER);
                SCOPE_GUARD({
                    clearStatus(Status::URC_HANDLER);
                });
                PARSER_CHECK(handler->callback(&reader, handler->prefix, handler->data));
            } else {
                clearStatus(Status::CHECK_URC);
            }
        }
        if (checkStatus(Status::CHECK_ECHO)) {
            const size_t n = std::min(bufPos_, cmdPrefixSize_);
            if (memcmp(buf_, cmdPrefix_, n) != 0) {
                clearStatus(Status::CHECK_ECHO);
            } else if (n == cmdPrefixSize_) {
                setStatus(Status::IS_ECHO);
            }
        }
    }
}

int AtParserImpl::writeCmdTerm(unsigned* timeout) {
    size_t n = cmdTermSize_ - cmdTermOffs_;
    const int ret = write(cmdTerm_ + cmdTermOffs_, &n, timeout);
    cmdTermOffs_ += n;
    if (cmdTermOffs_ == cmdTermSize_) {
        clearStatus(Status::WRITE_CMD_TERM);
    }
    return ret;
}

int AtParserImpl::readMore(unsigned* timeout) {
    assert(bufPos_ < INPUT_BUF_SIZE);
    const auto strm = conf_.stream();
    size_t n = 0;
    for (;;) {
        n = PARSER_CHECK(strm->read(buf_ + bufPos_, INPUT_BUF_SIZE - bufPos_));
        if (n > 0 || (timeout && *timeout == 0)) {
            break;
        }
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout) {
            waitTimeout = *timeout; // Use global timeout
        }
        auto t = millis();
        PARSER_CHECK(strm->waitEvent(Stream::READABLE, waitTimeout));
        if (timeout) {
            t = millis() - t;
            if (t >= *timeout) {
                return error(SYSTEM_ERROR_TIMEOUT);
            }
            *timeout -= t;
        }
    }
    bufPos_ += n;
    return n;
}

int AtParserImpl::write(const char* data, size_t* size, unsigned* timeout) {
    const auto strm = conf_.stream();
    const auto end = data + *size;
    auto t = millis();
    *size = 0;
    for (;;) {
        const size_t n = PARSER_CHECK(strm->write(data, end - data));
        data += n;
        *size += n;
        if (data == end || (timeout && *timeout == 0)) {
            break;
        }
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout && (waitTimeout == 0 || *timeout < waitTimeout)) {
            waitTimeout = *timeout; // Use global timeout
        }
        PARSER_CHECK(strm->waitEvent(Stream::WRITABLE, waitTimeout));
        if (timeout) {
            const auto t2 = millis();
            t = t2 - t;
            if (t >= *timeout) {
                return error(SYSTEM_ERROR_TIMEOUT);
            }
            *timeout -= t;
            t = t2;
        }
    }
    return *size;
}

const AtParserImpl::ResultCode* AtParserImpl::findResultCode(const char* data, size_t size) {
    const ResultCode* result = nullptr;
    size_t maxSize = 0;
    for (size_t i = 0; i < RESULT_CODE_COUNT; ++i) {
        const ResultCode& r = RESULT_CODES[i];
        const size_t n = std::min(size, r.strSize);
        if (memcmp(data, r.str, n) == 0 && n >= maxSize) {
            result = &r;
            maxSize = n;
            if (n == size) {
                break;
            }
        }
    }
    return result;
}

const AtParserImpl::UrcHandler* AtParserImpl::findUrcHandler(const char* data, size_t size) {
    const UrcHandler* handler = nullptr;
    size_t maxSize = 0;
    for (size_t i = 0; i < (size_t)urcHandlers_.size(); ++i) {
        const UrcHandler& h = urcHandlers_.at(i);
        const size_t n = std::min(size, h.prefixSize);
        if (memcmp(data, h.prefix, n) == 0 && n >= maxSize) {
            handler = &h;
            maxSize = n;
            if (n == size) {
                break;
            }
        }
    }
    return handler;
}

#if AT_COMMAND_LOG_ENABLED

void AtParserImpl::newLogLine(bool isCmd) {
    logLine_[11] = isCmd ? '>' : '<';
    logLineSize_ = LOG_LINE_PREFIX_SIZE;
}

void AtParserImpl::writeLogLine(const char* data, size_t size) {
    size_t pos = 0;
    while (pos < size && logLineSize_ < LOG_LINE_BUF_SIZE) {
        char c = data[pos++];
        if (!isprint((unsigned char)c)) {
            c = '.';
        }
        logLine_[logLineSize_++] = c;
    }
    if (pos < size) {
        logLine_[LOG_LINE_BUF_SIZE - 1] = '~';
    }
}

void AtParserImpl::flushLogLine() {
    if (logLineSize_ > LOG_LINE_PREFIX_SIZE) {
        const auto t = millis();
        snprintf(logLine_, LOG_LINE_PREFIX_SIZE, "%010u", (unsigned)t);
        logLine_[10] = ' '; // Overwrite '\0'
        LOG_WRITE(TRACE, logLine_, logLineSize_);
        logLineSize_ = LOG_LINE_PREFIX_SIZE;
    }
}

#endif // AT_COMMAND_LOG_ENABLED

int AtParserImpl::error(int ret) {
    cancelCommand();
    return ret;
}

} // particle::detail

} // particle
