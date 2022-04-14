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

#include <cstring>
#include <cstddef>
#include <cstdarg>

namespace particle {

namespace detail {

class AtParserImpl;

} // particle::detail

class AtResponse;

/**
 * AT command.
 *
 * This class implements the primary interface for formatting and sending AT commands to the DCE.
 *
 * The main methods used for writing the AT command data to the parser's stream are `write()` and
 * `printf()`. After writing the complete data to the stream, the client code needs to call `send()`
 * or `exec()` to make the parser send the command line terminator sequence to the DCE and start
 * receiving the response data:
 *
 * ```cpp
 * int initModem(AtParser& parser, bool echoOff) {
 *     auto cmd = parser.command();
 *     cmd.print("ATZ");
 *     if (echoOff) {
 *         cmd.print("E0");
 *     }
 *     return cmd.exec();
 * }
 * ```
 *
 * If an error occurs while sending an AT command, the AT command object transitions into the
 * failed state, and all further operations on that object will fail. The result code of the first
 * failed operation can be retrieved using the `error()` method:
 *
 * ```cpp
 * if (!cmd) {
 *     LOG(ERROR, "Error: %d", cmd.error());
 * }
 * ```
 *
 * @see `AtParser::command()`
 */
class AtCommand {
public:
    /**
     * Move-constructs an AT command object.
     */
    AtCommand(AtCommand&& cmd);
    /**
     * Destroys the AT command object.
     *
     * @note Destroying an active AT command object makes the parser flush the pending command
     *       data and discard the final result code received from the DCE.
     */
    ~AtCommand();
    /**
     * Writes a buffer to the stream.
     *
     * @param data Buffer data.
     * @param size Buffer size.
     * @return This AT command object.
     *
     * @see `print()`
     * @see `printf()`
     * @see `error()`
     */
    AtCommand& write(const char* data, size_t size);
    /**
     * Writes a null-terminated string to the stream.
     *
     * @param str String data.
     * @return This AT command object.
     *
     * @see `printf()`
     * @see `write()`
     * @see `error()`
     */
    AtCommand& print(const char* str);
    /**
     * Writes a formatted string to the stream.
     *
     * @param fmt printf-style format string.
     * @param ... Formatting arguments.
     * @return This AT command object.
     *
     * @see `vprintf()`
     * @see `print()`
     * @see `write()`
     * @see `error()`
     */
    AtCommand& printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    /**
     * Writes a formatted string to the stream.
     *
     * @param fmt printf-style format string.
     * @param args Formatting arguments.
     * @return This AT command object.
     *
     * @see `printf()`
     * @see `print()`
     * @see `write()`
     * @see `error()`
     */
    AtCommand& vprintf(const char* fmt, va_list args);
    /**
     * Sets the command timeout.
     *
     * @param timeout Timeout in milliseconds.
     * @return This AT command object.
     *
     * @see `AtParserConfig::commandTimeout()`
     */
    AtCommand& timeout(unsigned timeout);
    /**
     * Sends this AT command.
     *
     * @return Response object.
     */
    AtResponse send();
    /**
     * Sends this AT command and waits for the final result code.
     *
     * @return One of the values defined by `AtResponse::Result`, or a negative result code in
     *         case of an error.
     */
    int exec();
    /**
     * Cancels the processing of the current AT command.
     */
    void reset();
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
    AtCommand(const AtCommand&) = delete;
    AtCommand& operator=(const AtCommand&) = delete;

private:
    detail::AtParserImpl* parser_;
    int error_;

    explicit AtCommand(detail::AtParserImpl* parser);
    explicit AtCommand(int error);

    int error(int ret);

    friend class AtParser;
};

inline AtCommand& AtCommand::print(const char* str) {
    return write(str, strlen(str));
}

inline int AtCommand::error() const {
    return error_;
}

inline AtCommand::operator bool() const {
    return (error_ != 0);
}

} // particle
