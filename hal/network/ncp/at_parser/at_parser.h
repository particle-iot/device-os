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

#include <memory>

namespace particle {

namespace detail {

class AtParserImpl;

} // particle::detail

class AtCommand;
class AtResponse;
class AtResponseReader;
class Stream;

/**
 * Command line terminator.
 */
enum class AtCommandTerminator {
    CR, ///< "\r"
    LF, ///< "\n"
    // Doesn't seem to be allowed by V.250, but some modems use it
    CRLF ///< "\r\n"
};

/**
 * AT parser settings.
 */
class AtParserConfig {
public:
    /**
     * Default command line terminator.
     *
     * @see `commandTerminator()`
     */
    static const auto DEFAULT_COMMAND_TERMINATOR = AtCommandTerminator::CR;
    /**
     * Default command timeout in milliseconds.
     *
     * @see `commandTimeout()`
     */
    static const auto DEFAULT_COMMAND_TIMEOUT = 90000;
    /**
     * Default stream timeout in milliseconds.
     *
     * @see `streamTimeout()`
     */
    static const auto DEFAULT_STREAM_TIMEOUT = 5000;
    /**
     * Default state of the echo handling.
     *
     * @see `echoEnabled()`
     */
    static const auto DEFAULT_ECHO_ENABLED = true;
    /**
     * Default state of the logging.
     *
     * @see `logEnabled()`
     */
    static const auto DEFAULT_LOG_ENABLED = true;

    /**
     * Constructs a settings object with all parameters set to their default values.
     */
    AtParserConfig();
    /**
     * Sets the stream used for communication with the DCE.
     *
     * @param strm Stream instance.
     * @return This settings object.
     */
    AtParserConfig& stream(Stream* strm);
    /**
     * Returns the stream used for communication with the DCE.
     *
     * @return Stream instance.
     */
    Stream* stream() const;
    /**
     * Sets the command line terminator.
     *
     * @param term Command line terminator.
     * @return This settings object.
     *
     * @see `DEFAULT_COMMAND_TERMINATOR`
     */
    AtParserConfig& commandTerminator(AtCommandTerminator term);
    /**
     * Returns the command line terminator.
     *
     * @return Command line terminator.
     *
     * @see `DEFAULT_COMMAND_TERMINATOR`
     */
    AtCommandTerminator commandTerminator() const;
    /**
     * Sets the default command timeout.
     *
     * @param timeout Timeout in milliseconds.
     * @return This settings object.
     *
     * @see `DEFAULT_COMMAND_TIMEOUT`
     */
    AtParserConfig& commandTimeout(unsigned timeout);
    /**
     * Returns the default command timeout.
     *
     * @return Timeout in milliseconds.
     *
     * @see `DEFAULT_COMMAND_TIMEOUT`
     */
    unsigned commandTimeout() const;
    /**
     * Sets the stream timeout.
     *
     * This method sets the inter-byte timeout for read and write operations.
     *
     * @param timeout Timeout in milliseconds.
     * @return This settings object.
     *
     * @see `DEFAULT_STREAM_TIMEOUT`
     */
    AtParserConfig& streamTimeout(unsigned timeout);
    /**
     * Returns the stream timeout.
     *
     * @return Timeout in milliseconds.
     *
     * @see `DEFAULT_STREAM_TIMEOUT`
     */
    unsigned streamTimeout() const;
    /**
     * Enables or disables the echo handling.
     *
     * @return This settings object.
     *
     * @see `DEFAULT_ECHO_ENABLED`
     */
    AtParserConfig& echoEnabled(bool enabled);
    /**
     * Returns `true` if the echo handling is enabled, or `false` otherwise.
     *
     * @see `DEFAULT_ECHO_ENABLED`
     */
    bool echoEnabled() const;
    /**
     * Enables or disables the logging of AT commands.
     *
     * @return This settings object.
     *
     * @see `DEFAULT_LOG_ENABLED`
     */
    AtParserConfig& logEnabled(bool enabled);
    /**
     * Returns `true` if the logging of AT commands is enabled, or `false` otherwise.
     *
     * @see `DEFAULT_LOG_ENABLED`
     */
    bool logEnabled() const;

private:
    Stream* strm_;
    AtCommandTerminator cmdTerm_;
    unsigned cmdTimeout_;
    unsigned strmTimeout_;
    bool echoEnabled_;
    bool logEnabled_;
};

/**
 * AT parser.
 */
class AtParser {
public:
    /**
     * The signature of a function invoked by the parser to process an unsolicited result code (URC).
     *
     * @param reader Response reader instance.
     * @param prefix URC prefix string.
     * @param data User data.
     *
     * @return `0` on success, or a negative result code in case of an error.
     *
     * @see `addUrcHandler()`
     */
    typedef int(*UrcHandler)(AtResponseReader* reader, const char* prefix, void* data);

