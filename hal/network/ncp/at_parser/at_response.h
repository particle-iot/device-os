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

#include <cstddef>
#include <cstdarg>

#include "system_error.h"

namespace particle {

namespace detail {

class AtParserImpl;

} // particle::detail

class AtParser;
class AtCommand;
class CString;

/**
 * Response reader.
 *
 * Instances of this class are used to read and parse AT command responses and unsolicited result
 * codes (URCs).
 *
 * @see `AtResponse`
 * @see `AtParser::UrcHandlerFn`
 */
class AtResponseReader {
public:
    /**
     * Flags controlling the behavior of the `read()` method.
     *
     * @see `read()`
     */
    enum ReadFlag {
        /**
         * Read the response data in the binary mode.
         *
         * @note Reading the response data with this flag set can get the parser into an inconsistent
         *       state. Use this flag only when the number of bytes to be read is known in advance.
         */
        BINARY_DATA = 0x01,
        /**
         * Stop reading when the the end of the current line is reached.
         *
         * @see `atLineEnd()`
         */
        STOP_AT_LINE_END = 0x02,
        /**
         * Discard remaining characters of the current line.
         *
         * This flag implies `STOP_AT_LINE_END`.
         */
        DISCARD_REST_OF_LINE = 0x04
    };
    /**
     * Parser-specific result codes.
     *
     * @see `read()`
     * @see `readLine()`
     * @see `scanf()`
     */
    enum ReadResult {
        /**
         * The end of the response data is reached (`-1000`).
         */
        END_OF_RESPONSE = SYSTEM_ERROR_AT_PARSER_END_OF_RESPONSE
    };

    /**
     * Reads the response data.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @param flags Combination of the flags defined by `ReadFlag`.
     * @return Number of bytes read, or a negative result code in case of an error.
     *
     * @see `readLine()`
     * @see `readAll()`
     * @see `scanf()`
     * @see `ReadFlag`
     * @see `ReadResult`
     */
    int read(char* data, size_t size, unsigned flags = 0);
    /**
     * Reads a line of the response.
     *
     * This method is similar to `read()` invoked with the `DISCARD_REST_OF_LINE` flag set.
     * The output is always null-terminated, unless `size` is set to `0`.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `read()`
     * @see `readAll()`
     * @see `scanf()`
     * @see `ReadResult`
     */
    int readLine(char* data, size_t size);
    /**
     * Reads a line of the response.
     *
     * This method reads a single entire line of the response, allocating a buffer for it
     * dynamically.
     *
     * @return A string.
     *
     * @see `read()`
     * @see `readAll()`
     * @see `scanf()`
     */
    CString readLine();
    /**
     * Reads and parses a line of the response.
     *
     * @param fmt scanf-style format string.
     * @param ... Output arguments.
     * @return Number of items matched and assigned, or a negative result code in case of an error.
     *
     * @see `vscanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `readAll()`
     * @see `ReadResult`
     */
    int scanf(const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
    /**
     * Reads and parses a line of the response.
     *
     * @param fmt scanf-style format string.
     * @param args Output arguments.
     * @return Number of items matched and assigned, or a negative result code in case of an error.
     *
     * @see `scanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `readAll()`
     * @see `ReadResult`
     */
    int vscanf(const char* fmt, va_list args);
    /**
     * Returns `true` if the end of the current line is reached.
     */
    bool atLineEnd() const;
    /**
     * Returns the result code of the first failed operation.
     */
    int error() const;
    /**
     * Returns `false` if this object is in the failed state.
     *
     * @see `error()`
     */
    explicit operator bool() const;

    // Instances of this class are non-copyable
    AtResponseReader(const AtResponseReader&) = delete;
    AtResponseReader& operator=(const AtResponseReader&) = delete;

protected:
    detail::AtParserImpl* parser_;
    int error_;

    explicit AtResponseReader(detail::AtParserImpl* parser);
    explicit AtResponseReader(int error);
    AtResponseReader(AtResponseReader&& reader);
    virtual ~AtResponseReader();

    int readLine(char* buf, size_t size, size_t offs);
    int error(int ret);

    friend class detail::AtParserImpl;
};

/**
 * AT command response.
 *
 * @see `AtCommand::send()`
 * @see `AtParser::sendCommand()`
 * @see `AtResponseReader`
 */
class AtResponse: public AtResponseReader {
public:
    /**
     * Final result codes.
     */
    enum Result {
        OK = 0,
        ERROR = 1,
        BUSY = 2,
        NO_ANSWER = 3,
        NO_CARRIER = 4,
        NO_DIALTONE = 5,
        CME_ERROR = 6,
        CMS_ERROR = 7
    };

    /**
     * Move-constructs a response object.
     */
    AtResponse(AtResponse&& resp);
    /**
     * Destroys the response object.
     *
     * @note Destroying an active response object makes the parser read and discard the remaining
     *       response data sent by the DCE.
     */
    ~AtResponse();
    /**
     * Reads the entire response data.
     *
     * This method reads the entire response data, allocating a buffer for it dynamically.
     * The output doesn't include the result code.
     *
     * @return A string.
     *
     * @see `read()`
     * @see `readLine()`
     * @see `scanf()`
     */
    CString readAll();
    /**
     * Reads the final result code.
     *
     * @return One of the values defined by `Result`, or a negative result code in case of an error.
     */
    int readResult();
    /**
     * Returns the error code reported via the `CME_ERROR` or `CMS_ERROR` result code.
     *
     * @return Error code. 
     */
    int resultErrorCode() const;
    /**
     * Returns `true` if the end of the response data is reached.
     */
    bool atResponseEnd() const;

private:
    int resultErrorCode_;

    explicit AtResponse(detail::AtParserImpl* parser);
    explicit AtResponse(int error);

    friend class AtCommand;
};

inline int AtResponseReader::error() const {
    return error_;
}

inline AtResponseReader::operator bool() const {
    return (error_ == 0);
}

inline int AtResponse::resultErrorCode() const {
    return resultErrorCode_;
}

} // particle
