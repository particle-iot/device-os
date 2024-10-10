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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "logging.h"
#include "at_server.h"
#include "check.h"
#include "timer_hal.h"
#include <algorithm>
#include "stream.h"

namespace particle {

namespace {

// Initial buffer size for the vprintf() method
const size_t PRINTF_INIT_BUF_SIZE = 128;

} // unnamed


AtServer::AtServer()
        : requestBuf_{},
          bufPos_(0),
          newLineOffset_(0),
          suspended_(false) {
}

AtServer::~AtServer() {
}

int AtServer::init(const AtServerConfig& config) {
    conf_ = config;
    return 0;
}

void AtServer::destroy() {
    reset();
}

int AtServer::reset() {
    memset(requestBuf_, 0, sizeof(requestBuf_));
    bufPos_ = 0;
    newLineOffset_ = 0;
    return 0;
}

int AtServer::addCommandHandler(const AtServerCommandHandler& handler) {
    if (!handlers_.append(handler)) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    std::sort(handlers_.begin(), handlers_.end(), [](const AtServerCommandHandler& h1, const AtServerCommandHandler& h2) {
        return (strlen(h1.command()) > strlen(h2.command())); // In descending order
    });
    return 0;
}

int AtServer::removeCommandHandler(const char* command, AtServerCommandType type) {
    bool removed = false;
    for (int i = 0; i < handlers_.size();) {
        auto& handler = handlers_[i];
        if (handler.type() != type && type != AtServerCommandType::ANY) {
            i++;
            continue;
        }
        if (!strcmp(handler.command(), command)) {
            // Remove
            handlers_.removeAt(i);
            removed = true;
            continue;
        }
        i++;
    }
    return removed ? 0 : SYSTEM_ERROR_NOT_FOUND;
}

int AtServer::process() {
    if (suspended_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    while (hasNextLine()) {
        CHECK(processRequest());
        if (suspended_) {
            return 0;
        }
    }

    auto stream = conf_.stream();
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);

    auto start = HAL_Timer_Get_Milli_Seconds();
    auto end = start + conf_.streamTimeout();

    for (;;) {
        // Consume non-command bytes if any
        CHECK(processRequest());

        auto now = HAL_Timer_Get_Milli_Seconds();
        auto waitTimeout = now <= end ? end - now : 0;
        CHECK(stream->waitEvent(Stream::READABLE, waitTimeout));

        size_t read = CHECK(stream->read(requestBuf_ + bufPos_, DEFAULT_REQUEST_BUFFER_SIZE - bufPos_));
        if (read > 0 && conf_.echoEnabled()) {
            // Echo
            CHECK(write(requestBuf_ + bufPos_, read));
        }
        bufPos_ += read;

        while (hasNextLine()) {
            CHECK(processRequest());
            if (suspended_) {
                return 0;
            }
        }

        if (waitTimeout == 0) {
            break;
        }
    }

    return 0;
}

int AtServer::suspend() {
    suspended_ = true;
    return 0;
}

bool AtServer::suspended() const {
    return suspended_;
}

int AtServer::resume() {
    suspended_ = false;
    return 0;
}

AtServerConfig AtServer::config() const {
    return conf_;
}

int AtServer::processRequest() {
    size_t skip = 0;
    bool found = false;
    for (size_t i = 0; i < bufPos_; i++) {
        if (std::toupper(requestBuf_[i]) == 'A') {
            if ((i + 1) >= bufPos_) {
                break;
            }
            if (std::toupper(requestBuf_[i + 1]) == 'T') {
                // Found AT
                found = true;
                break;
            }
        } else {
            skip++;
        }
    }
    if (skip) {
        memmove(requestBuf_, requestBuf_ + skip, bufPos_ - skip);
        bufPos_ -= skip;
    }
    if (!found) {
        return 0;
    }

    // Supposedly a command
    if (!hasNextLine()) {
        return 0;
    }
    size_t lineLength = newLineOffset_;
    AtServerCommandType type = AtServerCommandType::EXEC; // Default
    size_t commandLength = lineLength;
    size_t dataStart = lineLength;
    for (size_t i = 0; i < lineLength; i++) {
        if (requestBuf_[i] == '=') {
            if ((i + 1) < lineLength && requestBuf_[i + 1] == '?') {
                if ((i + 2) == lineLength) {
                    type = AtServerCommandType::TEST;
                } else {
                    // This is kinda wrong, but ok, we'll try to handle it
                    type = AtServerCommandType::WRITE;
                }
                commandLength = i;
                dataStart = i + 2;
                break;
            } else {
                type = AtServerCommandType::WRITE;
                commandLength = i;
                dataStart = i + 1;
                break;
            }
        } else if (requestBuf_[i] == '?') {
            type = AtServerCommandType::READ;
            commandLength = i;
            dataStart = i + 1;
            break;
        }
    }

    std::transform(requestBuf_, requestBuf_ + commandLength, requestBuf_, ::toupper);
    found = false;
    requestBuf_[lineLength] = '\0';
    AtServerRequest req(this, type, requestBuf_ + 2 /* AT */, lineLength - 2 /* AT */, dataStart - 2);

    if (commandLength > 2) {
        logCmdLine(requestBuf_, lineLength);
        for (auto& handler: handlers_) {
            if (!strncmp(handler.command(), requestBuf_ + 2, std::max(commandLength - 2, strlen(handler.command()))) ||
                (handler.type() == AtServerCommandType::WILDCARD && !strncmp(handler.command(), requestBuf_ + 2, strlen(handler.command())))) {
                // Matches
                if (handler.type() == type || handler.type() == AtServerCommandType::ANY || handler.type() == AtServerCommandType::WILDCARD) {
                    found = true;
                    if (handler.handler(&req, type, (const char*)handler.command(), handler.data()) < 0) {
                        req.setFinalResponse(AtServerRequest::ERROR);
                    }
                    break;
                }
            } else if (handler.type() == AtServerCommandType::ONE_INT_ARG && type == AtServerCommandType::EXEC) {
                if (!strncmp(handler.command(), requestBuf_ + 2, strlen(handler.command()))) {
                    int dummy = 0;
                    if (sscanf(requestBuf_ + 2 + strlen(handler.command()), "%d", &dummy) == 1) {
                        // Matches
                        found = true;
                        req.dataOffset_ = strlen(handler.command());
                        if (handler.handler(&req, handler.type(), (const char*)handler.command(), handler.data()) < 0) {
                            req.setFinalResponse(AtServerRequest::ERROR);
                        }
                        break;
                    }
                }
            }
        }
    }
    if (!found) {
        if (commandLength > 2) {
            req.setFinalResponse(AtServerRequest::ERROR);
        } else {
            req.setFinalResponse(AtServerRequest::OK);
        }
    }
    // Consume with newline character
    lineLength++;
    memmove(requestBuf_, requestBuf_ + lineLength, bufPos_ - lineLength);
    bufPos_ -= lineLength;
    return 0;
}

int AtServer::write(const char* str, size_t len) {
    return write(str, len, conf_.streamTimeout());
}

int AtServer::write(const char* str, size_t len, system_tick_t timeout) {
    auto stream = conf_.stream();
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);
    CHECK(stream->waitEvent(Stream::WRITABLE, timeout));
    return stream->write(str, len);
}

