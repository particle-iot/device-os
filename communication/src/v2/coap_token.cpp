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

#include <algorithm>
#include <cstring>

#include "coap_token.h"

#include "endian_util.h"
#include "random.h"

namespace particle::protocol::v2 {

CoapToken::CoapToken(const char* data, size_t size) :
        size_(std::min(size, MAX_COAP_TOKEN_SIZE)) {
    std::memcpy(data_, data, size_);
}

CoapToken& CoapToken::increment() {
    uint64_t v = 0;
    static_assert(sizeof(v) >= MAX_COAP_TOKEN_SIZE);
    std::memcpy(&v, data_, size_);
    v = nativeToLittleEndian(littleEndianToNative(v) + 1);
    std::memcpy(data_, &v, size_);
    return *this;
}

int CoapToken::compareWith(const CoapToken& token) const {
    if (size_ < token.size_) {
        return -1;
    }
    if (size_ > token.size_) {
        return 1;
    }
    return std::memcmp(data_, token.data_, size_);
}

static CoapToken CoapToken::generate(size_t size) {
    CoapToken token;
    token.size_ = std::min(size, MAX_COAP_TOKEN_SIZE);
    Random::genSecure(token.data_, token.size_);
    return token;
}

} // particle::protocol::v2
