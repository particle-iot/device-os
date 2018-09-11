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

#include "at_response.h"

#include "at_parser_impl.h"

#include "scope_guard.h"
#include "c_string.h"

#include <cstdio>

namespace particle {

namespace {

// Initial buffer sizes for different read methods
const size_t READ_LINE_INIT_BUF_SIZE = 128; // Allocated on the heap
const size_t READ_ALL_INIT_BUF_SIZE = 128; // Allocated on the heap
const size_t SCANF_INIT_BUF_SIZE = 128; // Allocated on the stack

inline size_t increaseBufSize(size_t size) {
    return size * 2;
}

} // unnamed

AtResponseReader::AtResponseReader(detail::AtParserImpl* parser) :
        parser_(parser),
        error_(0) {
}

AtResponseReader::AtResponseReader(int error) :
        parser_(nullptr),
        error_(error) {
}

AtResponseReader::AtResponseReader(AtResponseReader&& reader) :
        parser_(reader.parser_),
        error_(reader.error_) {
    reader.parser_ = nullptr;
    reader.error_ = SYSTEM_ERROR_INVALID_STATE;
}

AtResponseReader::~AtResponseReader() {
}

int AtResponseReader::read(char* data, size_t size) {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    return parser_->read(data, size);
}

int AtResponseReader::readLine(char* data, size_t size) {
    int n = readLine(data, size);
    if (n < 0) {
        return n;
    }
    if (size > 0) {
        if ((size_t)n == size) {
            --n;
        }
        data[n] = '\0';
    }
    PARSER_CHECK(nextLine());
    return n;
}

CString AtResponseReader::readLine() {
    const size_t size = READ_LINE_INIT_BUF_SIZE;
    auto buf = (char*)malloc(size);
    if (!buf) {
        error(SYSTEM_ERROR_NO_MEMORY);
        return CString();
    }
    NAMED_SCOPE_GUARD(g, {
        free(buf);
    });
    const int ret = readLine(buf, size, 0);
    if (ret < 0) {
        return CString();
    }
    g.dismiss();
    return CString::wrap(buf);
}

CString AtResponseReader::readAll() {
    size_t size = READ_ALL_INIT_BUF_SIZE;
    auto buf = (char*)malloc(size);
    if (!buf) {
        error(SYSTEM_ERROR_NO_MEMORY);
        return CString();
    }
    NAMED_SCOPE_GUARD(g, {
        free(buf);
    });
    size_t offs = 0;
    for (;;) {
        const int n = read(buf + offs, size - offs - 1);
        if (n < 0) {
            return CString();
        }
        offs += n;
        if (false /* atResponseEnd() */) {
            break;
        }
        size = increaseBufSize(size);
        const auto p = (char*)realloc(buf, size);
        if (!p) {
            error(SYSTEM_ERROR_NO_MEMORY);
            return CString();
        }
        buf = p;
    }
    buf[offs] = '\0';
    g.dismiss();
    return CString::wrap(buf);
}

int AtResponseReader::scanf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int ret = vscanf(fmt, args);
    va_end(args);
    return ret;
}

int AtResponseReader::vscanf(const char* fmt, va_list args) {
    char buf[SCANF_INIT_BUF_SIZE];
    int n = parser_->readLine(buf, sizeof(buf) - 1);
    if (n < 0) {
        return n;
    }
    if (parser_->endOfLine()) {
        buf[n] = '\0';
        n = vsscanf(buf, fmt, args);
    } else {
        // Allocate a larger buffer on the heap
        const size_t size = increaseBufSize(sizeof(buf));
        auto buf2 = (char*)malloc(size);
        if (!buf2) {
            return error(SYSTEM_ERROR_NO_MEMORY);
        }
        SCOPE_GUARD({
            free(buf2);
        });
        memcpy(buf2, buf, n);
        n = readLine(buf2, size, n);
        if (n < 0) {
            return n;
        }
        n = vsscanf(buf2, fmt, args);
    }
    if (n < 0) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    PARSER_CHECK(nextLine());
    return n;
}

int AtResponseReader::readLine(char* buf, size_t size, size_t offs) {
    for (;;) {
        const int n = parser_->readLine(buf + offs, size - offs - 1);
        if (n < 0) {
            return n;
        }
        offs += n;
        if (parser_->endOfLine()) {
            break;
        }
        size = increaseBufSize(size);
        buf = (char*)realloc(buf, size);
        if (!buf) {
            return error(SYSTEM_ERROR_NO_MEMORY);
        }
    }
    buf[offs] = '\0';
    return offs;
}

int AtResponseReader::error(int ret) {
    if (parser_) {
        parser_ = nullptr;
    }
    if (error_ == 0) {
        error_ = ret;
    }
    return error_;
}

AtResponse::AtResponse(detail::AtParserImpl* parser) :
        AtResponseReader(parser),
        resultErrorCode_(0) {
}

AtResponse::AtResponse(int error) :
        AtResponseReader(error),
        resultErrorCode_(0) {
}

AtResponse::AtResponse(AtResponse&& resp) :
        AtResponseReader(std::move(resp)),
        resultErrorCode_(resp.resultErrorCode_) {
    resp.resultErrorCode_ = 0;
}

AtResponse::~AtResponse() {
    if (parser_) {
        parser_->cancelCommand();
    }
}

int AtResponse::readResult() {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    const int ret = parser_->readResult(&resultErrorCode_);
    if (ret < 0) {
        return error(ret);
    }
    parser_ = nullptr;
    return ret;
}

void AtResponse::reset() {
    if (parser_) {
        parser_->cancelCommand();
        parser_ = nullptr;
    }
}

} // particle