int AtServer::writeNewLine() {
    switch (conf_.commandTerminator()) {
        case AtCommandTerminator::CR: {
            return write("\r", 1);
        }
        case AtCommandTerminator::LF: {
            return write("\n", 1);
        }
        case AtCommandTerminator::CRLF: {
            return write("\r\n", 2);
        }
    }
    return 0;
}

bool AtServer::hasNextLine() {
    for (size_t i = 0; i < bufPos_; i++) {
        if (requestBuf_[i] == '\r' || requestBuf_[i] == '\n') {
            newLineOffset_ = i;
            return true;
        }
    }
    return false;
}

void AtServer::logCmdLine(const char* data, size_t size) const {
    if (conf_.logEnabled() && size > 0) {
        LOG_C(TRACE, conf_.logCategory(), "> %.*s", size, data);
    }
}

void AtServer::logRespLine(const char* data, size_t size) const {
    if (conf_.logEnabled() && size > 0) {
        LOG_C(TRACE, conf_.logCategory(), "< %.*s", size, data);
    }
}

AtServerCommandHandler::AtServerCommandHandler(AtServerCommandType type, const char* command, ServerCommandHandler handler, void* data)
        : type_(type),
          command_(command),
          handler_(handler),
          data_(data),
          cResponse_(nullptr),
          response_(nullptr) {
    std::transform((const char*)command, (const char*)command + strlen(command), (char*)((const char*)command_), ::toupper);
}

