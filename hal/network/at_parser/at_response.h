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

class AtParser;

namespace detail {

class AtParserImpl;

} // particle::detail

/**
 * Response data reader.
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
     * Parser-specific result codes returned by the `read()` method.
     *
     * @see `read()`
     */
    enum ReadResult {
        /**
         * The end of the response data is reached (`-1000`).
         */
        END_OF_RESPONSE = SYSTEM_ERROR_AT_PARSER_END_OF_RESPONSE,
        /**
         * The end of the current line of the response data is reached (`-1001`).
         */
        END_OF_LINE = SYSTEM_ERROR_AT_PARSER_END_OF_LINE
    };
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
         * Stop reading the response data when the end of the current line is reached.
         *
         * If this flag is set, `read()` will return `END_OF_LINE` when the end of the current line
         * is reached. The next call to `read()` with this flag set will read the response data
         * starting from the next line.
         *
         * @note Empty lines are skipped.
         */
        STOP_AT_LINE_END = 0x02,
        /**
         * Discard the remaining characters of the current line.
         *
         * This flag implies `STOP_AT_LINE_END`.
         */
        SKIP_REST_OF_LINE = 0x04
    };

    /**
     * Move-constructs a reader object.
     */
    AtResponseReader(AtResponseReader&& reader);
    /**
     * Destroys the reader object.
     */
    virtual ~AtResponseReader();
    /**
     * Reads the response data.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @param flags Combination of the flags defined by `ReadFlag`.
     * @return Number of bytes read, or a negative result code in case of an error.
     *
     * @see `readLine()`
     * @see `scanf()`
     * @see `ReadFlag`
     * @see `ReadResult`
     */
    int read(char* data, size_t size, unsigned flags = 0);
    /**
     * Reads a single line of the response data.
     *
     * This method is similar to `read()` invoked with the `SKIP_REST_OF_LINE` flag set.
     * In addition, this method ensures that the resulting string is null-terminated.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `read()`
     * @see `scanf()`
     * @see `ReadResult`
     */
    int readLine(char* data, size_t size);
    /**
     * Reads and parses a single line of the response data.
     *
     * @param fmt scanf-style format string.
     * @param ... Output arguments.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `vscanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `ReadResult`
     */
    int scanf(const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
    /**
     * Reads and parses a single line of the response data.
     *
     * @param fmt scanf-style format string.
     * @param args Output arguments.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `scanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `ReadResult`
     */
    int vscanf(const char* fmt, va_list args);
    /**
     * Returns the result code of the first failed operation.
     *
     * @return Result code.
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

    friend class AtParserImpl;
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
    int resultErrorCode();

private:
    using AtResponseReader::AtResponseReader;

    friend class AtParserImpl;
};

inline int AtResponseReader::error() const {
    return error_;
}

inline AtResponseReader::operator bool() const {
    return (error_ == 0);
}

} // particle
