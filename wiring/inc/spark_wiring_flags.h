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

namespace particle {

template<typename TagT, typename ValueT>
class Flags;

// Class storing a typed flag value
template<typename TagT, typename ValueT = unsigned>
class Flag {
public:
    explicit Flag(ValueT val);

    Flags<TagT, ValueT> operator|(Flag<TagT, ValueT> flag) const;
    Flags<TagT, ValueT> operator|(Flags<TagT, ValueT> flags) const;
    Flags<TagT, ValueT> operator&(Flags<TagT, ValueT> flags) const;
    Flags<TagT, ValueT> operator^(Flags<TagT, ValueT> flags) const;

    explicit operator ValueT() const;

    ValueT value() const;

private:
    ValueT val_;
};

// Class storing or-combinations of typed flag values
template<typename TagT, typename ValueT = unsigned>
class Flags {
public:
    typedef TagT TagType;
    typedef ValueT ValueType;
    typedef Flag<TagT, ValueT> FlagType;

    Flags();
    Flags(Flag<TagT, ValueT> flag);

    Flags<TagT, ValueT> operator|(Flags<TagT, ValueT> flags) const;
    Flags<TagT, ValueT>& operator|=(Flags<TagT, ValueT> flags);

    Flags<TagT, ValueT> operator&(Flags<TagT, ValueT> flags) const;
    Flags<TagT, ValueT>& operator&=(Flags<TagT, ValueT> flags);

    Flags<TagT, ValueT> operator^(Flags<TagT, ValueT> flags) const;
    Flags<TagT, ValueT>& operator^=(Flags<TagT, ValueT> flags);

    Flags<TagT, ValueT> operator~() const;

    explicit operator ValueT() const;
    explicit operator bool() const;

    ValueT value() const;

private:
    ValueT val_;

    explicit Flags(ValueT val);
};

} // namespace particle

// particle::Flag<TagT, ValueT>
template<typename TagT, typename ValueT>
inline particle::Flag<TagT, ValueT>::Flag(ValueT val) :
        val_(val) {
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flag<TagT, ValueT>::operator|(Flag<TagT, ValueT> flag) const {
    return (Flags<TagT, ValueT>(*this) | flag);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flag<TagT, ValueT>::operator|(Flags<TagT, ValueT> flags) const {
    return (flags | *this);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flag<TagT, ValueT>::operator&(Flags<TagT, ValueT> flags) const {
    return (flags & *this);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flag<TagT, ValueT>::operator^(Flags<TagT, ValueT> flags) const {
    return (flags ^ *this);
}

template<typename TagT, typename ValueT>
inline particle::Flag<TagT, ValueT>::operator ValueT() const {
    return val_;
}

template<typename TagT, typename ValueT>
inline ValueT particle::Flag<TagT, ValueT>::value() const {
    return val_;
}

// particle::Flags<TagT, ValueT>
template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>::Flags() :
        val_(0) {
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>::Flags(Flag<TagT, ValueT> flag) :
        val_(flag.value()) {
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>::Flags(ValueT val) :
        val_(val) {
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flags<TagT, ValueT>::operator|(Flags<TagT, ValueT> flags) const {
    return Flags<TagT, ValueT>(val_ | flags.val_);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>& particle::Flags<TagT, ValueT>::operator|=(Flags<TagT, ValueT> flags) {
    val_ |= flags.val_;
    return *this;
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flags<TagT, ValueT>::operator&(Flags<TagT, ValueT> flags) const {
    return Flags<TagT, ValueT>(val_ & flags.val_);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>& particle::Flags<TagT, ValueT>::operator&=(Flags<TagT, ValueT> flags) {
    val_ &= flags.val_;
    return *this;
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flags<TagT, ValueT>::operator^(Flags<TagT, ValueT> flags) const {
    return Flags<TagT, ValueT>(val_ ^ flags.val_);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>& particle::Flags<TagT, ValueT>::operator^=(Flags<TagT, ValueT> flags) {
    val_ ^= flags.val_;
    return *this;
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT> particle::Flags<TagT, ValueT>::operator~() const {
    return Flags<TagT, ValueT>(~val_);
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>::operator ValueT() const {
    return val_;
}

template<typename TagT, typename ValueT>
inline particle::Flags<TagT, ValueT>::operator bool() const {
    return val_;
}

template<typename TagT, typename ValueT>
inline ValueT particle::Flags<TagT, ValueT>::value() const {
    return val_;
}

#endif // SPARK_WIRING_FLAGS_H