AtServerCommandHandler::AtServerCommandHandler(AtServerCommandType type, const char* command, const char* response)
        : AtServerCommandHandler(type, command, nullptr, nullptr) {
    cResponse_ = response;
}

AtServerCommandHandler::AtServerCommandHandler(AtServerCommandType type, const char* command, CString response)
        : AtServerCommandHandler(type, command, nullptr, nullptr) {
    response_ = response;
}

AtServerCommandType AtServerCommandHandler::type() const {
    return type_;
}

CString AtServerCommandHandler::command() const {
    return command_;
}

AtServerCommandHandler::ServerCommandHandler AtServerCommandHandler::handler() const {
    return handler_;
}

void* AtServerCommandHandler::data() const {
    return data_;
}

int AtServerCommandHandler::handler(AtServerRequest* request, AtServerCommandType type, const char* command, void* data) {
    if (handler_) {
        return handler_(request, type, command, data);
    } else if ((const char*)response_) {
        return request->sendResponse(response_);
    } else if (cResponse_) {
        return request->sendResponse(cResponse_);
    }
    return 0;
}

AtServerRequest::AtServerRequest(AtServer* server, AtServerCommandType type, const char* data, size_t len, size_t dataOffset)
        : server_(server),
          finalResponse_(AtServerRequest::OK),
          type_(type),
          buf_(data),
          len_(len),
          dataOffset_(dataOffset) {

}

AtServer* AtServerRequest::server() const {
    return server_;
}

AtServerRequest::~AtServerRequest() {
    // FIXME: duplication with AT parser
    const char* finalResponses[] = {
        "OK",
        "ERROR",
        "BUSY",
        "NO_ANSWER",
        "NO_CARRIER",
        "NO_DIALTONE",
        "+CME_ERROR",
        "+CMS_ERROR",
        "CONNECT"
    };
    server_->writeNewLine();
    server_->logRespLine(finalResponses[finalResponse_], strlen(finalResponses[finalResponse_]));
    server_->write(finalResponses[finalResponse_], strlen(finalResponses[finalResponse_]));
    server_->writeNewLine();
}

CString AtServerRequest::readLine() {
    return CString(buf_, len_);
}

int AtServerRequest::readLine(char* data, size_t size) {
    if (data) {
        strncpy(data, buf_, size);
    }
    return std::min(size, len_);
}

CString AtServerRequest::read() {
    return CString(buf_ + dataOffset_, len_ - dataOffset_);
}

int AtServerRequest::read(char* data, size_t size) {
    if (data) {
        strncpy(data, buf_ + dataOffset_, size);
    }
    return std::min(size, len_ - dataOffset_);
}

int AtServerRequest::scanf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int ret = vscanf(fmt, args);
    va_end(args);
    return ret;
}

int AtServerRequest::vscanf(const char* fmt, va_list args) {
    int n = vsscanf(type_ == AtServerCommandType::ANY ? buf_ : buf_ + dataOffset_, fmt, args);
    return n;
}

int AtServerRequest::sendResponse(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int r = sendResponseV(fmt, args);
    va_end(args);
    return r;
}

int AtServerRequest::sendResponseV(const char* fmt, va_list args) {
    if (!sentResponse_) {
        server_->writeNewLine();
    }
    sentResponse_ = true;

    char buf[PRINTF_INIT_BUF_SIZE];
    va_list args2;
    va_copy(args2, args);
    int err = 0;
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > 0) {
        if ((size_t)n < sizeof(buf)) {
            server_->write(buf, n);
            server_->logRespLine(buf, n);
            server_->writeNewLine();
        } else {
            // Allocate a larger buffer on the heap
            const std::unique_ptr<char[]> buf2(new char[n + 1]);
            if (buf2) {
                n = vsnprintf(buf2.get(), n + 1, fmt, args2);
                if (n > 0) {
                    server_->write(buf2.get(), n);
                    server_->logRespLine(buf2.get(), n);
                    server_->writeNewLine();
                }
            } else {
                err = SYSTEM_ERROR_NO_MEMORY;
            }
        }
    }
    if (n < 0) {
        err = SYSTEM_ERROR_UNKNOWN;
    }
    va_end(args2);
    return err;
}

int AtServerRequest::sendNewLine() {
    return server_->writeNewLine();
}

int AtServerRequest::setFinalResponse(Result result) {
    finalResponse_ = result;
    return 0;
}

} // particle