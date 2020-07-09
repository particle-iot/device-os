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

#include "coap_option.h"

#include "appender.h"
#include "endian_util.h"

#include <cstdint>

namespace particle::protocol {

CoapOptionEncoder& CoapOptionEncoder::encodeOpaque(unsigned opt, const char* data, size_t size) {
    // RFC 7252, 3.1. Option Format
    const unsigned optDelta = opt - prevOpt_;
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
    appender_->appendUInt8((deltaNibble << 4) | lenNibble);
    if (optDelta > 12) {
        if (optDelta <= 268) {
            appender_->appendUInt8(optDelta - 13);
        } else {
            appender_->appendUInt16BE(optDelta - 269);
        }
    }
    if (size > 12) {
        if (size <= 268) {
            appender_->appendUInt8(size - 13);
        } else {
            appender_->appendUInt16BE(size - 269);
        }
    }
    appender_->append((const uint8_t*)data, size);
    prevOpt_ = opt;
    return *this;
}

CoapOptionEncoder& CoapOptionEncoder::encodeUInt(unsigned opt, unsigned val) {
    if (val == 0) {
        encodeOpaque(opt, nullptr, 0);
    } else if (val <= 255) {
        const uint8_t v = val;
        encodeOpaque(opt, (const char*)&v, 1);
    } else if (val <= 65535) {
        const uint16_t v = nativeToBigEndian<uint16_t>(val);
        encodeOpaque(opt, (const char*)&v, 2);
    } else {
        const uint32_t v = nativeToBigEndian<uint32_t>(val);
        encodeOpaque(opt, (const char*)&v, 4);
    }
    return *this;
}

} // namespace particle::protocol
