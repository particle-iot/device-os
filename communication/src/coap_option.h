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

#include <cstring>

namespace particle {

class Appender;

namespace protocol {

class CoapOptionEncoder {
public:
    explicit CoapOptionEncoder(Appender* appender);

    CoapOptionEncoder& encodeEmpty(unsigned opt);
    CoapOptionEncoder& encodeOpaque(unsigned opt, const char* data, size_t size);
    CoapOptionEncoder& encodeString(unsigned opt, const char* str);
    CoapOptionEncoder& encodeUInt(unsigned opt, unsigned val);

private:
    Appender* const appender_;
    unsigned prevOpt_;
};

inline CoapOptionEncoder::CoapOptionEncoder(Appender* appender) :
        appender_(appender),
        prevOpt_(0) {
}

inline CoapOptionEncoder& CoapOptionEncoder::encodeEmpty(unsigned opt) {
    return encodeOpaque(opt, nullptr, 0);
}

inline CoapOptionEncoder& CoapOptionEncoder::encodeString(unsigned opt, const char* str) {
    return encodeOpaque(opt, str, strlen(str));
}

} // namespace protocol

} // namespace particle
