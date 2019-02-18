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

#define PARSER_CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
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

// Maximum number of AT command characters stored by the parser
const size_t CMD_BUF_SIZE = 128;

// Maximum number of response line characters stored by the parser
const size_t RESP_BUF_SIZE = 128;

class AtParserImpl {
public:
    explicit AtParserImpl(AtParserConfig conf);
    ~AtParserImpl();

    int newCommand();
    int sendCommand();
    void resetCommand();
    void commandTimeout(unsigned timeout);
    int write(const char* data, size_t size);

    int readResult(int* errorCode);
    int readLine(char* data, size_t size);
    int nextLine();
    int hasNextLine(bool* hasLine);
    bool atLineEnd() const;

    int addUrcHandler(const char* prefix, AtParser::UrcHandler handler, void* data);
    void removeUrcHandler(const char* prefix);
    int processUrc(unsigned timeout);

    void reset();

    void echoEnabled(bool enabled);
    void logEnabled(bool enabled);
    const AtParserConfig& config() const;

    static bool isConfigValid(const AtParserConfig& conf);

private:
    enum StatusFlag {
        READY = 0x0001, // Ready to send a command or process URCs
        WRITE_CMD = 0x0002, // Writing the command data
        FLUSH_CMD = 0x0004, // Writing the command terminator
        LINE_BEGIN = 0x0008, // The parser is at the beginning of the line
        LINE_END = 0x0010, // The end of the line is reached
        HAS_RESULT = 0x0020, // A final result code has been parsed
        HAS_ECHO = 0x0040, // The command echo has been parsed
        ECHO_ENABLED = 0x0080, // The echo is enabled
        URC_HANDLER = 0x0100 // An URC handler is running
    };

    enum ParseFlag {
        PARSE_RESULT = 0x01, // Parse a final result code
        PARSE_URC = 0x02, // Parse a known URC
        PARSE_ECHO = 0x04 // Parse the command echo
    };

    enum ParseResult {
        NO_MATCH, // No match found
        PARSED_RESULT, // Parsed a final result code
        PARSED_URC, // Parsed a known URC
        PARSED_ECHO, // Parsed the command echo
        READ_MORE // More data is needed
    };

    struct UrcHandler {
        const char* prefix; // Prefix string
        size_t prefixSize; // Size of the prefix string
        AtParser::UrcHandler callback; // Handler callback
        void* data; // User data
    };

    const char* const cmdTerm_; // Command terminator string
    const size_t cmdTermSize_; // Size of the command terminator string

    char buf_[INPUT_BUF_SIZE]; // Input buffer
    size_t bufPos_; // Number of bytes in the input buffer

    char cmdData_[CMD_BUF_SIZE]; // Command data
    size_t cmdSize_; // Size of the command data

    char respData_[RESP_BUF_SIZE]; // Response data
    size_t respSize_; // Size of the response data

    AtResponse::Result result_; // Final result code
    int errorCode_; // Error code reported via "+CME ERROR" or "+CMS ERROR"
    size_t cmdTermOffs_; // Number of characters of the command terminator written to the stream
    unsigned cmdTimeout_; // Command timeout
    unsigned status_; // Status flags

    Vector<UrcHandler> urcHandlers_; // URC handlers
    AtParserConfig conf_; // Parser settings

    int readRespLine(char* data, size_t size);
    int waitEcho();

    int parseLine(unsigned flags, unsigned* timeout);
    int parseResult();
    int parseUrc(const UrcHandler** handler);
    int parseEcho();

    int readLine(char* data, size_t size, unsigned* timeout);
    int nextLine(unsigned* timeout);
    int readMore(unsigned* timeout);

    int flushCommand(unsigned* timeout);
    int write(const char* data, size_t* size, unsigned* timeout);

    void setStatus(unsigned flags);
    void clearStatus(unsigned flags);
    unsigned checkStatus(unsigned flags) const;

    int error(int ret);
};

inline void AtParserImpl::commandTimeout(unsigned timeout) {
    cmdTimeout_ = timeout;
}

inline bool AtParserImpl::atLineEnd() const {
    return checkStatus(StatusFlag::LINE_END);
}

inline void AtParserImpl::echoEnabled(bool enabled) {
    conf_.echoEnabled(enabled);
}

inline void AtParserImpl::logEnabled(bool enabled) {
    conf_.logEnabled(enabled);
}

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

} // particle::detail

} // particle
