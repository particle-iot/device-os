/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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
#include "c_string.h"
#include "at_parser.h"
#include "spark_wiring_vector.h"

namespace particle {

class AtServerConfig {
public:
    /**
     * Default command line terminator.
     * For requests this is usally <CR> for responses the default is <CR><LF>
     *
     * @see `commandTerminator()`
     */
    static const auto DEFAULT_COMMAND_TERMINATOR = AtCommandTerminator::CRLF;
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
     * Default logging category
     *
     * @see `logCategory()`
     */
    static constexpr auto DEFAULT_LOG_CATEGORY = "at.server";

    /**
     * Constructs a settings object with all parameters set to their default values.
     */
    AtServerConfig();
    /**
     * Sets the stream used for communication with the DCE.
     *
     * @param strm Stream instance.
     * @return This settings object.
     */
    AtServerConfig& stream(Stream* strm);
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
    AtServerConfig& commandTerminator(AtCommandTerminator term);
    /**
     * Returns the command line terminator.
     *
     * @return Command line terminator.
     *
     * @see `DEFAULT_COMMAND_TERMINATOR`
     */
    AtCommandTerminator commandTerminator() const;
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
    AtServerConfig& streamTimeout(unsigned timeout);
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
    AtServerConfig& echoEnabled(bool enabled);
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
    AtServerConfig& logEnabled(bool enabled);
    /**
     * Returns `true` if the logging of AT commands is enabled, or `false` otherwise.
     *
     * @see `DEFAULT_LOG_ENABLED`
     */
    bool logEnabled() const;
    /**
     * Sets the logging category.
     *
     * @return This settings object.
     *
     * @see `DEFAULT_LOG_CATEGORY`
     */
    AtServerConfig& logCategory(const char* category);
    /**
     * Returns the logging category.
     *
     * @return Logging category.
     *
     * @see `DEFAULT_LOG_CATEGORY`
     */
    const char* logCategory() const;

private:
    Stream* strm_;
    AtCommandTerminator cmdTerm_;
    unsigned strmTimeout_;
    bool echoEnabled_;
    bool logEnabled_;
    CString logCategory_;
};

class AtServer;

enum class AtServerCommandType {
    ANY,
    EXEC,
    READ,
    WRITE,
    TEST,
    ONE_INT_ARG,
    WILDCARD
};

class AtServerRequest {
public:
    AtServerRequest(AtServer* server, AtServerCommandType type, const char* data, size_t len, size_t dataOffset);
    ~AtServerRequest();

    enum Result {
        OK = 0,
        ERROR = 1,
        BUSY = 2,
        NO_ANSWER = 3,
        NO_CARRIER = 4,
        NO_DIALTONE = 5,
        CME_ERROR = 6,
        CMS_ERROR = 7,
        CONNECT = 8
    };

    CString readLine();
    int readLine(char* data, size_t size);

    // Argument data if type != ANY, otherwise full command line
    int read(char* data, size_t size);
    CString read();
    int scanf(const char* fmt, ...) __attribute__((format(scanf, 2, 3)));
    int vscanf(const char* fmt, va_list args);

    int sendResponse(const char* fmt, ...);
    int sendResponseV(const char* fmt, va_list args);
    int sendNewLine();
    int setFinalResponse(Result result);

    AtServer* server() const;

private:
    friend class AtServer;

    bool sentResponse_ = false;

    AtServer* server_;
    Result finalResponse_;
    AtServerCommandType type_;
    const char* buf_;
    size_t len_;
    size_t dataOffset_;
};

class AtServerCommandHandler {
public:
    typedef int(*ServerCommandHandler)(AtServerRequest* request, AtServerCommandType type, const char* command, void* data);
    AtServerCommandHandler() = delete;
    AtServerCommandHandler(AtServerCommandType type, const char* command, ServerCommandHandler handler, void* data);
    AtServerCommandHandler(AtServerCommandType type, const char* command, const char* response);
    AtServerCommandHandler(AtServerCommandType type, const char* command, CString response);

    AtServerCommandType type() const;
    CString command() const;
    ServerCommandHandler handler() const;
    void* data() const;

    virtual int handler(AtServerRequest* request, AtServerCommandType type, const char* command, void* data);

private:
    AtServerCommandType type_;
    CString command_;
    ServerCommandHandler handler_;
    void* data_;
    const char* cResponse_;
    CString response_;
};

class AtServer {
public:
    AtServer();
    ~AtServer();

    int init(const AtServerConfig& config);
    void destroy();
    int reset();

    int addCommandHandler(const AtServerCommandHandler& handler);
    int removeCommandHandler(const char* command, AtServerCommandType type = AtServerCommandType::ANY);

    int process();
    int processRequest();

    int write(const char* str, size_t len);
    int write(const char* str, size_t len, system_tick_t timeout);
    int writeNewLine();
    bool hasNextLine();

    int suspend();
    bool suspended() const;
    int resume();

    void logCmdLine(const char* data, size_t size) const;

    void logRespLine(const char* data, size_t size) const;

    AtServerConfig config() const;

private:
    static constexpr size_t DEFAULT_REQUEST_BUFFER_SIZE = 512;
    Vector<AtServerCommandHandler> handlers_;
    char requestBuf_[DEFAULT_REQUEST_BUFFER_SIZE];
    size_t bufPos_;
    size_t newLineOffset_;
    AtServerConfig conf_;
    bool suspended_;
};


inline AtServerConfig::AtServerConfig() :
        strm_(nullptr),
        cmdTerm_(DEFAULT_COMMAND_TERMINATOR),
        strmTimeout_(DEFAULT_STREAM_TIMEOUT),
        echoEnabled_(DEFAULT_ECHO_ENABLED),
        logEnabled_(DEFAULT_LOG_ENABLED) {
}

inline AtServerConfig& AtServerConfig::stream(Stream* strm) {
    strm_ = strm;
    return *this;
}

inline Stream* AtServerConfig::stream() const {
    return strm_;
}

inline AtServerConfig& AtServerConfig::commandTerminator(AtCommandTerminator term) {
    cmdTerm_ = term;
    return *this;
}

inline AtCommandTerminator AtServerConfig::commandTerminator() const {
    return cmdTerm_;
}

inline AtServerConfig& AtServerConfig::streamTimeout(unsigned timeout) {
    strmTimeout_ = timeout;
    return *this;
}

inline unsigned AtServerConfig::streamTimeout() const {
    return strmTimeout_;
}

inline AtServerConfig& AtServerConfig::echoEnabled(bool enabled) {
    echoEnabled_ = enabled;
    return *this;
}

inline bool AtServerConfig::echoEnabled() const {
    return echoEnabled_;
}

inline AtServerConfig& AtServerConfig::logEnabled(bool enabled) {
    logEnabled_ = enabled;
    return *this;
}

inline bool AtServerConfig::logEnabled() const {
    return logEnabled_;
}

inline AtServerConfig& AtServerConfig::logCategory(const char* category) {
    logCategory_ = category;
    return *this;
}

inline const char* AtServerConfig::logCategory() const {
    return logCategory_ ? static_cast<const char*>(logCategory_) : DEFAULT_LOG_CATEGORY;
}

} // particle
