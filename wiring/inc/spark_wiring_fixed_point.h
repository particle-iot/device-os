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
#include <cstdint>
#include <cstddef>
#include <limits>

namespace particle {

template<bool S, size_t bits, typename T>
struct bits_fit_in_type {
    using type = typename std::conditional<S, typename std::make_signed<T>::type, typename std::make_unsigned<T>::type>::type;
    static const bool value = (bits <= std::numeric_limits<type>::digits);
};

template<bool S, size_t bits>
struct minimum_int_for_bits {
    using type = typename std::conditional<
        bits_fit_in_type<S, bits, int8_t>::value, typename bits_fit_in_type<S, bits, int8_t>::type,
            typename std::conditional<bits_fit_in_type<S, bits, int16_t>::value, typename bits_fit_in_type<S, bits, int16_t>::type,
                typename std::conditional<bits_fit_in_type<S, bits, int32_t>::value, typename bits_fit_in_type<S, bits, int32_t>::type,
                    typename std::conditional<bits_fit_in_type<S, bits, int64_t>::value, typename bits_fit_in_type<S, bits, int64_t>::type, void>::type >::type >::type >::type;
};

template<typename T, typename U>
constexpr T constexpr_pow(T b, U e) {
    return e == 0 ? 1 : b * constexpr_pow(b, e - 1);
}

template <bool S, size_t M, size_t N, typename T = void>
class FixedPointQ {
public:
    using StorageT = typename std::conditional<std::is_same<T, void>::value, typename minimum_int_for_bits<S, M + N>::type, T>::type;
    using type = StorageT;

    constexpr FixedPointQ(StorageT v = 0) :
        value_(v) {}
    constexpr FixedPointQ(float v) :
        value_(fromFloat(v)) {}
    constexpr FixedPointQ(double v) :
        value_(fromDouble(v)) {}

    StorageT value() const {
        return value_;
    }

    constexpr float toFloat() const {
        return static_cast<float>(value_) / constexpr_pow(2.0f, N);
    }

    constexpr float toDouble() const {
        return static_cast<double>(value_) / constexpr_pow(2.0, N);
    }

    operator StorageT() const {
        return value();
    }
private:
    template<typename U>
    static constexpr StorageT fromFloat(const U v) {
        return static_cast<StorageT>(v * constexpr_pow(2.0f, N));
    }
    template<typename U>
    static constexpr StorageT fromDouble(const U v) {
        return static_cast<StorageT>(v * constexpr_pow(2.0, N));
    }

    static_assert(!std::is_same<void, StorageT>::value, "FixedPointQ: Couldn't deduce storage type");
    static_assert(bits_fit_in_type<S, M + N, StorageT>::value, "FixedPointQ: Storage type will not be able to hold specified number of bits");
    StorageT value_;
};

template <size_t M, size_t N>
using FixedPointSQ = FixedPointQ<true, M - 1, N>;

template <size_t M, size_t N>
using FixedPointUQ = FixedPointQ<false, M, N>;

} // particle