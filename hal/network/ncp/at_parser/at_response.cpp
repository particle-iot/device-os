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
#include "check.h"

#include <cstdio>

namespace particle {

namespace {

// Initial buffer sizes for different read methods
const size_t READ_LINE_INIT_BUF_SIZE = 128; // Allocated on the heap
const size_t SCANF_INIT_BUF_SIZE = 128; // Allocated on the stack

inline size_t incBufSize(size_t size) {
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

int AtResponseReader::readLine(char* data, size_t size) {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    int n = parser_->readLine(data, size);
    if (n < 0) {
        return error(n);
    }
    if (size > 0) {
        if ((size_t)n == size) {
            --n;
        }
        data[n] = '\0';
    }
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

int AtResponseReader::scanf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int ret = vscanf(fmt, args);
    va_end(args);
    return ret;
}

int AtResponseReader::vscanf(const char* fmt, va_list args) {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    char buf[SCANF_INIT_BUF_SIZE];
    int n = parser_->readLine(buf, sizeof(buf) - 1);
    if (n < 0) {
        return error(n);
    }
    if (parser_->atLineEnd()) {
        buf[n] = '\0';
        n = vsscanf(buf, fmt, args);
    } else {
        // Allocate a larger buffer on the heap
        const size_t size = incBufSize(sizeof(buf));
        auto buf2 = (char*)malloc(size);
        if (!buf2) {
            return error(SYSTEM_ERROR_NO_MEMORY);
        }
        SCOPE_GUARD({
            free(buf2);
        });
        memcpy(buf2, buf, n);
        CHECK(readLine(buf2, size, n));
        n = vsscanf(buf2, fmt, args);
    }
    if (n < 0) {
        // Do not invalidate the reader object on scanf() errors
        return SYSTEM_ERROR_BAD_DATA;
    }
    return n;
}

int AtResponseReader::readLine(char* buf, size_t size, size_t offs) {
    for (;;) {
        const int n = parser_->readLine(buf + offs, size - offs - 1);
        if (n < 0) {
            return error(n);
        }
        offs += n;
        if (parser_->atLineEnd()) {
            break;
        }
        size = incBufSize(size);
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

AtResponseReader& AtResponseReader::operator=(AtResponseReader&& reader) {
    parser_ = reader.parser_;
    error_ = reader.error_;
    reader.parser_ = nullptr;
    reader.error_ = SYSTEM_ERROR_INVALID_STATE;
    return *this;
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
    reset();
}

bool AtResponse::hasNextLine() {
    if (!parser_) {
        error(SYSTEM_ERROR_INVALID_STATE);
        return false;
    }
    bool hasLine = false;
    const int ret = parser_->hasNextLine(&hasLine);
    if (ret < 0) {
        error(ret);
        return false;
    }
    return hasLine;
}

int AtResponse::nextLine() {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    const int ret = parser_->nextLine();
    if (ret < 0) {
        return error(ret);
    }
    return ret;
}

int AtResponse::readResult() {
    if (!parser_) {
        return error(SYSTEM_ERROR_INVALID_STATE);
    }
    const int ret = parser_->readResult(&resultErrorCode_);
    if (ret < 0) {
        return error(ret);
    }
    reset();
    return ret;
}

void AtResponse::reset() {
    if (parser_) {
        parser_->resetCommand();
        parser_ = nullptr;
    }
}

AtResponse& AtResponse::operator=(AtResponse&& resp) {
    AtResponseReader::operator=(std::move(resp));
    resultErrorCode_ = resp.resultErrorCode_;
    resp.resultErrorCode_ = 0;
    return *this;
}

} // particle
