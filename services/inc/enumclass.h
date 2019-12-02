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

template<typename T>
class BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>> {
public:
    typedef typename std::underlying_type<T>::type ValueType;
    
    BitMaskFlags();
    BitMaskFlags(T value);
    BitMaskFlags(const BitMaskFlags<T>& flags);

    BitMaskFlags<T> operator&(BitMaskFlags<T> flags) const;
    BitMaskFlags<T>& operator&=(BitMaskFlags<T> flags);

    BitMaskFlags<T> operator|(BitMaskFlags<T> flags) const;
    BitMaskFlags<T>& operator|=(BitMaskFlags<T> flags);

    BitMaskFlags<T> operator^(BitMaskFlags<T> flags) const;
    BitMaskFlags<T>& operator^=(BitMaskFlags<T> flags);

    BitMaskFlags<T> operator~() const;

    BitMaskFlags<T> operator<<(size_t n) const;
    BitMaskFlags<T>& operator<<=(size_t n);
    BitMaskFlags<T> operator>>(size_t n) const;
    BitMaskFlags<T>& operator>>=(size_t n);

    typename std::underlying_type<T>::type value() const;

    operator bool() const;
    operator ValueType() const;

    bool operator==(BitMaskFlags<T> flags) const;
    bool operator!=(BitMaskFlags<T> flags) const;
    bool operator>(BitMaskFlags<T> flags) const;
    bool operator>=(BitMaskFlags<T> flags) const;
    bool operator<(BitMaskFlags<T> flags) const;
    bool operator<=(BitMaskFlags<T> flags) const;

private:
    typename std::underlying_type<T>::type value_;

    explicit BitMaskFlags(typename std::underlying_type<T>::type value);
};

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value>>
inline BitMaskFlags<T> operator&(T lhs, T rhs) {
    return BitMaskFlags<T>(lhs) & BitMaskFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value>>
inline BitMaskFlags<T> operator|(T& lhs, T rhs) {
    return BitMaskFlags<T>(lhs) | BitMaskFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value>>
inline BitMaskFlags<T> operator^(T& lhs, T rhs) {
    return BitMaskFlags<T>(lhs) ^ BitMaskFlags<T>(rhs);
}

template<typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

} /* particle */


// particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>
template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags()
        : value_(0) {
}

template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags(T value) {
    value_ = static_cast< typename std::underlying_type<T>::type >(value);
}

template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags(const BitMaskFlags<T>& flags) {
    value_ = flags.value_;
}

template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::BitMaskFlags(typename std::underlying_type<T>::type value)
        : value_(value) {
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&(BitMaskFlags<T> flags) const {
    return BitMaskFlags<T>(value_ & flags.value_);
}

template<typename T>
inline particle::BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&=(BitMaskFlags<T> flags) {
    value_ &= flags.value_;
    return *this;
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|(BitMaskFlags<T> flags) const {
    return BitMaskFlags<T>(value_ | flags.value_);
}

template<typename T>
inline particle::BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|=(BitMaskFlags<T> flags) {
    value_ |= flags.value_;
    return *this;
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^(BitMaskFlags<T> flags) const {
    return BitMaskFlags<T>(value_ ^ flags.value_);
}

template<typename T>
inline particle::BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^=(BitMaskFlags<T> flags) {
    value_ ^= flags.value_;
    return *this;
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator~() const {
    return BitMaskFlags<T>(~value_);
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<<(size_t n) const {
    return BitMaskFlags<T>(value_ << n);
}

template<typename T>
inline particle::BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<<=(size_t n) {
    value_ <<= n;
    return *this;
}

template<typename T>
inline particle::BitMaskFlags<T> particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>>(size_t n) const {
    return BitMaskFlags<T>(value_ >> n);
}

template<typename T>
inline particle::BitMaskFlags<T>& particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>>=(size_t n) {
    value_ >>= n;
    return *this;
}

template<typename T>
inline typename std::underlying_type<T>::type particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::value() const {
    return value_;
}

template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator bool() const {
    return (value_ != 0);
}

template<typename T>
inline particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator ValueType() const {
    return value_;
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator==(BitMaskFlags<T> flags) const {
    return (value_ == flags.value_);
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator!=(BitMaskFlags<T> flags) const {
    return (value_ != flags.value_);
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>(BitMaskFlags<T> flags) const {
    return (value_ > flags.value_);
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>=(BitMaskFlags<T> flags) const {
    return (value_ >= flags.value_);
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<(BitMaskFlags<T> flags) const {
    return (value_ < flags.value_);
}

template<typename T>
inline bool particle::BitMaskFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<=(BitMaskFlags<T> flags) const {
    return (value_ <= flags.value_);
}

#endif /* SERVICES_ENUMCLASS_H */
