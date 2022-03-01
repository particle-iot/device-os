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

#include "at_command.h"

#include "at_response.h"
#include "at_parser_impl.h"

#include <memory>
#include <cstdio>

namespace particle {

namespace {

// Initial buffer size for the vprintf() method
const size_t PRINTF_INIT_BUF_SIZE = 128;

} // unnamed

AtCommand::AtCommand(detail::AtParserImpl* parser) :
        parser_(parser),
        error_(0) {
}

AtCommand::AtCommand(int error) :
        parser_(nullptr),
        error_(error) {
}

AtCommand::AtCommand(AtCommand&& cmd) :
        parser_(cmd.parser_),
        error_(cmd.error_) {
    cmd.parser_ = nullptr;
    cmd.error_ = SYSTEM_ERROR_INVALID_STATE;
}

AtCommand::~AtCommand() {
    reset();
}

AtCommand& AtCommand::write(const char* data, size_t size) {
    int ret = SYSTEM_ERROR_INVALID_STATE;
    if (parser_) {
        ret = parser_->write(data, size);
    }
    if (ret < 0) {
        error(ret);
    }
    return *this;
}

AtCommand& AtCommand::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return *this;
}

AtCommand& AtCommand::vprintf(const char* fmt, va_list args) {
    char buf[PRINTF_INIT_BUF_SIZE];
    va_list args2;
    va_copy(args2, args);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > 0) {
        if ((size_t)n < sizeof(buf)) {
            write(buf, n);
        } else {
            // Allocate a larger buffer on the heap
            const std::unique_ptr<char[]> buf2(new char[n + 1]);
            if (buf2) {
                n = vsnprintf(buf2.get(), n + 1, fmt, args2);
                if (n > 0) {
                    write(buf2.get(), n);
                }
            } else {
                error(SYSTEM_ERROR_NO_MEMORY);
            }
        }
    }
    if (n < 0) {
        error(SYSTEM_ERROR_UNKNOWN);
    }
    va_end(args2);
    return *this;
}

AtCommand& AtCommand::timeout(unsigned timeout) {
    if (parser_) {
        parser_->commandTimeout(timeout);
    } else {
        error(SYSTEM_ERROR_INVALID_STATE);
    }
    return *this;
}

AtResponse AtCommand::send() {
    if (!parser_) {
        error(SYSTEM_ERROR_INVALID_STATE);
        return AtResponse(error_);
    }
    const int ret = parser_->sendCommand();
    if (ret < 0) {
        error(ret);
        return AtResponse(error_);
    }
    const auto p = parser_;
    parser_ = nullptr;
    return AtResponse(p);
}

int AtCommand::exec() {
    AtResponse resp = send();
    return resp.readResult();
}

void AtCommand::reset() {
    if (parser_) {
        parser_->resetCommand();
    }
}

int AtCommand::error(int ret) {
    if (parser_) {
        parser_ = nullptr;
    }
    if (error_ == 0) {
        error_ = ret;
    }
    return error_;
}

} // particle
