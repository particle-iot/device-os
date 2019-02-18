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

size_t escape(const char* src, const char* spec, char esc, char* dest, size_t destSize) {
    size_t n = 0;
    for (;;) {
        const char c = *src++;
        if (!c) {
            break;
        }
        if (strchr(spec, c) != nullptr) {
            if (n < destSize) {
                dest[n] = esc;
            }
            ++n;
        }
        if (n < destSize) {
            dest[n] = c;
        }
        ++n;
    }
    if (n < destSize) {
        dest[n] = '\0';
    } else if (destSize > 0) {
        dest[destSize - 1] = '\0';
    }
    return n;
}

} // particle
