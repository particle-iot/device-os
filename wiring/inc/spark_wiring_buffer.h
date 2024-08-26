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

#pragma once

#include <cstring>
#include <cstdint>

#include "spark_wiring_string.h"
#include "spark_wiring_vector.h"

namespace particle {

class Buffer {
public:
    Buffer() = default;

    explicit Buffer(size_t size) :
            d_(size) {
    }

    Buffer(const char* data, size_t size) :
            d_(data, size) {
    }

    Buffer(const uint8_t* data, size_t size) :
            d_((const char*)data, size) {
    }

    char* data() {
        return d_.data();
    }

    const char* data() const {
        return d_.data();
    }

    size_t size() const {
        return d_.size();
    }

    bool isEmpty() const {
        return d_.isEmpty();
    }

    bool resize(size_t size) {
        return d_.resize(size);
    }

    size_t capacity() const {
        return d_.capacity();
    }

    bool reserve(size_t size) {
        return d_.reserve(size);
    }

    bool trimToSize() {
        return d_.trimToSize();
    }

    String toHex() const;

    bool operator==(const Buffer& buf) const {
        if (d_.size() != buf.d_.size()) {
            return false;
        }
        return std::memcmp(d_.data(), buf.d_.data(), d_.size()) == 0;
    }

    bool operator!=(const Buffer& buf) const {
        return !(operator==(buf));
    }

    static Buffer fromHex(const String& str) {
        return fromHex(str.c_str(), str.length());
    }

    static Buffer fromHex(const char* str) {
        return fromHex(str, std::strlen(str));
    }

    static Buffer fromHex(const char* str, size_t len);

private:
    Vector<char> d_;
};

} // namespace particle
