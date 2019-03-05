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

size_t appendToBuf(char* dest, size_t destSize, const char* src, size_t srcSize) {
    const size_t n = std::min(srcSize, destSize);
    memcpy(dest, src, n);
    return n;
}

void logCmdLine(const char* data, size_t size) {
    if (size > 0) {
        LOG(TRACE, "> %.*s", size, data);
    }
}

void logRespLine(const char* data, size_t size) {
    if (size > 0) {
        LOG(TRACE, "< %.*s", size, data);
    }
}

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

inline bool isNewline(char c) {
    return (c == '\r' || c == '\n');
}

size_t findNewline(const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (isNewline(data[i])) {
            return i;
        }
    }
    return size;
}

inline system_tick_t millis() {
    return HAL_Timer_Get_Milli_Seconds();
}

} // unnamed

AtParserImpl::AtParserImpl(AtParserConfig conf) :
        cmdTerm_(cmdTermStr(conf.commandTerminator())),
        cmdTermSize_(strlen(cmdTerm_)),
        conf_(std::move(conf)) {
    reset();
}

AtParserImpl::~AtParserImpl() {
}

int AtParserImpl::newCommand() {
    if (!checkStatus(StatusFlag::READY)) {
        return SYSTEM_ERROR_BUSY; // This error doesn't affect the current command
    }
    cmdTimeout_ = conf_.commandTimeout(); // Default command timeout
    if (conf_.echoEnabled()) {
        setStatus(StatusFlag::ECHO_ENABLED);
    } else {
        clearStatus(StatusFlag::ECHO_ENABLED);
    }
    if (!checkStatus(StatusFlag::FLUSH_CMD)) {
        cmdSize_ = 0;
    }
    clearStatus(StatusFlag::READY);
    setStatus(StatusFlag::WRITE_CMD);
    return 0;
}

