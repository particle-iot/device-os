/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "coap_defs.h"

#include "appender.h"

#include <utility>
#include <cstring>

namespace particle {

namespace protocol {

/**
 * A class for encoding CoAP messages.
 *
 * @note All message fields must be encoded in the order in which they follow in the CoAP message
 *       layout (see RFC 7252, 3. Message Format).
 */
class CoapMessageEncoder {
public:
    CoapMessageEncoder(char* buf, size_t size);

    CoapMessageEncoder& type(CoapType type);
    CoapMessageEncoder& code(CoapCode code);
    CoapMessageEncoder& code(unsigned code);
    CoapMessageEncoder& code(unsigned cls, unsigned detail);
    CoapMessageEncoder& id(CoapMessageId id);
    CoapMessageEncoder& token(const char* data, size_t size);
    // Note: Options must be encoded in the ascending order of their numbers
    CoapMessageEncoder& option(unsigned opt, const char* data, size_t size);
    CoapMessageEncoder& option(unsigned opt, const char* str);
    CoapMessageEncoder& option(unsigned opt, unsigned val);
    CoapMessageEncoder& option(unsigned opt, int val);
    CoapMessageEncoder& option(unsigned opt);
    CoapMessageEncoder& payload(const char* data, size_t size);
    CoapMessageEncoder& payload(const char* str);

    template<typename... ArgsT>
    CoapMessageEncoder& option(CoapOption opt, ArgsT&&... args);

    // TODO: Add convenience methods for encoding URI path and query options

    // Note: The returned message size can be larger than the size of the destination buffer
    int encode();

private:
    enum Flag {
        HEADER_ENCODED = 0x01,
        TOKEN_ENCODED = 0x02,
        OPTION_ENCODED = 0x04,
        PAYLOAD_ENCODED = 0x08,
        HAS_TYPE = 0x10,
        HAS_CODE = 0x20,
        HAS_ID = 0x40
    };

    BufferAppender buf_;
    CoapType type_;
    CoapMessageId id_;
    unsigned prevOpt_;
    unsigned flags_;
    unsigned code_;
    int error_;

    bool encodeHeader(size_t tokenSize = 0);
};

inline CoapMessageEncoder& CoapMessageEncoder::code(CoapCode code) {
    return this->code((unsigned)code);
}

inline CoapMessageEncoder& CoapMessageEncoder::code(unsigned cls, unsigned detail) {
    return code(coapCode(cls, detail));
}

inline CoapMessageEncoder& CoapMessageEncoder::option(unsigned opt, const char* str) {
    return option(opt, str, strlen(str));
}

inline CoapMessageEncoder& CoapMessageEncoder::option(unsigned opt, int val) {
    return option(opt, (unsigned)val);
}

inline CoapMessageEncoder& CoapMessageEncoder::option(unsigned opt) {
    return option(opt, nullptr, 0);
}

template<typename... ArgsT>
inline CoapMessageEncoder& CoapMessageEncoder::option(CoapOption opt, ArgsT&&... args) {
    return option((unsigned)opt, std::forward<ArgsT>(args)...);
}

inline CoapMessageEncoder& CoapMessageEncoder::payload(const char* str) {
    return payload(str, strlen(str));
}

} // namespace protocol

} // namespace particle
