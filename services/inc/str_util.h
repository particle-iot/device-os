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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cctype>

namespace particle {

inline char* toUpperCase(char* str, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        str[i] = std::toupper((unsigned char)str[i]);
    }
    return str;
}

inline char* toUpperCase(char* str) {
    return toUpperCase(str, std::strlen(str));
}

inline char* toLowerCase(char* str, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        str[i] = std::tolower((unsigned char)str[i]);
    }
    return str;
}

inline char* toLowerCase(char* str) {
    return toLowerCase(str, std::strlen(str));
}

inline bool isPrintable(const char* str, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (!std::isprint((unsigned char)str[i])) {
            return false;
        }
    }
    return true;
}

inline bool isPrintable(const char* str) {
    return isPrintable(str, std::strlen(str));
}

inline bool startsWith(const char* str, size_t strSize, const char* prefix, size_t prefixSize) {
    if (strSize < prefixSize) {
        return false;
    }
    if (strncmp(str, prefix, prefixSize) != 0) {
        return false;
    }
    return true;
}

inline bool startsWith(const char* str, const char* prefix) {
    return startsWith(str, strlen(str), prefix, strlen(prefix));
}

inline bool endsWith(const char* str, size_t strSize, const char* suffix, size_t suffixSize) {
    if (strSize < suffixSize) {
        return false;
    }

    if (strncmp(str + strSize - suffixSize, suffix, suffixSize) != 0) {
        return false;
    }

    return true;
}

inline bool endsWith(const char* str, const char* suffix) {
    return endsWith(str, strlen(str), suffix, strlen(suffix));
}

/**
 * Escapes a set of characters in a string by prepending them with an escape character. The output
 * is always null-terminated, unless the size of the destination buffer is `0`.
 *
 * @param src Source string.
 * @param spec Characters that need to be escaped.
 * @param esc Escape character.
 * @param dest Destination buffer.
 * @param destSize Size of the destination buffer.
 *
 * @return Number of characters in the entire escaped string, not including the trailing `\0`.
 *         The value can be larger than the size of the destination buffer.
 */
size_t escape(const char* src, const char* spec, char esc, char* dest, size_t destSize);

/**
 * Converts binary data to a hex-encoded string. The output is always null-terminated, unless the
 * size of the destination buffer is 0.
 *
 * @param src Source data.
 * @param srcSize Size of the source data.
 * @param dest Destination buffer.
 * @param destSize Size of the destination buffer.
 *
 * @return Number of characters written to the destination buffer, not including the trailing `\0`.
 */
inline size_t toHex(const void* src, size_t srcSize, char* dest, size_t destSize) {
    static const char alpha[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    size_t n = 0;
    auto srcBytes = (const uint8_t*)src;
    for (size_t i = 0; i < srcSize && n + 1 < destSize; ++i) {
        const auto b = srcBytes[i];
        dest[n++] = alpha[b >> 4];
        if (n + 1 < destSize) {
            dest[n++] = alpha[b & 0x0f];
        }
    }
    if (n < destSize) {
        dest[n] = '\0';
    }
    return n;
}

/**
 * Converts binary data to a printable string. The output is null-terminated unless the size of the
 * destination buffer is 0.
 *
 * @param src Source data.
 * @param srcSize Size of the source data.
 * @param dest Destination buffer.
 * @param destSize Size of the destination buffer.
 *
 * @return Number of characters written to the destination buffer not including the trailing `\0`.
 */
inline size_t toPrintable(const char* src, size_t srcSize, char* dest, size_t destSize) {
    size_t pos = 0;
    char hex[5] = { '\\', 'x' };
    for (size_t i = 0; i < srcSize && pos + 1 < destSize; ++i) {
        if (std::isprint((unsigned char)src[i])) {
            dest[pos++] = src[i];
        } else {
            snprintf(hex + 2, sizeof(hex) - 2, "%02x", (unsigned char)src[i]);
            auto n = std::min(sizeof(hex) - 1, destSize - pos - 1);
            memcpy(dest + pos, hex, n);
            pos += n;
        }
    }
    if (pos < destSize) {
        dest[pos] = '\0';
    }
    return pos;
}

} // particle
