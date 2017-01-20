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

#ifndef SPARK_WIRING_FLAGS_H
#define SPARK_WIRING_FLAGS_H

#include <type_traits>

// Helper macro defining global operators for a specified enum type T
#define PARTICLE_DEFINE_FLAG_OPERATORS(T) \
        inline __attribute__((unused)) ::particle::Flags<T> operator|(T flag1, T flag2) { \
            return ::particle::Flags<T>(flag1) | flag2; \
        } \
        inline __attribute__((unused)) ::particle::Flags<T> operator|(T flag, ::particle::Flags<T> flags) { \
            return flags | flag; \
        } \
        inline __attribute__((unused)) ::particle::Flags<T> operator&(T flag, ::particle::Flags<T> flags) { \
            return flags & flag; \
        } \
        inline __attribute__((unused)) ::particle::Flags<T> operator^(T flag, ::particle::Flags<T> flags) { \
            return flags ^ flag; \
        }

namespace particle {

// Class storing or-combinations of enum values in a type-safe way
template<typename T>
class Flags {
public:
    typedef T EnumType;
    typedef typename std::underlying_type<T>::type ValueType;

    explicit Flags(ValueType val = 0);
    Flags(T flag);

    Flags<T> operator|(T flag) const;
    Flags<T> operator|(Flags<T> flags) const;
    Flags<T>& operator|=(T flag);
    Flags<T>& operator|=(Flags<T> flags);

    Flags<T> operator&(T flag) const;
    Flags<T> operator&(Flags<T> flags) const;
    Flags<T>& operator&=(T flag);
    Flags<T>& operator&=(Flags<T> flags);

    Flags<T> operator^(T flag) const;
    Flags<T> operator^(Flags<T> flags) const;
    Flags<T>& operator^=(T flag);
    Flags<T>& operator^=(Flags<T> flags);

    Flags<T> operator~() const;

    Flags<T>& operator=(ValueType val);

    operator ValueType() const;
    bool operator!() const;

    ValueType value() const;

private:
    ValueType val_;
};

} // namespace particle

template<typename T>
inline particle::Flags<T>::Flags(ValueType val) :
        val_(val) {
}

template<typename T>
inline particle::Flags<T>::Flags(T flag) :
        val_((ValueType)flag) {
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator|(T flag) const {
    return Flags<T>(val_ | (ValueType)flag);
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator|(Flags<T> flags) const {
    return Flags<T>(val_ | flags.val_);
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator|=(T flag) {
    val_ |= (ValueType)flag;
    return *this;
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator|=(Flags<T> flags) {
    val_ |= flags.val_;
    return *this;
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator&(T flag) const {
    return Flags<T>(val_ & (ValueType)flag);
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator&(Flags<T> flags) const {
    return Flags<T>(val_ & flags.val_);
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator&=(T flag) {
    val_ &= (ValueType)flag;
    return *this;
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator&=(Flags<T> flags) {
    val_ &= flags.val_;
    return *this;
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator^(T flag) const {
    return Flags<T>(val_ ^ (ValueType)flag);
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator^(Flags<T> flags) const {
    return Flags<T>(val_ ^ flags.val_);
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator^=(T flag) {
    val_ ^= (ValueType)flag;
    return *this;
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator^=(Flags<T> flags) {
    val_ ^= flags.val_;
    return *this;
}

template<typename T>
inline particle::Flags<T> particle::Flags<T>::operator~() const {
    return Flags<T>(~val_);
}

template<typename T>
inline particle::Flags<T>& particle::Flags<T>::operator=(ValueType val) {
    val_ = val;
    return *this;
}

template<typename T>
inline particle::Flags<T>::operator ValueType() const {
    return val_;
}

template<typename T>
inline bool particle::Flags<T>::operator!() const {
    return !val_;
}

template<typename T>
inline typename particle::Flags<T>::ValueType particle::Flags<T>::value() const {
    return val_;
}

#endif // SPARK_WIRING_FLAGS_H
