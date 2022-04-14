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

namespace particle {

inline int hexToNibble(char c) {
    if (c >= '0' && c <= '9') {
        return (c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 0x0a);
    } else if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 0x0a);
    }
    return -1;
}

inline size_t hexToBytes(const char* src, char* dest, size_t size) {
    size_t n = 0;
    while (n < size) {
        const int h = hexToNibble(*src++);
        if (h < 0) {
            break;
        }
        const int l = hexToNibble(*src++);
        if (l < 0) {
            break;
        }
        *dest++ = ((unsigned)h << 4) | (unsigned)l;
        ++n;
    }
    return n;
}

} // namespace particle
