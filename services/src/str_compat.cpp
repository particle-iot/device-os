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

#include "str_compat.h"

#if PLATFORM_ID == PLATFORM_GCC && defined(__GLIBC__)

size_t strlcpy(char* dest, const char* src, size_t size) {
    auto srcLen = strlen(src);
    if (srcLen + 1 <= size) {
        memcpy(dest, src, srcLen + 1);
    } else if (size > 0) {
        memcpy(dest, src, size - 1);
        dest[size - 1] = '\0';
    }
    return srcLen;
}

#endif // PLATFORM_ID == PLATFORM_GCC && defined(__GLIBC__)
