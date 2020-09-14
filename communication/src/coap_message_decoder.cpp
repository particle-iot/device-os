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

#include "coap_message_decoder.h"

#include "endian_util.h"
#include "check.h"

#include <algorithm>
#include <type_traits>

namespace particle {

namespace protocol {

namespace {

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
int readUIntBE(T* val, const char* data, size_t size) {
    CHECK_TRUE(size >= sizeof(T), SYSTEM_ERROR_NOT_ENOUGH_DATA);
    memcpy(val, data, sizeof(T));
    *val = bigEndianToNative(*val);
    return sizeof(T);
}

int readOption(unsigned* opt, const char** optData, size_t* optSize, unsigned prevOpt, const char* data, size_t size) {
    size_t offs = 0;
    uint8_t b = 0;
    offs += CHECK(readUIntBE(&b, data + offs, size - offs));
    unsigned optDelta = b >> 4;
    size_t optLen = b & 0x0f;
    if (optDelta == 13) {
        offs += CHECK(readUIntBE(&b, data + offs, size - offs));
        optDelta = 13 + b;
    } else if (optDelta == 14) {
        uint16_t d = 0;
        offs += CHECK(readUIntBE(&d, data + offs, size - offs));
        optDelta = 269 + d;
    } else if (optDelta == 15) {
        return SYSTEM_ERROR_BAD_DATA; // Reserved
    }
    if (optLen == 13) {
        offs += CHECK(readUIntBE(&b, data + offs, size - offs));
        optLen = 13 + b;
    } else if (optLen == 14) {
        uint16_t d = 0;
        offs += CHECK(readUIntBE(&d, data + offs, size - offs));
        optLen = 269 + d;
    } else if (optLen == 15) {
        return SYSTEM_ERROR_BAD_DATA; // Reserved
    }
    CHECK_TRUE(size - offs >= optLen, SYSTEM_ERROR_NOT_ENOUGH_DATA);
    if (opt) {
        *opt = prevOpt + optDelta;
    }
    if (optData) {
        *optData = data + offs;
    }
    if (optSize) {
        *optSize = optLen;
    }
    offs += optLen;
    return offs;
}

} // namespace

CoapMessageDecoder::CoapMessageDecoder() :
        token_(),
        type_(CoapType::CON),
        id_(0),
        opts_(nullptr),
        payload_(nullptr),
        tokenSize_(0),
        optsSize_(0),
        payloadSize_(0),
        code_(0) {
}

int CoapMessageDecoder::decode(const char* data, size_t size) {
    size_t offs = 0;
    // Message header
    uint32_t h = 0;
    offs += CHECK(readUIntBE(&h, data + offs, size - offs));
    // Version
    unsigned v = h >> 30;
    CHECK_TRUE(v == 1, SYSTEM_ERROR_BAD_DATA);
    // Type
    v = (h >> 28) & 0x03;
    CHECK_TRUE(isCoapValidType(v), SYSTEM_ERROR_BAD_DATA); // Can't be invalid since it's a 2-bit field
    const auto type = (CoapType)v;
    // Token length
    const size_t tokenSize = (h >> 24) & 0x0f;
    CHECK_TRUE(tokenSize <= MAX_COAP_TOKEN_SIZE, SYSTEM_ERROR_BAD_DATA);
    // Code
    const unsigned code = (h >> 16) & 0xff;
    CHECK_TRUE(isCoapValidCode(code), SYSTEM_ERROR_BAD_DATA);
    // Message ID
    const CoapMessageId id = h & 0xffff;
    // Token
    CHECK_TRUE(size - offs >= tokenSize, SYSTEM_ERROR_NOT_ENOUGH_DATA);
    const size_t tokenOffs = offs;
    offs += tokenSize;
    // Options
    size_t optsOffs = offs;
    while (size - offs > 0 && *(data + offs) != (char)0xff) {
        offs += CHECK(readOption(nullptr /* opt */, nullptr /* optData */, nullptr /* optSize */, 0 /* prevOpt */,
                data + offs, size - offs));
    }
    const size_t optsSize = offs - optsOffs;
    // Payload
    if (size - offs > 0) {
        ++offs; // Skip the payload marker
    }
    payloadSize_ = size - offs;
    payload_ = (payloadSize_ > 0) ? data + offs : nullptr;
    optsSize_ = optsSize;
    opts_ = (optsSize_ > 0) ? data + optsOffs : nullptr;
    tokenSize_ = tokenSize;
    memcpy(token_, data + tokenOffs, tokenSize_);
    code_ = code;
    type_ = type;
    id_ = id;
    return size;
}

CoapOptionIterator CoapMessageDecoder::findOption(unsigned opt) const {
    auto it = options();
    while (it.next()) {
        if (it.option() == opt) {
            break;
        }
    }
    return it;
}

bool CoapOptionIterator::next() {
    if (!nextOpt_) {
        reset();
        return false;
    }
    unsigned opt = 0;
    const char* data = nullptr;
    size_t size = 0;
    const int n = readOption(&opt, &data, &size, opt_, nextOpt_, bufSize_);
    if (n < 0) { // Shouldn't be possible
        reset();
        return false;
    }
    if (n < (int)bufSize_) {
        nextOpt_ += n;
        bufSize_ -= n;
    } else {
        nextOpt_ = nullptr;
        bufSize_ = 0;
    }
    opt_ = opt;
    optData_ = data;
    optSize_ = size;
    return true;
}

unsigned CoapOptionIterator::toUInt() const {
    uint32_t v = 0;
    if (optData_ && optSize_ <= 4) {
        memcpy((char*)&v + 4 - optSize_, optData_, optSize_);
    }
    return bigEndianToNative(v);
}

void CoapOptionIterator::reset() {
    nextOpt_ = nullptr;
    bufSize_ = 0;
    optData_ = nullptr;
    optSize_ = 0;
    opt_ = 0;
}

} // namespace protocol

} // namespace particle