    /**
     * Constructs a parser object.
     */
    AtParser();
    /**
     * Move-constructs a parser object.
     */
    AtParser(AtParser&& parser);
    /**
     * Destroys the parser object.
     */
    ~AtParser();
    /**
     * Initializes the parser.
     *
     * @param conf Parser settings.
     * @return `0` on success, or a negative result code in case of an error.
     */
    int init(AtParserConfig conf);
    /**
     * Uninitializes the parser.
     */
    void destroy();
    /**
     * Initiates an AT command.
     *
     * @return AT command object.
     *
     * @see `sendCommand()`
     * @see `execCommand()`
     */
    AtCommand command();
    /**
     * Formats and sends an AT command.
     *
     * This method is provided for convenience. The following two calls are equivalent:
     *
     * ```cpp
     * parser.sendCommand("ATE%d", 0);
     * parser.command().printf("ATE%d", 0).send();
     * ```
     *
     * @param fmt printf-style format string.
     * @param ... Formatting arguments.
     * @return Response object.
     *
     * @see `command()`
     * @see `execCommand()`
     */
    AtResponse sendCommand(const char* fmt, ...);
    /**
     * Formats and sends an AT command.
     *
     * This method is similar to `sendCommand(const char* fmt, ...)`, but it also overrides
     * the default command timeout. The following two calls are equivalent:
     *
     * ```cpp
     * parser.sendCommand(1000, "ATE%d", 0);
     * parser.command().timeout(1000).printf("ATE%d", 0).send();
     * ```
     *
     * @param timeout Timeout in milliseconds.
     * @param fmt printf-style format string.
     * @param ... Formatting arguments.
     * @return Response object.
     *
     * @see `command()`
     * @see `execCommand()`
     * @see `AtParserConfig::commandTimeout()`
     */
    AtResponse sendCommand(unsigned timeout, const char* fmt, ...);
    /**
     * Sends an AT command and waits for a final result code.
     *
     * This method is provided for convenience. The following two calls are equivalent:
     *
     * ```cpp
     * parser.execCommand("ATE%d", 0);
     * parser.command().printf("ATE%d", 0).exec();
     * ```
     *
     * @param fmt printf-style format string.
     * @param ... Formatting arguments.
     * @return One of the values defined by `AtResponse::Result`, or a negative result code in
     *         case of an error.
     *
     * @see `command()`
     * @see `sendCommand()`
     */
    int execCommand(const char* fmt, ...);
    /**
     * Sends an AT command and waits for a final result code.
     *
     * This method is similar to `execCommand(const char* fmt, ...)`, but it also overrides
     * the default command timeout. The following two calls are equivalent:
     *
     * ```cpp
     * parser.execCommand(1000, "ATE%d", 0);
     * parser.command().timeout(1000).printf("ATE%d", 0).exec();
     * ```
     *
     * @param timeout Timeout in milliseconds.
     * @param fmt printf-style format string.
     * @param ... Formatting arguments.
     * @return One of the values defined by `AtResponse::Result`, or a negative result code in
     *         case of an error.
     *
     * @see `command()`
     * @see `sendCommand()`
     * @see `AtParserConfig::commandTimeout()`
     */
    int execCommand(unsigned timeout, const char* fmt, ...);
    /**
     * Registers an URC handler.
     *
     * Only one handler can be associated with a given prefix string. A new handler registered for
     * the same prefix string replaces the existing handler.
     *
     * @param prefix URC prefix string.
     * @param handler Callback function.
     * @param data User data.
     * @return `0` on success, or a negative result code in case of an error.
     *
     * @see `removeUrcHandler()`
     */
    int addUrcHandler(const char* prefix, UrcHandler handler, void* data);
    /**
     * Removes a previously registered URC handler.
     *
     * @param prefix URC prefix string.
     *
     * @see `addUrcHandler()`
     */
    void removeUrcHandler(const char* prefix);
    /**
     * Processes URCs.
     *
     * This method needs to be called periodically in order to process pending URCs when there are
     * no active AT commands.
     *
     * @param timeout Maximum time in milliseconds to spend waiting for URCs. If this argument is
     *        set to `0` the parser will process URCs in a non-blocking manner.
     *
     * @return Number of URCs processed, or a negative result code in case of an error.
     */
    int processUrc(unsigned timeout = 0);
    /**
     * Resets the parser state.
     */
    void reset();
    /**
     * Enables or disables the echo handling.
     *
     * @see `config()`
     * @see `AtParserConfig::echoEnabled()`
     */
    void echoEnabled(bool enabled);
    /**
     * Enables or disables the logging of AT commands.
     *
     * @see `config()`
     * @see `AtParserConfig::logEnabled()`
     */
    void logEnabled(bool enabled);
    /**
     * Returns the parser settings.
     *
     * @return Settings object.
     */
    AtParserConfig config() const;

