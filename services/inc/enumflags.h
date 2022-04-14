/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERVICES_ENUMFLAGS_H
#define SERVICES_ENUMFLAGS_H

#include <type_traits>
#include <cstddef>
#include <algorithm>

namespace particle {

template <typename T, typename Enable = void>
class EnumFlags;

template<typename T>
struct EnableBitwise {
    static const bool enable = false;
};

#define ENABLE_ENUM_CLASS_BITWISE(T) \
template<>                           \
struct EnableBitwise<T> {            \
    static const bool enable = true; \
}

template<typename T>
class EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>> {
public:
    typedef typename std::underlying_type_t<T> ValueType;

    EnumFlags();
    EnumFlags(const T& value);

    EnumFlags<T>& set(const EnumFlags<T>& flags);
    EnumFlags<T>& clear(const EnumFlags<T>& flags);

    ValueType value() const;

    bool isSet() const;
    bool isSet(const EnumFlags<T>& flags) const;

    EnumFlags<T> operator&(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator&=(const EnumFlags<T>& flags);

    EnumFlags<T> operator|(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator|=(const EnumFlags<T>& flags);

    EnumFlags<T> operator^(const EnumFlags<T>& flags) const;
    EnumFlags<T>& operator^=(const EnumFlags<T>& flags);

    EnumFlags<T> operator~() const;

    EnumFlags<T> operator<<(size_t n) const;
    EnumFlags<T>& operator<<=(size_t n);
    EnumFlags<T> operator>>(size_t n) const;
    EnumFlags<T>& operator>>=(size_t n);

    bool operator==(const EnumFlags<T>& flags) const;
    bool operator!=(const EnumFlags<T>& flags) const;
    bool operator>(const EnumFlags<T>& flags) const;
    bool operator>=(const EnumFlags<T>& flags) const;
    bool operator<(const EnumFlags<T>& flags) const;
    bool operator<=(const EnumFlags<T>& flags) const;

    operator bool() const;
    bool operator!() const;

    static EnumFlags<T> fromUnderlying(ValueType value);

private:
    ValueType value_;

    explicit EnumFlags(ValueType value);
};

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator&(const T& lhs, const T& rhs) {
    return EnumFlags<T>(lhs) & EnumFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator&(const T& lhs, const EnumFlags<T>& rhs) {
    return EnumFlags<T>(lhs) & rhs;
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator|(const T& lhs, const T& rhs) {
    return EnumFlags<T>(lhs) | EnumFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator|(const T& lhs, const EnumFlags<T>& rhs) {
    return EnumFlags<T>(lhs) | rhs;
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator^(const T& lhs, const T& rhs) {
    return EnumFlags<T>(lhs) ^ EnumFlags<T>(rhs);
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator^(const T& lhs, const EnumFlags<T>& rhs) {
    return EnumFlags<T>(lhs) ^ rhs;
}

template<typename T, typename = typename std::enable_if_t<std::is_enum<T>::value && EnableBitwise<T>::enable>>
inline EnumFlags<T> operator~(const T& lhs) {
    return ~EnumFlags<T>(lhs);
}

} /* particle */


// particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>
template<typename T>
inline particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::EnumFlags()
        : value_(0) {
}

template<typename T>
inline particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::EnumFlags(const T& value) {
    value_ = static_cast<ValueType>(value);
}

template<typename T>
inline particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::EnumFlags(ValueType value)
        : value_(value) {
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::set(const EnumFlags<T>& flags) {
    value_ |= flags.value_;
    return *this;
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::clear(const EnumFlags<T>& flags) {
    value_ &= (~flags.value_);
    return *this;
}

template<typename T>
inline typename std::underlying_type_t<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::value() const {
    return value_;
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::isSet() const {
    return (value_ > 0);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::isSet(const EnumFlags<T>& flags) const {
    return ((value_ & flags.value_) == flags.value_);
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&(const EnumFlags<T>& flags) const {
    return EnumFlags<T>(value_ & flags.value_);
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator&=(const EnumFlags<T>& flags) {
    value_ &= flags.value_;
    return *this;
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|(const EnumFlags<T>& flags) const {
    return EnumFlags<T>(value_ | flags.value_);
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator|=(const EnumFlags<T>& flags) {
    value_ |= flags.value_;
    return *this;
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^(const EnumFlags<T>& flags) const {
    return EnumFlags<T>(value_ ^ flags.value_);
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator^=(const EnumFlags<T>& flags) {
    value_ ^= flags.value_;
    return *this;
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator~() const {
    return EnumFlags<T>(~value_);
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<<(size_t n) const {
    return EnumFlags<T>(value_ << n);
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<<=(size_t n) {
    value_ <<= n;
    return *this;
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>>(size_t n) const {
    return EnumFlags<T>(value_ >> n);
}

template<typename T>
inline particle::EnumFlags<T>& particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>>=(size_t n) {
    value_ >>= n;
    return *this;
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator==(const EnumFlags<T>& flags) const {
    return (value_ == flags.value_);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator!=(const EnumFlags<T>& flags) const {
    return (value_ != flags.value_);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>(const EnumFlags<T>& flags) const {
    return (value_ > flags.value_);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator>=(const EnumFlags<T>& flags) const {
    return (value_ >= flags.value_);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<(const EnumFlags<T>& flags) const {
    return (value_ < flags.value_);
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator<=(const EnumFlags<T>& flags) const {
    return (value_ <= flags.value_);
}

template<typename T>
inline particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator bool() const {
    return value_;
}

template<typename T>
inline bool particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::operator!() const {
    return !value_;
}

template<typename T>
inline particle::EnumFlags<T> particle::EnumFlags<T, typename std::enable_if_t<std::is_enum<T>::value>>::fromUnderlying(ValueType value) {
    return EnumFlags<T>(value);
}

#endif /* SERVICES_ENUMFLAGS_H */
