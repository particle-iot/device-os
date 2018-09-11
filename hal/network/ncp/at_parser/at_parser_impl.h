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
const size_t CMD_PREFIX_BUF_SIZE = 32;

// Maximum size of an AT command or response line that can be written to the log
const size_t LOG_LINE_BUF_SIZE = 128;

class AtParserImpl {
public:
    using UrcHandlerFn = AtParser::UrcHandlerFn;
    using Result = AtResponse::Result;

    explicit AtParserImpl(AtParserConfig conf);
    ~AtParserImpl();

    int newCommand();
    int sendCommand();
    void cancelCommand();
    void commandTimeout(unsigned timeout);
    int write(const char* data, size_t size);

    int read(char* data, size_t size);
    int readLine(char* data, size_t size);
    int readResult(int* errorCode);

    int addUrcHandler(const char* prefix, UrcHandlerFn handler, void* data);
    void removeUrcHandler(const char* prefix);
    int processUrc(unsigned timeout);

    bool endOfLine();

    void echoEnabled(bool enabled);

    const AtParserConfig& config() const;

    void reset();

private:
    // Status flags
    enum Status {
        READY = 0x0001, // Ready to send a new command or process URCs
        CHECK_RESULT = 0x0002, // The current line may contain a final result code
        CHECK_URC = 0x0004, // The current line may contain a known URC
        CHECK_ECHO = 0x0008, // The current line may contain a command echo
        WRITE_CMD_DATA = 0x0010, // Writing the command data
        WRITE_CMD_TERM = 0x0020, // Writing the command terminator
        CMD_NOT_EMPTY = 0x0040, // At least one byte of the command data have been written
        URC_HANDLER = 0x0080, // An URC handler is running
        END_OF_LINE = 0x0100, // The end of the current line is reached
        HAS_RESULT = 0x0200, // The result code has been parsed
        IS_ECHO = 0x0400, // The current line is a command echo
    };

    enum ReadFlag {
        STOP_AT_LINE_END = 0x01, // Stop reading when the end of the current line is reached
        STOP_AT_NEXT_LINE = 0x02
    };

    struct ResultCode {
        Result val; // Result code value
        const char* str; // Result code string
        size_t strSize; // Size of the result code string
    };

    struct UrcHandler {
        const char* prefix; // Prefix string
        size_t prefixSize; // Size of the prefix string
        UrcHandlerFn callback; // Handler callback
        void* data; // User data
    };

    char buf_[INPUT_BUF_SIZE]; // Input buffer
    size_t bufPos_; // Number of bytes in the input buffer available for reading

    char cmdPrefix_[CMD_PREFIX_BUF_SIZE]; // Prefix of the current command
    size_t cmdPrefixSize_; // Size of the command prefix

#if AT_COMMAND_LOG_ENABLED
    char logLine_[LOG_LINE_BUF_SIZE]; // Command or response line
    size_t logLineSize_; // Size of the command or response line
#endif
    const char* cmdTerm_; // Command terminator string
    size_t cmdTermSize_; // Size of the command terminator string
    size_t cmdTermOffs_; // Number of command terminator characters sent to the DCE

    Result result_; // Last result code
    unsigned errorCode_; // Last error code (for "CME ERROR" and "CMS ERROR" result codes)
    unsigned cmdTimeout_; // Command timeout
    unsigned status_; // Status flags

    Vector<UrcHandler> urcHandlers_; // URC handlers
    AtParserConfig conf_; // Parser settings

    static const ResultCode RESULT_CODES[]; // Final result codes recognized by the parser
    static const size_t RESULT_CODE_COUNT; // Number of the final result codes

    int processInput(char* data, size_t size, unsigned flags, unsigned* urcCount, unsigned* timeout);

    int writeCmdTerm(unsigned* timeout);

    int readMore(unsigned* timeout);
    int write(const char* data, size_t* size, unsigned* timeout);

    static const ResultCode* findResultCode(const char* data, size_t size);
    const UrcHandler* findUrcHandler(const char* data, size_t size);

    void newLogLine(bool isCmd);
    void writeLogLine(const char* data, size_t size);
    void flushLogLine();

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

inline void AtParserImpl::newLogLine(bool isCmd) {
}

inline void AtParserImpl::writeLogLine(const char* data, size_t size) {
}

inline void AtParserImpl::flushLogLine() {
}

#endif // !AT_COMMAND_LOG_ENABLED

} // particle::detail

} // particle
