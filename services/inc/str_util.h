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

#include <cstring>
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

} // particle
