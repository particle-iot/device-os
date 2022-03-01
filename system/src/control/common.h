/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_control.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_error.h"
#include "inet_hal.h"

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <stdlib.h>

#include "proto/common.pb.h"

namespace particle {
namespace control {
namespace common {

int encodeReplyMessage(ctrl_request* req, const pb_field_t* fields, const void* src);
int decodeRequestMessage(ctrl_request* req, const pb_field_t* fields, void* dst);
int appendReplySubmessage(ctrl_request* req, size_t offset, const pb_field_t* field,
        const pb_field_t* fields, const void* src);

int protoIpFromHal(particle_ctrl_IPAddress* ip, const HAL_IPAddress* sip);
int halIpFromProto(particle_ctrl_IPAddress* ip, HAL_IPAddress* halip);

template<typename T>
struct ProtoDecodeBytesLengthHelper {
    explicit ProtoDecodeBytesLengthHelper(pb_callback_t* cb, const void** ptr, T* size)
        : ptr_(ptr),
          size_(size) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* stream, const pb_field_t* field, void** arg) -> bool {
            auto self = static_cast<ProtoDecodeBytesLengthHelper*>(*arg);
            self->fill(stream->state, stream->bytes_left);
            return pb_read(stream, nullptr, stream->bytes_left);
        };
    }

    void fill(const void* p, size_t size) {
        *ptr_ = p;
        *size_ = size;
    }

    const void** ptr_;
    T* size_;
};

// Helper classes for working with nanopb's string fields
struct EncodedString {
    const char* data;
    size_t size;

    explicit EncodedString(pb_callback_t* cb, const char* data = nullptr, size_t size = 0) :
            data(data),
            size(size) {
        cb->arg = this;
        cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
            const auto str = (const EncodedString*)*arg;
            if (str->data && str->size > 0 && (!pb_encode_tag_for_field(strm, field) ||
                    !pb_encode_string(strm, (const uint8_t*)str->data, str->size))) {
                return false;
            }
            return true;
        };
    }
};

// Note: Do not use this class with non-buffered nanopb streams
struct DecodedString {
    const char* data;
    size_t size;

    explicit DecodedString(pb_callback_t* cb) :
            data(nullptr),
            size(0) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
            const size_t n = strm->bytes_left;
            if (n > 0) {
                const auto str = (DecodedString*)*arg;
                str->data = (const char*)strm->state;
                str->size = n;
            }
            return pb_read(strm, nullptr, n);
        };
    }
};

// Class storing a null-terminated string
struct DecodedCString {
    char* data;
    size_t size;

    explicit DecodedCString(pb_callback_t* cb) :
            data(nullptr),
            size(0) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
            const size_t n = strm->bytes_left;
            const auto str = (DecodedCString*)*arg;
            str->data = (char*)malloc(n + 1);
            if (!str->data) {
                return false;
            }
            str->data[n] = '\0';
            str->size = n;
            return pb_read(strm, (pb_byte_t*)str->data, n);
        };
    }

    ~DecodedCString() {
        free((char*)data);
    }

    // This class is non-copyable
    DecodedCString(const DecodedCString&) = delete;
    DecodedCString& operator=(const DecodedCString&) = delete;
};

} } } /* particle::control::common */

#endif /* #if SYSTEM_CONTROL_ENABLED */
