/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include <type_traits>

// This macro defines operators for comparing values of an enum with values of an integral type.
// This can be useful when handling the result of a function that can either return a negative
// error code or some non-error value defined by an enum class
#define PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(_type) \
        inline bool operator==(_type a, std::underlying_type<_type>::type b) { \
            return a == (_type)b; \
        } \
        inline bool operator==(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a == b; \
        } \
        inline bool operator!=(_type a, std::underlying_type<_type>::type b) { \
            return a != (_type)b; \
        } \
        inline bool operator!=(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a != b; \
        } \
        inline bool operator<(_type a, std::underlying_type<_type>::type b) { \
            return a < (_type)b; \
        } \
        inline bool operator<(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a < b; \
        } \
        inline bool operator<=(_type a, std::underlying_type<_type>::type b) { \
            return a <= (_type)b; \
        } \
        inline bool operator<=(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a <= b; \
        } \
        inline bool operator>(_type a, std::underlying_type<_type>::type b) { \
            return a > (_type)b; \
        } \
        inline bool operator>(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a > b; \
        } \
        inline bool operator>=(_type a, std::underlying_type<_type>::type b) { \
            return a >= (_type)b; \
        } \
        inline bool operator>=(std::underlying_type<_type>::type a, _type b) { \
            return (_type)a >= b; \
        }

namespace particle {

// Wrapper for std::underlying_type that falls back to the original type if it is not an enum
template<typename T, typename EnableT = void>
struct UnderlyingType {
    typedef T Type;
};

template<typename T>
struct UnderlyingType<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    typedef typename std::underlying_type<T>::type Type;
};

} // namespace particle
