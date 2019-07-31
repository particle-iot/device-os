/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "endian_util.h"
#include "check.h"

#include <cstdint>

namespace particle {

/**
 * Encode an unsigned integer as a varint.
 *
 * This function does not write more than `size` bytes to the destination buffer. If the output was
 * truncated due to that limit, then the return value is the number of bytes which would have been
 * written to the buffer if it had enough space available.
 *
 * The varint format is described here:
 * https://developers.google.com/protocol-buffers/docs/encoding#varints
 *
 * @param[out] buf Destination buffer.
 * @param size Buffer size.
 * @param val A value.
 * @return The number of bytes written.
 */
template<typename T>
inline int encodeUnsignedVarint(char* buf, size_t size, T val) {
    val = nativeToLittleEndian(val);
    size_t bytes = 0;
    do {
        uint8_t b = val & 0x7f;
        val >>= 7;
        if (bytes < size) {
            if (val) {
                b |= 0x80;
            }
            *buf++ = b;
        }
        ++bytes;
    } while (val);
    return bytes;
}

/**
 * Decode an unsigned varint.
 *
 * The varint format is described here:
 * https://developers.google.com/protocol-buffers/docs/encoding#varints
 *
 * @param buf Source buffer.
 * @param size Buffer size.
 * @param[out] val Pointer to the resulting value.
 * @return The number of bytes read or a negative result code in case of an error.
 */
template<typename T>
inline int decodeUnsignedVarint(const char* buf, size_t size, T* val) {
    T v = 0;
    size_t bytes = 0;
    size_t bits = 0;
    bool stop = false;
    do {
        if (bytes == size) {
            return SYSTEM_ERROR_NOT_ENOUGH_DATA;
        }
        uint8_t b = *buf++;
        if (!(b & 0x80)) {
            stop = true; // This is the last byte of the encoded value
        } else {
            b &= 0x7f;
        }
        // Make sure the value fits into the destination variable
        if (val && sizeof(unsigned) * 8 - __builtin_clz(b) > sizeof(T) * 8 - bits) {
            return SYSTEM_ERROR_TOO_LARGE;
        }
        v |= (T)b << bits;
        bits += 7;
        ++bytes;
    } while (!stop);
    if (val) {
        *val = littleEndianToNative(v);
    }
    return bytes;
}

/**
 * Returns the maximum number of bytes that can be taken by a varint when encoding a value of a
 * given unsigned integer type.
 *
 * @tparam T Integer type.
 * @return Maximum size of a varint in bytes.
 */
template<typename T>
constexpr size_t maxUnsignedVarintSize() {
    return (sizeof(T) * 8 + 6) / 7;
}

} // particle
