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

#include "str_util.h"

namespace particle {

char* escape(const char* src, size_t srcSize, const char* spec, size_t specSize, char esc, char* dest, size_t destSize) {
    size_t i = 0;
    size_t j = 0;
    while (i < srcSize && j < destSize) {
        const char c = src[i++];
        if (memchr(spec, (unsigned char)c, specSize) != nullptr) {
            if (j == destSize - 1) {
                break; // Avoid generating an invalid escaped string
            }
            dest[j++] = esc;
        }
        dest[j++] = c;
    }
    return dest;
}

} // particle
