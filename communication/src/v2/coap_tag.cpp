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

#include "v2/coap_tag.h"
#include "coap_defs.h"

#include "endian_util.h"
#include "random.h"

namespace particle::protocol::v2 {

static_assert(CoapTag::MAX_SIZE >= MAX_COAP_TOKEN_SIZE &&
        CoapTag::MAX_SIZE >= MAX_COAP_ETAG_OPTION_SIZE &&
        CoapTag::MAX_SIZE >= MAX_COAP_REQUEST_TAG_OPTION_SIZE);

CoapTag::CoapTag(const char* data, size_t size) :
        size_(std::min(size, MAX_SIZE)) {
    std::memcpy(data_, data, size_);
}

CoapTag& CoapTag::next() {
    uint64_t v = 0;
    static_assert(sizeof(v) >= MAX_SIZE);
    std::memcpy(&v, data_, size_);
    v = nativeToLittleEndian(littleEndianToNative(v) + 1);
    std::memcpy(data_, &v, size_);
    return *this;
}

int CoapTag::compareWith(const CoapTag& tag) const {
    if (size_ < tag.size_) {
        return -1;
    }
    if (size_ > tag.size_) {
        return 1;
    }
    return std::memcmp(data_, tag.data_, size_);
}

CoapTag CoapTag::generate(size_t size) {
    CoapTag tag;
    tag.size_ = std::min(size, MAX_SIZE);
    Random::genSecure(tag.data_, tag.size_);
    return tag;
}

} // particle::protocol::v2
