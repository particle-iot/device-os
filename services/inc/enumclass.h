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

#ifndef SERVICES_ENUMCLASS_H
#define SERVICES_ENUMCLASS_H

#include <type_traits>

namespace particle {

template <typename T, typename Enable = void>
class BitMaskFlags;

// template< typename T, typename = typename std::enable_if_t<std::is_enum<T>::value> >
// class BitMaskFlags<T> {
template< typename T>
class BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>> {
public:
    BitMaskFlags();
    BitMaskFlags(T value);

    BitMaskFlags<T> operator&(T value);
    BitMaskFlags<T>& operator&=(T value);
    BitMaskFlags<T> operator&(BitMaskFlags<T> flags);
    BitMaskFlags<T>& operator&=(BitMaskFlags<T> flags);
    BitMaskFlags<T> operator|(T value);
    BitMaskFlags<T>& operator|=(T value);
    BitMaskFlags<T> operator|(BitMaskFlags<T> flags);
    BitMaskFlags<T>& operator|=(BitMaskFlags<T> flags);
    BitMaskFlags<T> operator^(T value);
    BitMaskFlags<T> operator^=(T value);
    BitMaskFlags<T> operator^(BitMaskFlags<T> flags);
    BitMaskFlags<T>& operator^=(BitMaskFlags<T> flags);

    typename std::underlying_type<T>::type value();

    // Used for the case: if ( true && BitMaskFlags<T>) {}
    operator bool() {
        return (value_ != 0);
    }

private:
    typename std::underlying_type<T>::type value_;
};

template< typename T, typename = typename std::enable_if_t<std::is_enum<T>::value> >
inline BitMaskFlags<T> operator&(T lhs, T rhs) {
    return BitMaskFlags<T>(lhs) & BitMaskFlags<T>(rhs);
}

template< typename T, typename = typename std::enable_if_t<std::is_enum<T>::value> >
inline BitMaskFlags<T> operator|(T& lhs, T rhs) {
    return BitMaskFlags<T>(lhs) | BitMaskFlags<T>(rhs);
}

template< typename T, typename = typename std::enable_if_t<std::is_enum<T>::value> >
inline BitMaskFlags<T> operator^(T& lhs, T rhs) {
    return BitMaskFlags<T>(lhs) ^ BitMaskFlags<T>(rhs);
}

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

} /* particle */

template< typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags()
        : value_(0) {
}

template< typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags(T value) {
    value_ = static_cast< typename std::underlying_type<T>::type >(value);
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&(T value) {
    value_ &= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&=(T value) {
    value_ &= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&(BitMaskFlags<T> flags) {
    value_ &= flags.value_;
    return *this;
}

template< typename T>
inline BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&=(BitMaskFlags<T> flags) {
    value_ &= flags.value_;
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|(T value) {
    value_ |= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|=(T value) {
    value_ |= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|(BitMaskFlags<T> flags) {
    value_ |= flags.value_;
    return *this;
}

template< typename T>
inline BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|=(BitMaskFlags<T> flags) {
    value_ |= flags.value_;
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^(T value) {
    value_ ^= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^=(T value) {
    value_ ^= BitMaskFlags<T>(value);
    return *this;
}

template< typename T>
inline BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^(BitMaskFlags<T> flags) {
    value_ ^= flags.value_;
    return *this;
}

template< typename T>
inline BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^=(BitMaskFlags<T> flags) {
    value_ ^= flags.value_;
    return *this;
}

template< typename T>
inline typename std::underlying_type<T>::type particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::value() {
    return value_;
}

#endif /* SERVICES_ENUMCLASS_H */