int AtParserImpl::sendCommand() {
    if (!checkStatus(StatusFlag::WRITE_CMD)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (checkStatus(StatusFlag::FLUSH_CMD) || cmdSize_ == 0) {
        return error(SYSTEM_ERROR_NOT_ENOUGH_DATA);
    }
    clearStatus(StatusFlag::WRITE_CMD);
    setStatus(StatusFlag::FLUSH_CMD);
    cmdTermOffs_ = 0;
    PARSER_CHECK(flushCommand(&cmdTimeout_));
    return 0;
}

void AtParserImpl::resetCommand() {
    if (checkStatus(StatusFlag::WRITE_CMD)) {
        clearStatus(StatusFlag::WRITE_CMD);
        if (!checkStatus(StatusFlag::FLUSH_CMD) && cmdSize_ > 0) {
            // Even though the command is incomplete, it needs to be terminated properly in order
            // to keep the parser in a consistent state
            cmdTermOffs_ = 0;
            setStatus(StatusFlag::FLUSH_CMD);
        }
    }
    clearStatus(StatusFlag::HAS_RESULT | StatusFlag::HAS_ECHO);
    setStatus(StatusFlag::READY);
}

int AtParserImpl::write(const char* data, size_t size) {
    if (!checkStatus(StatusFlag::WRITE_CMD)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    // Make sure the previous command has been terminated
    if (checkStatus(StatusFlag::FLUSH_CMD)) {
        PARSER_CHECK(flushCommand(&cmdTimeout_));
        cmdSize_ = 0;
    }
    const int ret = write(data, &size, &cmdTimeout_);
    cmdSize_ += appendToBuf(cmdData_ + cmdSize_, CMD_BUF_SIZE - cmdSize_, data, size);
    if (ret < 0) {
        return error(ret);
    }
    return size;
}

int AtParserImpl::readResult(int* errorCode) {
    if (checkStatus(StatusFlag::READY)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (!checkStatus(StatusFlag::HAS_RESULT)) {
        if (checkStatus(StatusFlag::ECHO_ENABLED) && !checkStatus(StatusFlag::HAS_ECHO)) {
            PARSER_CHECK(waitEcho());
            setStatus(StatusFlag::HAS_ECHO);
        }
        for (;;) {
            if (checkStatus(StatusFlag::LINE_BEGIN)) {
                const int ret = PARSER_CHECK(parseLine(ParseFlag::PARSE_RESULT | ParseFlag::PARSE_URC, &cmdTimeout_));
                if (ret == ParseResult::PARSED_RESULT) {
                    break;
                }
            }
            PARSER_CHECK(nextLine(&cmdTimeout_));
        }
    }
    if (errorCode) {
        *errorCode = errorCode_;
    }
    return result_;
}

int AtParserImpl::readLine(char* data, size_t size) {
    int ret = 0;
    if (checkStatus(StatusFlag::URC_HANDLER)) {
        ret = readLine(data, size, nullptr /* timeout */);
    } else if (!checkStatus(StatusFlag::READY)) {
        ret = readRespLine(data, size);
    } else {
        ret = SYSTEM_ERROR_INVALID_STATE;
    }
    if (ret < 0) {
        error(ret);
    }
    return ret;
}

int AtParserImpl::nextLine() {
    if (checkStatus(StatusFlag::READY)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (checkStatus(StatusFlag::HAS_RESULT)) {
        return error(SYSTEM_ERROR_END_OF_STREAM);
    }
    if (checkStatus(StatusFlag::ECHO_ENABLED) && !checkStatus(StatusFlag::HAS_ECHO)) {
        PARSER_CHECK(waitEcho());
        setStatus(StatusFlag::HAS_ECHO);
    }
    for (;;) {
        if (checkStatus(StatusFlag::LINE_BEGIN)) {
            const int ret = CHECK(parseLine(ParseFlag::PARSE_RESULT | ParseFlag::PARSE_URC, &cmdTimeout_));
            if (ret == ParseResult::PARSED_RESULT) {
                return SYSTEM_ERROR_END_OF_STREAM;
            }
        }
        if (!checkStatus(StatusFlag::LINE_END)) {
            break;
        }
        CHECK(nextLine(&cmdTimeout_));
    }
    return 0;
}

int AtParserImpl::hasNextLine(bool* hasLine) {
    if (checkStatus(StatusFlag::READY)) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    if (checkStatus(StatusFlag::HAS_RESULT)) {
        *hasLine = false;
        return 0;
    }
    if (checkStatus(StatusFlag::ECHO_ENABLED) && !checkStatus(StatusFlag::HAS_ECHO)) {
        PARSER_CHECK(waitEcho());
        setStatus(StatusFlag::HAS_ECHO);
    }
    for (;;) {
        if (checkStatus(StatusFlag::LINE_BEGIN)) {
            const int ret = PARSER_CHECK(parseLine(ParseFlag::PARSE_RESULT | ParseFlag::PARSE_URC, &cmdTimeout_));
            if (ret == ParseResult::PARSED_RESULT) {
                setStatus(StatusFlag::HAS_RESULT);
                *hasLine = false;
                break;
            }
        }
        if (!checkStatus(StatusFlag::LINE_END)) {
            *hasLine = true;
            break;
        }
        PARSER_CHECK(nextLine(&cmdTimeout_));
    }
    return 0;
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
        return SYSTEM_ERROR_BUSY;
    }
    size_t urcCount = 0;
    for (;;) {
        if (!checkStatus(StatusFlag::LINE_BEGIN)) {
            PARSER_CHECK(nextLine(&timeout));
        }
        const int ret = PARSER_CHECK(parseLine(ParseFlag::PARSE_URC, &timeout));
        if (ret == ParseResult::PARSED_URC) {
            ++urcCount;
            break;
        }
        PARSER_CHECK(readLine(nullptr, 0, &timeout));
    }
    return urcCount;
}

void AtParserImpl::reset() {
    bufPos_ = 0;
    cmdSize_ = 0;
    cmdTimeout_ = 0;
    cmdTermOffs_ = 0;
    respSize_ = 0;
    errorCode_ = 0;
    result_ = AtResponse::OK;
    status_ = StatusFlag::READY | StatusFlag::LINE_BEGIN;
}

bool AtParserImpl::isConfigValid(const AtParserConfig& conf) {
    return (conf.stream() != nullptr && conf.commandTimeout() > 0 && conf.streamTimeout() > 0);
}

int AtParserImpl::readRespLine(char* data, size_t size) {
    if (checkStatus(StatusFlag::HAS_RESULT)) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    if (checkStatus(StatusFlag::ECHO_ENABLED) && !checkStatus(StatusFlag::HAS_ECHO)) {
        CHECK(waitEcho());
        setStatus(StatusFlag::HAS_ECHO);
    }
    size_t bytesRead = 0;
    for (;;) {
        if (checkStatus(StatusFlag::LINE_BEGIN)) {
            const int ret = CHECK(parseLine(ParseFlag::PARSE_RESULT | ParseFlag::PARSE_URC, &cmdTimeout_));
            if (ret == ParseResult::PARSED_RESULT) {
                return SYSTEM_ERROR_END_OF_STREAM;
            }
        }
        if (!checkStatus(StatusFlag::LINE_END)) {
            bytesRead = CHECK(readLine(data, size, &cmdTimeout_));
            break;
        }
        CHECK(nextLine(&cmdTimeout_));
    }
    return bytesRead;
}

int AtParserImpl::waitEcho() {
    if (!checkStatus(StatusFlag::LINE_BEGIN)) {
        CHECK(nextLine(&cmdTimeout_));
    }
    for (;;) {
        const int ret = CHECK(parseLine(ParseFlag::PARSE_ECHO | ParseFlag::PARSE_URC, &cmdTimeout_));
        CHECK(nextLine(&cmdTimeout_));
        if (ret == ParseResult::PARSED_ECHO) {
            break;
        }
    }
    return 0;
}

int AtParserImpl::parseLine(unsigned flags, unsigned* timeout) {
    int ret = ParseResult::NO_MATCH;
    for (;;) {
        if (flags & ParseFlag::PARSE_RESULT) {
            // Check if the line contains a final result code
            ret = parseResult();
            if (ret == ParseResult::NO_MATCH) {
                flags &= ~ParseFlag::PARSE_RESULT;
            }
        } else if (flags & ParseFlag::PARSE_ECHO) {
            // Check if the line contains the command echo
            ret = parseEcho();
            if (ret == ParseResult::NO_MATCH) {
                flags &= ~ParseFlag::PARSE_ECHO;
            }
        } else if (flags & ParseFlag::PARSE_URC) {
            // Check if the line contains a known URC
            const UrcHandler* h = nullptr;
            ret = parseUrc(&h);
            if (ret == ParseResult::NO_MATCH) {
                flags &= ~ParseFlag::PARSE_URC;
            } else if (ret == ParseResult::PARSED_URC && h->callback) {
                // Read the line recursively
                AtResponseReader reader(this);
                setStatus(StatusFlag::URC_HANDLER);
                const int r = h->callback(&reader, h->prefix, h->data);
                clearStatus(StatusFlag::URC_HANDLER);
                if (r < 0) {
                    LOG(ERROR, "URC handler error: %d", r);
                }
            }
        }
        if (ret == ParseResult::READ_MORE) {
            CHECK(readMore(timeout));
        } else if (ret != ParseResult::NO_MATCH) {
            const auto logEnabled = conf_.logEnabled();
            if (ret == ParseResult::PARSED_ECHO) {
                conf_.logEnabled(false); // Do not log the command echo
            }
            // Read and discard remaining characters of the line
            const int r = readLine(nullptr, 0, timeout);
            conf_.logEnabled(logEnabled);
            CHECK(r);
            break;
        } else if (!flags) {
            break;
        }
    }
    return ret;
}

int AtParserImpl::parseResult() {
    if (bufPos_ == 0) {
        return ParseResult::READ_MORE;
    }
    // Look for a result code that matches the buffer contents
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
        return ParseResult::NO_MATCH;
    }
    if (bufPos_ < r->strSize + 1) {
        return ParseResult::READ_MORE;
    }
    char c = buf_[r->strSize]; // Separator character
    if (r->val == AtResponse::CME_ERROR || r->val == AtResponse::CMS_ERROR) {
        // "+CME ERROR" or "+CMS ERROR" should be followed by ':'
        if (c != ':') {
            return ParseResult::NO_MATCH;
        }
        if (bufPos_ < r->strSize + 2) {
            return ParseResult::READ_MORE;
        }
        const auto codeStr = buf_ + r->strSize + 1; // First character after ':'
        const size_t codeStrSize = bufPos_ - r->strSize - 1;
        const size_t n = findNewline(codeStr, codeStrSize);
        if (n == codeStrSize) {
            return ParseResult::READ_MORE;
        }
        if (n == 0) {
            return ParseResult::NO_MATCH; // Malformed result code line
        }
        c = codeStr[n]; // Newline character
        codeStr[n] = '\0';
        char* end = nullptr;
        const auto code = strtol(codeStr, &end, 10);
        codeStr[n] = c; // Restore newline character
        if (end == codeStr + n) {
            errorCode_ = code;
        } else {
            // Error code is not a number, use some generic code
            errorCode_ = 0; // Phone failure
        }
    } else {
        // Any other result code should be followed by a newline character
        if (!isNewline(c)) {
            return ParseResult::NO_MATCH;
        }
        errorCode_ = 0;
    }
    result_ = r->val;
    return ParseResult::PARSED_RESULT;
}

int AtParserImpl::parseUrc(const UrcHandler** handler) {
    if (bufPos_ == 0) {
        return ParseResult::READ_MORE;
    }
    // Look for an URC prefix that matches the buffer contents
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
        return ParseResult::NO_MATCH;
    }
    if (bufPos_ < h->prefixSize) {
        return ParseResult::READ_MORE;
    }
    *handler = h;
    return ParseResult::PARSED_URC;
}

int AtParserImpl::parseEcho() {
    if (bufPos_ == 0) {
        return ParseResult::READ_MORE;
    }
    // Check if the command line matches the buffer contents
    size_t n = std::min(bufPos_, cmdSize_);
    if (memcmp(buf_, cmdData_, n) != 0) {
        return ParseResult::NO_MATCH;
    }
    n = std::min(cmdSize_, INPUT_BUF_SIZE);
    if (bufPos_ < n) {
        return ParseResult::READ_MORE;
    }
    return ParseResult::PARSED_ECHO;
}

int AtParserImpl::readLine(char* data, size_t size, unsigned* timeout) {
    size_t bytesRead = 0;
    for (;;) {
        size_t n = findNewline(buf_, bufPos_);
        if (data && n > size) {
            n = size;
        }
        if (n > 0) {
            clearStatus(StatusFlag::LINE_BEGIN);
            respSize_ += appendToBuf(respData_ + respSize_, RESP_BUF_SIZE - respSize_, buf_, n);
            if (data) {
                memcpy(data, buf_, n);
                data += n;
                size -= n;
            }
            bytesRead += n;
            bufPos_ -= n;
        }
        if (bufPos_ > 0) {
            memmove(buf_, buf_ + n, bufPos_);
            if (isNewline(buf_[0])) {
                setStatus(StatusFlag::LINE_END);
                if (conf_.logEnabled()) {
                    logRespLine(respData_, respSize_);
                }
                respSize_ = 0;
            }
            break;
        }
        CHECK(readMore(timeout));
    }
    return bytesRead;
}

int AtParserImpl::nextLine(unsigned* timeout) {
    size_t bytesRead = 0;
    for (;;) {
        size_t n = findNewline(buf_, bufPos_);
        respSize_ += appendToBuf(respData_ + respSize_, RESP_BUF_SIZE - respSize_, buf_, n);
        if (n < bufPos_) {
            setStatus(StatusFlag::LINE_END);
            if (conf_.logEnabled()) {
                logRespLine(respData_, respSize_);
            }
            respSize_ = 0;
            do {
                ++n;
            } while (n < bufPos_ && isNewline(buf_[n]));
        }
        if (n > 0) {
            clearStatus(StatusFlag::LINE_BEGIN);
            bytesRead += n;
            bufPos_ -= n;
        }
        if (bufPos_ > 0) {
            memmove(buf_, buf_ + n, bufPos_);
        } else {
            CHECK(readMore(timeout));
        }
        if (checkStatus(StatusFlag::LINE_END) && !isNewline(buf_[0])) {
            clearStatus(StatusFlag::LINE_END);
            setStatus(StatusFlag::LINE_BEGIN);
            break;
        }
    }
    return bytesRead;
}

int AtParserImpl::readMore(unsigned* timeout) {
    assert(bufPos_ < INPUT_BUF_SIZE);
    const auto strm = conf_.stream();
    size_t bytesRead = 0;
    for (;;) {
        bytesRead = CHECK(strm->read(buf_ + bufPos_, INPUT_BUF_SIZE - bufPos_));
        if (bytesRead > 0) {
            break;
        }
        // The global timeout overrides the stream timeout
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout) {
            if (*timeout == 0) {
                return SYSTEM_ERROR_WOULD_BLOCK; // Non-blocking reading
            }
            waitTimeout = *timeout;
        }
        auto t = millis();
        CHECK(strm->waitEvent(Stream::READABLE, waitTimeout));
        if (timeout) {
            t = millis() - t;
            if (t >= *timeout) {
                return SYSTEM_ERROR_TIMEOUT;
            }
            *timeout -= t;
        }
    }
    bufPos_ += bytesRead;
    return bytesRead;
}

