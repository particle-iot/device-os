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
/*
 * lfs utility functions
 *
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LFS_CONFIG_H
#define LFS_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define NO_STATIC_ASSERT
#include "module_info.h"

// #define LFS_NO_DEBUG
// #define LFS_NO_WARN
// #define LFS_NO_ERROR
// #define LFS_NO_ASSERT

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
#define LFS_NO_MALLOC
#else
#include <stdlib.h>
#endif /* MODULE_FUNCTION == MOD_FUNC_BOOTLOADER */

#if !defined(LFS_NO_DEBUG) || !defined(LFS_NO_WARN) || !defined(LFS_NO_ERROR)
#include "logging.h"
#endif /* !defined(LFS_NO_DEBUG) && !defined(LFS_NO_WARN) && !defined(LFS_NO_ERROR) */

#ifndef LFS_NO_DEBUG
#define LFS_DEBUG(fmt, ...) LOG_DEBUG(TRACE, fmt, __VA_ARGS__)
#else
#define LFS_DEBUG(fmt, ...)
#endif /* LFS_NO_DEBUG */

#ifndef LFS_NO_WARN
#define LFS_WARN(fmt, ...) LOG_DEBUG(WARN, fmt, __VA_ARGS__)
#else
#define LFS_WARN(fmt, ...)
#endif /* LFS_NO_WARN */

#ifndef LFS_NO_ERROR
#define LFS_ERROR(fmt, ...) LOG_DEBUG(ERROR, fmt, __VA_ARGS__)
#else
#define LFS_ERROR(fmt, ...)
#endif /* LFS_NO_ERROR */

#ifndef LFS_NO_ASSERT
#include "service_debug.h"
#define LFS_ASSERT(test) SPARK_ASSERT(test)
#else
#define LFS_ASSERT(test)
#endif /* LFS_NO_ASSERT */

// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Min/max functions for unsigned 32-bit numbers
static inline uint32_t lfs_max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

static inline uint32_t lfs_min(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Find the next smallest power of 2 less than or equal to a
static inline uint32_t lfs_npw2(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return 32 - __builtin_clz(a-1);
#else
    uint32_t r = 0;
    uint32_t s;
    a -= 1;
    s = (a > 0xffff) << 4; a >>= s; r |= s;
    s = (a > 0xff  ) << 3; a >>= s; r |= s;
    s = (a > 0xf   ) << 2; a >>= s; r |= s;
    s = (a > 0x3   ) << 1; a >>= s; r |= s;
    return (r | (a >> 1)) + 1;
#endif
}

// Count the number of trailing binary zeros in a
// lfs_ctz(0) may be undefined
static inline uint32_t lfs_ctz(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
    return __builtin_ctz(a);
#else
    return lfs_npw2((a & -a) + 1) - 1;
#endif
}

// Count the number of binary ones in a
static inline uint32_t lfs_popc(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return __builtin_popcount(a);
#else
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
static inline int lfs_scmp(uint32_t a, uint32_t b) {
    return (int)(unsigned)(a - b);
}

// Convert from 32-bit little-endian to native order
static inline uint32_t lfs_fromle32(uint32_t a) {
#if !defined(LFS_NO_INTRINSICS) && ( \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
    return a;
#elif !defined(LFS_NO_INTRINSICS) && ( \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    return __builtin_bswap32(a);
#else
    return (((uint8_t*)&a)[0] <<  0) |
           (((uint8_t*)&a)[1] <<  8) |
           (((uint8_t*)&a)[2] << 16) |
           (((uint8_t*)&a)[3] << 24);
#endif
}

// Convert to 32-bit little-endian from native order
static inline uint32_t lfs_tole32(uint32_t a) {
    return lfs_fromle32(a);
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// Calculate CRC-32 with polynomial = 0x04c11db7
void lfs_crc(uint32_t *crc, const void *buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

// Allocate memory, only used if buffers are not provided to littlefs
static inline void *lfs_malloc(size_t size) {
#ifndef LFS_NO_MALLOC
    return malloc(size);
#else
    return NULL;
#endif
}

// Deallocate memory, only used if buffers are not provided to littlefs
static inline void lfs_free(void *p) {
#ifndef LFS_NO_MALLOC
    free(p);
#endif
}

#endif /* LFS_CONFIG_H */