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
#include <cstdlib>
#include <cctype>
#include <cassert>

LOG_SOURCE_CATEGORY("ncp.at")

namespace particle {

namespace detail {

namespace {

struct ResultCode {
    AtResponse::Result val;
    const char* str;
    size_t strSize;
};

// Final result codes recognized by the parser
const ResultCode RESULT_CODES[] = {
    { AtResponse::OK, "OK", 2 },
    { AtResponse::ERROR, "ERROR", 5 },
    { AtResponse::BUSY, "BUSY", 4 },
    { AtResponse::NO_ANSWER, "NO ANSWER", 9 },
    { AtResponse::NO_CARRIER, "NO CARRIER", 10 },
    { AtResponse::NO_DIALTONE, "NO DIALTONE", 11 },
    { AtResponse::CME_ERROR, "+CME ERROR", 10 },
    { AtResponse::CMS_ERROR, "+CMS ERROR", 10 }
};

const size_t RESULT_CODE_COUNT = sizeof(RESULT_CODES) / sizeof(RESULT_CODES[0]);

const char* cmdTermStr(AtCommandTerminator term) {
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

bool findLineBreak(const char* data, size_t size, size_t* pos) {
    for (size_t i = 0; i < size; ++i) {
        if (isLineBreak(data[i])) {
            *pos = i;
            return true;
        }
    }
    return false;
}

inline system_tick_t millis() {
    return HAL_Timer_Get_Milli_Seconds();
}

} // unnamed

AtParserImpl::AtParserImpl(AtParserConfig conf) :
        conf_(std::move(conf)) {
    cmdTerm_ = cmdTermStr(conf_.commandTerminator());
    cmdTermSize_ = strlen(cmdTerm_);
    reset();
}

AtParserImpl::~AtParserImpl() {
}

int AtParserImpl::newCommand() {
    if (!checkStatus(StatusFlag::READY)) {
        return SYSTEM_ERROR_BUSY; // This error doesn't cancel the current command
    }
    cmdTimeout_ = conf_.commandTimeout(); // Default command timeout
    clearStatus(StatusFlag::READY);
    setStatus(StatusFlag::WRITE_CMD);
    return 0;
}

int AtParserImpl::sendCommand() {
    if (!checkStatus(StatusFlag::WRITE_CMD)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (cmdDataSize_ == 0) {
        return error(SYSTEM_ERROR_NOT_ENOUGH_DATA);
    }
    setStatus(StatusFlag::FLUSH_CMD);
    cmdTermOffs_ = 0;
    PARSER_CHECK(flushCommand(&cmdTimeout_));
    return 0;
}

void AtParserImpl::cancelCommand() {
    if (checkStatus(StatusFlag::WRITE_CMD)) {
        clearStatus(StatusFlag::WRITE_CMD);
        if (cmdDataSize_ > 0) {
            // We have to send the command terminator and skip the response if at least one byte of
            // the command data has been sent already
            setStatus(StatusFlag::FLUSH_CMD | StatusFlag::SKIP_RESP);
        }
    }
    if (checkStatus(StatusFlag::HAS_RESULT)) {
        clearStatus(StatusFlag::HAS_RESULT);
    } else if (!checkStatus(StatusFlag::READY)) {
        setStatus(StatusFlag::SKIP_RESP);
    }
    setStatus(StatusFlag::READY);
}

void AtParserImpl::commandTimeout(unsigned timeout) {
    cmdTimeout_ = timeout;
}

int AtParserImpl::write(const char* data, size_t size) {
    if (!checkStatus(StatusFlag::WRITE_CMD)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (cmdDataSize_ == 0 && !checkStatus(StatusFlag::LINE_END)) {
        // Finish reading the current line and flush the log buffer
        PARSER_CHECK(processInput(ParserFlag::STOP_AT_LINE_END, nullptr /* data */, 0 /* size */, nullptr /* urcCount */,
                &cmdTimeout_));
    }
    const int ret = write(data, &size, &cmdTimeout_);
    if (size > 0) {
        const size_t n = std::min(size, CMD_PREFIX_BUF_SIZE - cmdDataSize_);
        memcpy(cmdData_ + cmdDataSize_, data, n);
        cmdDataSize_ += n;
        writeLogLine(data, size);
    }
    if (ret < 0) {
        return error(ret);
    }
    return size;
}

int AtParserImpl::read(char* data, size_t size) {
    if (checkStatus(StatusFlag::READY) && !checkStatus(StatusFlag::URC_HANDLER)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    unsigned* timeout = nullptr;
    if (!checkStatus(StatusFlag::URC_HANDLER)) {
        timeout = &cmdTimeout_;
    }
    const size_t n = PARSER_CHECK(processInput(ParserFlag::STOP_AT_LINE_END, data, size, nullptr /* urcCount */,
            timeout));
    return n;
}

int AtParserImpl::readLine(char* data, size_t size, bool skipToLineEnd) {
    if (checkStatus(StatusFlag::READY) && !checkStatus(StatusFlag::URC_HANDLER)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    unsigned* timeout = nullptr;
    if (!checkStatus(StatusFlag::URC_HANDLER)) {
        timeout = &cmdTimeout_;
    }
    const size_t n = PARSER_CHECK(processInput(ParserFlag::STOP_AT_LINE_END, data, size, nullptr /* urcCount */,
            timeout));
    if (skipToLineEnd && !checkStatus(StatusFlag::LINE_END)) {
        // Skip the rest of the line
        PARSER_CHECK(processInput(ParserFlag::STOP_AT_LINE_END, nullptr /* data */, 0 /* size */, nullptr /* urcCount */,
                timeout));
    }
    return n;
}

int AtParserImpl::readResult(int* errorCode) {
    if (checkStatus(StatusFlag::READY)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    PARSER_CHECK(processInput(0 /* flags */, nullptr /* data */, 0 /* size */, nullptr /* urcCount */, &cmdTimeout_));
    *errorCode = errorCode_;
    return result_;
}

bool AtParserImpl::atLineEnd() const {
    return checkStatus(StatusFlag::LINE_END);
}

bool AtParserImpl::atResponseEnd() const {
    return checkStatus(StatusFlag::HAS_RESULT);
}

int AtParserImpl::addUrcHandler(const char* prefix, AtParser::UrcHandler handler, void* data) {
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
    if (!checkStatus(StatusFlag::READY)) {
        return SYSTEM_ERROR_BUSY; // This error doesn't cancel the current command
    }
    if (checkStatus(StatusFlag::FLUSH_CMD)) {
        PARSER_CHECK(flushCommand(&timeout));
        if (checkStatus(StatusFlag::FLUSH_CMD)) {
            return 0; // Non-blocking write did not complete
        }
    }
    unsigned urcCount = 0;
    PARSER_CHECK(processInput(0 /* flags */, nullptr /* data */, 0 /* size */, &urcCount, &timeout));
    return urcCount;
}

void AtParserImpl::echoEnabled(bool enabled) {
    // The echo setting can't be changed if there's an active command
    if (checkStatus(StatusFlag::READY)) {
        conf_.echoEnabled(enabled);
    }
}

void AtParserImpl::reset() {
    bufPos_ = 0;
    cmdDataSize_ = 0;
    cmdTimeout_ = 0;
    cmdTermOffs_ = 0;
#if AT_COMMAND_LOG_ENABLED
    logLineSize_ = 0;
#endif
    status_ = StatusFlag::READY | StatusFlag::LINE_END;
}

int AtParserImpl::processInput(unsigned flags, char* data, size_t size, unsigned* urcCount, unsigned* timeout) {
    bool readMore = false;
    size_t bytesRead = 0;
    for (;;) {
        if (readMore) {
            readMore = false;
            const size_t n = PARSER_CHECK(this->readMore(timeout));
            if (n == 0 && timeout && *timeout == 0) {
                break; // Non-blocking processing is requested
            }
        }
        int ret = parseLine();
        if (ret == ParserResult::READ_MORE) {
            readMore = true;
            continue;
        }
        if (checkStatus(StatusFlag::HAS_RESULT)) {
            if (checkStatus(StatusFlag::SKIP_RESP)) {
                clearStatus(StatusFlag::SKIP_RESP | StatusFlag::HAS_RESULT);
            } else {
                break;
            }
        }
    }
    return bytesRead;
}

int AtParserImpl::parseLine() {
    int ret = ParserResult::NO_MATCH;
    if (checkStatus(StatusFlag::CHECK_RESULT)) {
        int ret = PARSER_CHECK(parseResult(&result_, &errorCode_));
        if (ret == ParserResult::MATCH) {
            setStatus(StatusFlag::HAS_RESULT);
        } else if (ret == ParserResult::NO_MATCH) {
            clearStatus(StatusFlag::CHECK_RESULT);
        }
    } else if (checkStatus(StatusFlag::CHECK_ECHO)) {
        ret = PARSER_CHECK(parseEcho());
        if (ret == ParserResult::MATCH) {
            setStatus(StatusFlag::IS_ECHO);
        } else if (ret == ParserResult::NO_MATCH) {
            clearStatus(StatusFlag::CHECK_ECHO);
        }
    } else if (checkStatus(StatusFlag::CHECK_URC)) {
        const UrcHandler* h = nullptr;
        ret = PARSER_CHECK(parseUrc(&h));
        if (ret == ParserResult::MATCH) {
            clearStatus(StatusFlag::CHECK_URC);
            setStatus(StatusFlag::URC_HANDLER);
            SCOPE_GUARD({
                clearStatus(StatusFlag::URC_HANDLER);
            });
            AtResponseReader reader(this);
            PARSER_CHECK(h->callback(&reader, h->prefix, h->data));
        } else if (ret == ParserResult::NO_MATCH) {
            clearStatus(StatusFlag::CHECK_URC);
        }
    }
    return ret;
}

int AtParserImpl::parseResult(AtResponse::Result* result, int* errorCode) {
    if (bufPos_ == 0) {
        return ParserResult::READ_MORE;
    }
    const ResultCode* r = nullptr;
    size_t maxSize = 0;
    for (size_t i = 0; i < RESULT_CODE_COUNT; ++i) {
        const ResultCode& r2 = RESULT_CODES[i];
        const size_t n = std::min(bufPos_, r2.strSize);
        if (memcmp(buf_, r2.str, n) == 0 && n > maxSize) {
            r = &r2;
            maxSize = n;
        }
    }
    if (!r) {
        return ParserResult::NO_MATCH;
    }
    if (bufPos_ < r->strSize + 1) {
        return ParserResult::READ_MORE;
    }
    char c = buf_[r->strSize];
    if (r->val == AtResponse::CME_ERROR || r->val == AtResponse::CMS_ERROR) {
        if (c != ':') {
            return ParserResult::NO_MATCH;
        }
        if (bufPos_ < r->strSize + 2) {
            return ParserResult::READ_MORE;
        }
        const auto codeStr = buf_ + r->strSize + 1; // First character after ':'
        size_t pos = 0;
        if (!findLineBreak(codeStr, bufPos_ - r->strSize - 1, &pos)) {
            return ParserResult::READ_MORE;
        }
        if (pos == 0) {
            return ParserResult::NO_MATCH;
        }
        c = codeStr[pos];
        codeStr[pos] = '\0';
        char* end = nullptr;
        const auto code = strtol(codeStr, &end, 10);
        codeStr[pos] = c;
        if (end != codeStr + pos) {
            return ParserResult::NO_MATCH;
        }
        *errorCode = code;
        shift(end - buf_);
    } else {
        // All other result codes end with a line break character
        if (!isLineBreak(c)) {
            return ParserResult::NO_MATCH;
        }
        *errorCode = 0;
        shift(r->strSize);
    }
    *result = r->val;
    return ParserResult::MATCH;
}

int AtParserImpl::parseUrc(const UrcHandler** handler) {
    if (bufPos_ == 0) {
        return ParserResult::READ_MORE;
    }
    const UrcHandler* h = nullptr;
    size_t maxSize = 0;
    for (size_t i = 0; i < (size_t)urcHandlers_.size(); ++i) {
        const UrcHandler& h2 = urcHandlers_.at(i);
        const size_t n = std::min(bufPos_, h2.prefixSize);
        if (memcmp(buf_, h2.prefix, n) == 0 && n > maxSize) {
            h = &h2;
            maxSize = n;
        }
    }
    if (!h) {
        return ParserResult::NO_MATCH;
    }
    if (bufPos_ < h->prefixSize) {
        return ParserResult::READ_MORE;
    }
    *handler = h;
    return ParserResult::MATCH;
}

int AtParserImpl::parseEcho() {
    if (bufPos_ == 0) {
        return ParserResult::READ_MORE;
    }
    const size_t n = std::min(bufPos_, cmdDataSize_);
    if (memcmp(buf_, cmdData_, n) != 0) {
        return ParserResult::NO_MATCH;
    }
    if (bufPos_ < cmdDataSize_) {
        return ParserResult::READ_MORE;
    }
    shift(cmdDataSize_);
    return ParserResult::MATCH;
}

int AtParserImpl::flushCommand(unsigned* timeout) {
    size_t n = cmdTermSize_ - cmdTermOffs_;
    const int ret = write(cmdTerm_ + cmdTermOffs_, &n, timeout);
    cmdTermOffs_ += n;
    if (cmdTermOffs_ == cmdTermSize_) {
        clearStatus(StatusFlag::FLUSH_CMD);
        if (conf_.echoEnabled()) {
            setStatus(StatusFlag::WAIT_ECHO);
        }
        flushLogLine(true /* isCmd */);
    }
    return ret;
}

int AtParserImpl::write(const char* data, size_t* size, unsigned* timeout) {
    const auto strm = conf_.stream();
    const auto end = data + *size;
    auto t = millis();
    // The number of written bytes gets reported to the calling code even if the operation fails
    *size = 0;
    for (;;) {
        const size_t n = PARSER_CHECK(strm->write(data, end - data));
        data += n;
        *size += n;
        if (data == end || (timeout && *timeout == 0)) {
            break;
        }
        // When writing, we track both the stream and global timeouts
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout && (waitTimeout == 0 || *timeout < waitTimeout)) {
            waitTimeout = *timeout;
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

int AtParserImpl::readMore(unsigned* timeout) {
    assert(bufPos_ < INPUT_BUF_SIZE);
    const auto strm = conf_.stream();
    size_t n = 0;
    for (;;) {
        n = PARSER_CHECK(strm->read(buf_ + bufPos_, INPUT_BUF_SIZE - bufPos_));
        if (n > 0 || (timeout && *timeout == 0)) {
            break;
        }
        // When reading, we track either the stream or global timeout. Stream::waitEvent() will
        // wait indefinitely if the stream timeout is disabled (i.e. set to 0), and the global
        // timeout is not specified (timeout == nullptr)
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout) {
            waitTimeout = *timeout;
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

void AtParserImpl::shift(size_t size) {
    assert(size <= bufPos_);
    memmove(buf_, buf_ + size, bufPos_ - size);
    bufPos_ -= size;
}

#if AT_COMMAND_LOG_ENABLED

void AtParserImpl::writeLogLine(const char* data, size_t size) {
    for (size_t i = 0; i < size && logLineSize_ < LOG_LINE_BUF_SIZE; ++i, ++logLineSize_) {
        char c = data[i];
        if (!isprint((unsigned char)c)) {
            c = '.';
        }
        logLine_[logLineSize_] = c;
    }
}

void AtParserImpl::flushLogLine(bool isCmd) {
    if (logLineSize_ > 0) {
        size_t n = logLineSize_;
        if (n == LOG_LINE_BUF_SIZE) {
            n = LOG_LINE_BUF_SIZE - 1;
            logLine_[n - 1] = '~';
        }
        logLine_[n] = '\0';
        LOG(TRACE, "%s %s", isCmd ? ">" : "<", logLine_);
        logLineSize_ = 0;
    }
}

#endif // AT_COMMAND_LOG_ENABLED

int AtParserImpl::error(int ret) {
    cancelCommand();
    return ret;
}

} // particle::detail

} // particle