int AtParserImpl::flushCommand(unsigned* timeout) {
    if (!checkStatus(StatusFlag::FLUSH_CMD)) {
        return 0;
    }
    assert(cmdSize_ > 0);
    size_t n = cmdTermSize_ - cmdTermOffs_;
    const int ret = write(cmdTerm_ + cmdTermOffs_, &n, timeout);
    cmdTermOffs_ += n;
    if (cmdTermOffs_ == cmdTermSize_) {
        clearStatus(StatusFlag::FLUSH_CMD);
        if (conf_.logEnabled()) {
            logCmdLine(cmdData_, cmdSize_);
        }
    }
    return ret;
}

int AtParserImpl::write(const char* data, size_t* size, unsigned* timeout) {
    const auto strm = conf_.stream();
    const auto end = data + *size;
    auto t = millis();
    // The number of bytes gets reported to the calling code even if the operation fails
    *size = 0;
    for (;;) {
        const size_t n = CHECK(strm->write(data, end - data));
        *size += n;
        data += n;
        if (data == end) {
            break;
        }
        // Use the shortest of the global and stream timeouts
        unsigned waitTimeout = conf_.streamTimeout();
        if (timeout) {
            if (*timeout == 0) {
                return SYSTEM_ERROR_WOULD_BLOCK; // Non-blocking writing
            } else if (*timeout < waitTimeout) {
                waitTimeout = *timeout;
            }
        }
        CHECK(strm->waitEvent(Stream::WRITABLE, waitTimeout));
        if (timeout) {
            const auto t2 = millis();
            t = t2 - t;
            if (t >= *timeout) {
                return SYSTEM_ERROR_TIMEOUT;
            }
            *timeout -= t;
            t = t2;
        }
    }
    return *size;
}

int AtParserImpl::error(int ret) {
    if (!checkStatus(StatusFlag::READY)) {
        resetCommand();
    }
    return ret;
}

} // particle::detail

} // particle
