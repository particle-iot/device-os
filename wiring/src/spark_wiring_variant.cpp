/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include "spark_wiring_variant.h"

namespace particle {

namespace detail {

#ifndef __cpp_lib_to_chars

std::to_chars_result to_chars(char* first, char* last, double value) {
    std::to_chars_result res;
    int n = std::snprintf(first, last - first, "%g", value);
    if (n < 0 || n >= last - first) {
        res.ec = std::errc::value_too_large;
        res.ptr = last;
    } else {
        res.ec = std::errc();
        res.ptr = first + n;
    }
    return res;
}

std::from_chars_result from_chars(const char* first, const char* last, double& value) {
    std::from_chars_result res;
    if (last > first) {
        char* end = nullptr;
        errno = 0;
        double v = strtod(first, &end);
        if (errno == ERANGE) {
            res.ec = std::errc::result_out_of_range;
            res.ptr = end;
        } else if (end == first || std::isspace((unsigned char)*first)) {
            res.ec = std::errc::invalid_argument;
            res.ptr = first;
        } else {
            res.ec = std::errc();
            res.ptr = end;
            value = v;
        }
    } else {
        res.ec = std::errc::invalid_argument;
        res.ptr = first;
    }
    return res;
}

#endif // !defined(__cpp_lib_to_chars)

} // namespace detail

} // namespace particle