    // Instances of this class are non-copyable
    AtParser(const AtParser&) = delete;
    AtParser& operator=(const AtParser&) = delete;

private:
    std::unique_ptr<detail::AtParserImpl> p_;
};

inline AtParserConfig::AtParserConfig() :
        strm_(nullptr),
        cmdTerm_(DEFAULT_COMMAND_TERMINATOR),
        cmdTimeout_(DEFAULT_COMMAND_TIMEOUT),
        strmTimeout_(DEFAULT_STREAM_TIMEOUT),
        echoEnabled_(DEFAULT_ECHO_ENABLED),
        logEnabled_(DEFAULT_LOG_ENABLED) {
}

inline AtParserConfig& AtParserConfig::stream(Stream* strm) {
    strm_ = strm;
    return *this;
}

inline Stream* AtParserConfig::stream() const {
    return strm_;
}

inline AtParserConfig& AtParserConfig::commandTerminator(AtCommandTerminator term) {
    cmdTerm_ = term;
    return *this;
}

inline AtCommandTerminator AtParserConfig::commandTerminator() const {
    return cmdTerm_;
}

inline AtParserConfig& AtParserConfig::commandTimeout(unsigned timeout) {
    cmdTimeout_ = timeout;
    return *this;
}

inline unsigned AtParserConfig::commandTimeout() const {
    return cmdTimeout_;
}

inline AtParserConfig& AtParserConfig::streamTimeout(unsigned timeout) {
    strmTimeout_ = timeout;
    return *this;
}

inline unsigned AtParserConfig::streamTimeout() const {
    return strmTimeout_;
}

inline AtParserConfig& AtParserConfig::echoEnabled(bool enabled) {
    echoEnabled_ = enabled;
    return *this;
}

inline bool AtParserConfig::echoEnabled() const {
    return echoEnabled_;
}

inline AtParserConfig& AtParserConfig::logEnabled(bool enabled) {
    logEnabled_ = enabled;
    return *this;
}

inline bool AtParserConfig::logEnabled() const {
    return logEnabled_;
}

} // particle
