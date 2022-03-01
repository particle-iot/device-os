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

#pragma once

#include <cstdint>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PARTICLE_BIG_ENDIAN 1
#define PARTICLE_LITTLE_ENDIAN 0
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PARTICLE_BIG_ENDIAN 0
#define PARTICLE_LITTLE_ENDIAN 1
#else
#error "Unsupported platform" // TODO: Add support for the PDP-11 architecture
#endif

namespace particle {

inline int8_t reverseByteOrder(int8_t val) {
    return val;
}

inline uint8_t reverseByteOrder(uint8_t val) {
    return val;
}

inline int16_t reverseByteOrder(int16_t val) {
    return __builtin_bswap16(val);
}

inline uint16_t reverseByteOrder(uint16_t val) {
    return __builtin_bswap16(val);
}

inline int32_t reverseByteOrder(int32_t val) {
    return __builtin_bswap32(val);
}

inline uint32_t reverseByteOrder(uint32_t val) {
    return __builtin_bswap32(val);
}

inline int64_t reverseByteOrder(int64_t val) {
    return __builtin_bswap64(val);
}

inline uint64_t reverseByteOrder(uint64_t val) {
    return __builtin_bswap64(val);
}

template<typename T>
inline T nativeToBigEndian(T val) {
#if PARTICLE_BIG_ENDIAN
    return val;
#else
    return reverseByteOrder(val);
#endif
}

template<typename T>
inline T bigEndianToNative(T val) {
    return nativeToBigEndian(val);
}

template<typename T>
inline T nativeToLittleEndian(T val) {
#if PARTICLE_LITTLE_ENDIAN
    return val;
#else
    return reverseByteOrder(val);
#endif
}

template<typename T>
inline T littleEndianToNative(T val) {
    return nativeToLittleEndian(val);
}

} // particle
