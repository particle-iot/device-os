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
 * Responses are typically processed in a line-by-line manner:
 *
 * ```cpp
 * void logResponse(AtResponseReader& resp) {
 *     while (resp.hasNextLine()) {
 *         const CString s = resp.readLine();
 *         if (s) {
 *             LOG(INFO, "%s", (const char*)s);
 *         }
 *     }
 * }
 * ```
 *
 * It is also possible to read the response data in blocks of arbitrary size, preserving line
 * break characters:
 *
 * ```cpp
 * while (resp.hasMoreData()) {
 *     char data[128];
 *     const int n = resp.read(data, sizeof(data));
 *     if (n > 0) {
 *         LOG_WRITE(INFO, data, n);
 *     }
 * }
 * ```
 *
 * If an error occurs while reading the response data, the reader object transitions into the failed
 * state, and all further operations on that object will fail. The result code of the first failed
 * operation can be retrieved using the `error()` method:
 *
 * ```cpp
 * if (!resp) {
 *     LOG(ERROR, "Error: %d", resp.error());
 * }
 * ```
 *
 * @see `AtResponse`
 * @see `AtParser::UrcHandlerFn`
 */
class AtResponseReader {
public:
    /**
     * Reads the response data.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @return Number of characters read, or a negative result code in case of an error.
     *
     * @see `readLine()`
     * @see `readAll()`
     * @see `scanf()`
     */
    int read(char* data, size_t size);
    /**
     * Reads the current line.
     *
     * This method reads up to `size` characters of the current line, discarding the rest of
     * the line. The output is always null-terminated, unless `size` is `0`.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `read()`
     * @see `readAll()`
     * @see `scanf()`
     */
    int readLine(char* data, size_t size);
    /**
     * Reads the current line.
     *
     * This method reads the entire current line, allocating a buffer for it dynamically.
     *
     * @return A string.
     *
     * @see `read()`
     * @see `readAll()`
     * @see `scanf()`
     */
    CString readLine();
    /**
     * Reads the entire response.
     *
     * This method reads up to `size` characters of the response data, discarding the rest of
     * the data. The output is always null-terminated, unless `size` is `0`.
     *
     * @param data Destination buffer.
     * @param size Buffer size.
     * @return Number of characters read (not including `\0`), or a negative result code in
     *         case of an error.
     *
     * @see `read()`
     * @see `readLine()`
     * @see `scanf()`
     */
    int readAll(char* data, size_t size);
    /**
     * Reads the entire response.
     *
     * This method reads the entire response data, allocating a buffer for it dynamically.
     *
     * @return A string.
     *
     * @see `read()`
     * @see `readLine()`
     * @see `scanf()`
     */
    CString readAll();
    /**
     * Reads and parses the current line.
     *
     * @param fmt scanf-style format string.
     * @param ... Output arguments.
     * @return Number of items matched and assigned, or a negative result code in case of an error.
     *
     * @see `vscanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `readAll()`
     */
    int scanf(const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
    /**
     * Reads and parses the current line.
     *
     * @param fmt scanf-style format string.
     * @param args Output arguments.
     * @return Number of items matched and assigned, or a negative result code in case of an error.
     *
     * @see `scanf()`
     * @see `read()`
     * @see `readLine()`
     * @see `readAll()`
     */
    int vscanf(const char* fmt, va_list args);
    /**
     * Skips the rest of the current line and sets the position to the beginning of the next line.
     *
     * @return Number of characters skipped, or a negative result code in case of an error.
     */
    int nextLine();
    /**
     * Returns `true` if there is another line available for reading.
     */
    bool hasNextLine();
    /**
     * Returns `true` if there is more data available for reading.
     */
    bool hasMoreData();
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
 * This class inherits all the methods of `AtResponseReader` and, additionally, provides a method
 * to read the final result code:
 *
 * ```cpp
 * bool processResponse(AtResponse& resp) {
 *     while (resp.hasNextLine()) {
 *         LOG(INFO, "%s", (const char*)resp.readLine());
 *     }
 *     const int r = response.readResult();
 *     return (r == AtResponse::OK);
 * }
 * ```
 *
 * @see `AtCommand::send()`
 * @see `AtParser::sendCommand()`
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
     * Returns the error code reported via the `CME ERROR` or `CMS ERROR` result code.
     *
     * @return Error code.
     *
     * @see 3GPP TS 27.005
     * @see 3GPP TS 27.007
     */
    int resultErrorCode() const;
    /**
     * Cancels the processing of the current AT command.
     */
    void reset();

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
