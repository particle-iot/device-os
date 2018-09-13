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

#pragma once

#include "at_parser.h"
#include "at_response.h"

#include "timer_hal.h"

#include "spark_wiring_vector.h"

#ifndef AT_COMMAND_LOG_ENABLED
#define AT_COMMAND_LOG_ENABLED 1
#endif

#define PARSER_CHECK(_expr) \
        ({ \
            const int _ret = _expr; \
            if (_ret < 0) { \
                return this->error(_ret); \
            } \
            _ret; \
        })

namespace particle {

namespace detail {

using spark::Vector;

// Size of the intermediate buffer for received data
const size_t INPUT_BUF_SIZE = 64;

// Number of initial characters of an AT command stored by the parser
const size_t CMD_PREFIX_BUF_SIZE = 64;

// Maximum size of an AT command or response line that can be written to the log
const size_t LOG_LINE_BUF_SIZE = 128;

static_assert(CMD_PREFIX_BUF_SIZE <= INPUT_BUF_SIZE, "Input buffer is too small");

class AtParserImpl {
public:
    explicit AtParserImpl(AtParserConfig conf);
    ~AtParserImpl();

    int newCommand();
    int sendCommand();
    void cancelCommand();
    void commandTimeout(unsigned timeout);
    int write(const char* data, size_t size);

    int read(char* data, size_t size);
    int readLine(char* data, size_t size, bool skipToLineEnd);
    int readResult(int* errorCode);

    bool atLineEnd() const;
    bool atResponseEnd() const;

    int addUrcHandler(const char* prefix, AtParser::UrcHandler handler, void* data);
    void removeUrcHandler(const char* prefix);
    int processUrc(unsigned timeout);

    void echoEnabled(bool enabled);

    const AtParserConfig& config() const;

    void reset();

private:
    enum StatusFlag {
        READY = 0x0001, // Ready to send a new command or process URCs
        CHECK_RESULT = 0x0002, // The line may contain a final result code
        CHECK_URC = 0x0004, // The line may contain a known URC
        CHECK_ECHO = 0x0008, // The line may contain the command echo
        WRITE_CMD = 0x0010, // Writing the command data
        FLUSH_CMD = 0x0020, // Writing the command terminator
        SKIP_RESP = 0x0040, // Skipping the response data
        WAIT_ECHO = 0x0080, // Waiting for the command echo
        IS_ECHO = 0x0100, // The line contains the command echo
        HAS_RESULT = 0x0200, // The result code has been parsed
        LINE_END = 0x0400, // The end of the line is reached
        URC_HANDLER = 0x0800 // An URC handler is running
    };

    enum ParserFlag {
        STOP_AT_LINE_END = 0x01,
        SKIP_LINE_BREAK = 0x02
    };

    enum ParserResult {
        MATCH = 0,
        NO_MATCH = 1,
        READ_MORE = 2
    };

    // URC handler data
    struct UrcHandler {
        const char* prefix; // Prefix string
        size_t prefixSize; // Size of the prefix string
        AtParser::UrcHandler callback; // Handler callback
        void* data; // User data
    };

    char buf_[INPUT_BUF_SIZE]; // Input buffer
    size_t bufPos_; // Number of bytes in the input buffer available for reading

    char cmdData_[CMD_PREFIX_BUF_SIZE]; // Prefix of the current command
    size_t cmdDataSize_; // Size of the command prefix

#if AT_COMMAND_LOG_ENABLED
    char logLine_[LOG_LINE_BUF_SIZE]; // Command or response line
    size_t logLineSize_; // Size of the command or response line
#endif
    const char* cmdTerm_; // Command terminator string
    size_t cmdTermSize_; // Size of the command terminator string
    size_t cmdTermOffs_; // Number of command terminator characters sent to the DCE

    AtResponse::Result result_; // Final result code
    int errorCode_; // Error code reported via "+CME ERROR" or "+CMS ERROR"
    unsigned cmdTimeout_; // Command timeout
    unsigned status_; // Status flags

    Vector<UrcHandler> urcHandlers_; // URC handlers
    AtParserConfig conf_; // Parser settings

    int processInput(unsigned flags, char* data, size_t size, unsigned* urcCount, unsigned* timeout);

    int parseLine();
    int parseResult(AtResponse::Result* result, int* errorCode);
    int parseUrc(const UrcHandler** handler);
    int parseEcho();

    int flushCommand(unsigned* timeout);

    int write(const char* data, size_t* size, unsigned* timeout);
    int readMore(unsigned* timeout);
    void shift(size_t size);

    void writeLogLine(const char* data, size_t size);
    void flushLogLine(bool isCmd);

    void setStatus(unsigned flags);
    void clearStatus(unsigned flags);
    unsigned checkStatus(unsigned flags) const;

    int error(int ret);
};

inline const AtParserConfig& AtParserImpl::config() const {
    return conf_;
}

inline void AtParserImpl::setStatus(unsigned flags) {
    status_ |= flags;
}

inline void AtParserImpl::clearStatus(unsigned flags) {
    status_ &= ~flags;
}

inline unsigned AtParserImpl::checkStatus(unsigned flags) const {
    return (status_ & flags);
}

#if !AT_COMMAND_LOG_ENABLED

inline void AtParserImpl::writeLogLine(const char* data, size_t size) {
}

inline void AtParserImpl::flushLogLine() {
}

#endif // !AT_COMMAND_LOG_ENABLED

} // particle::detail

} // particle
