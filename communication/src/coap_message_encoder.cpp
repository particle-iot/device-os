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

#include "coap_message_encoder.h"

#include "endian_util.h"

namespace particle {

namespace protocol {

CoapMessageEncoder::CoapMessageEncoder(char* buf, size_t size) :
        buf_(buf, size),
        type_(CoapType::CON),
        id_(0),
        prevOpt_(0),
        flags_(0),
        code_(0),
        error_(0) {
}

CoapMessageEncoder& CoapMessageEncoder::type(CoapType type) {
    if (error_) {
        return *this;
    }
    if (flags_ & Flag::HAS_TYPE || flags_ & Flag::HEADER_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    type_ = type;
    flags_ |= Flag::HAS_TYPE;
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::code(unsigned code) {
    if (error_) {
        return *this;
    }
    if (flags_ & Flag::HAS_CODE || flags_ & Flag::HEADER_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    if (code > 255) {
        error_ = SYSTEM_ERROR_INVALID_ARGUMENT;
        return *this;
    }
    code_ = code;
    flags_ |= Flag::HAS_CODE;
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::id(CoapMessageId id) {
    if (error_) {
        return *this;
    }
    if (flags_ & Flag::HAS_ID || flags_ & Flag::HEADER_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    id_ = id;
    flags_ |= Flag::HAS_ID;
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::token(const char* token, size_t size) {
    if (error_) {
        return *this;
    }
    if (size > MAX_COAP_TOKEN_SIZE) { // RFC 7252, 3. Message Format
        error_ = SYSTEM_ERROR_INVALID_ARGUMENT;
        return *this;
    }
    if (flags_ & Flag::HEADER_ENCODED || flags_ & Flag::TOKEN_ENCODED || flags_ & Flag::OPTION_ENCODED ||
            flags_ & Flag::PAYLOAD_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    if (!encodeHeader(size)) {
        return *this;
    }
    buf_.append((const uint8_t*)token, size);
    flags_ |= Flag::TOKEN_ENCODED;
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::option(unsigned opt, const char* data, size_t size) {
    if (error_) {
        return *this;
    }
    if (size > 65535 + 269) { // RFC 7252, 3.1. Option Format
        error_ = SYSTEM_ERROR_INVALID_ARGUMENT;
        return *this;
    }
    if (opt < prevOpt_ || flags_ & Flag::PAYLOAD_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    const unsigned optDelta = opt - prevOpt_;
    if (optDelta > 65535 + 269) {
        error_ = SYSTEM_ERROR_INVALID_ARGUMENT;
        return *this;
    }
    if (!(flags_ & Flag::HEADER_ENCODED) && !encodeHeader()) {
        return *this;
    }
    uint8_t deltaNibble = 0;
    if (optDelta <= 12) {
        deltaNibble = optDelta;
    } else if (optDelta <= 268) {
        deltaNibble = 13;
    } else {
        deltaNibble = 14;
    }
    uint8_t lenNibble = 0;
    if (size <= 12) {
        lenNibble = size;
    } else if (size <= 268) {
        lenNibble = 13;
    } else {
        lenNibble = 14;
    }
    buf_.appendUInt8((deltaNibble << 4) | lenNibble);
    if (optDelta > 12) {
        if (optDelta <= 268) {
            buf_.appendUInt8(optDelta - 13);
        } else {
            buf_.appendUInt16BE(optDelta - 269);
        }
    }
    if (size > 12) {
        if (size <= 268) {
            buf_.appendUInt8(size - 13);
        } else {
            buf_.appendUInt16BE(size - 269);
        }
    }
    buf_.append((const uint8_t*)data, size);
    prevOpt_ = opt;
    flags_ |= Flag::OPTION_ENCODED;
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::option(unsigned opt, unsigned val) {
    if (val == 0) {
        option(opt, nullptr, 0);
    } else if (val <= 255) {
        const uint8_t v = val;
        option(opt, (const char*)&v, 1);
    } else if (val <= 65535) {
        const uint16_t v = nativeToBigEndian<uint16_t>(val);
        option(opt, (const char*)&v, 2);
    } else {
        const uint32_t v = nativeToBigEndian<uint32_t>(val);
        option(opt, (const char*)&v, 4);
    }
    return *this;
}

CoapMessageEncoder& CoapMessageEncoder::payload(const char* data, size_t size) {
    if (error_) {
        return *this;
    }
    if (flags_ & Flag::PAYLOAD_ENCODED) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return *this;
    }
    if (!(flags_ & Flag::HEADER_ENCODED) && !encodeHeader()) {
        return *this;
    }
    if (size > 0) {
        buf_.appendUInt8(0xff); // Payload marker
        buf_.append((const uint8_t*)data, size);
    }
    flags_ |= Flag::PAYLOAD_ENCODED;
    return *this;
}

int CoapMessageEncoder::encode() {
    if (error_ || (!(flags_ & Flag::HEADER_ENCODED) && !encodeHeader())) {
        return error_;
    }
    return buf_.dataSize();
}

bool CoapMessageEncoder::encodeHeader(size_t tokenSize) {
    if (error_) {
        return false;
    }
    // The code using this class is required to provide at least a message type
    if (flags_ & Flag::HEADER_ENCODED || !(flags_ & HAS_TYPE)) {
        error_ = SYSTEM_ERROR_INVALID_STATE;
        return false;
    }
    const uint32_t h = (1u << 30) | // Version
            ((unsigned)type_ << 28) | // Type
            (tokenSize << 24) | // Token length
            (code_ << 16) | // Code
            id_; // Message ID
    buf_.appendUInt32BE(h);
    flags_ |= Flag::HEADER_ENCODED;
    return true;
}

} // namespace protocol

} // namespace particle
