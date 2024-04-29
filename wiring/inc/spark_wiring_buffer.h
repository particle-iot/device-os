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

#include <utility>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace particle {

class Buffer {
public:
    Buffer() :
            data_(nullptr),
            size_(0) {
    }

    explicit Buffer(size_t size, uint8_t fill = 0) :
            Buffer() {
        resize(size, fill);
    }

    Buffer(const uint8_t* data, size_t size) :
            Buffer() {
        if (resize(size)) {
            std::memcpy(data_, data, size);
        }
    }

    Buffer(const Buffer& buf) :
            Buffer() {
        if (resize(buf.size_)) {
            std::memcpy(data_, buf.data_, size_);
        }
    }

    Buffer(Buffer&& buf) :
            Buffer() {
        swap(*this, buf);
    }

    ~Buffer() {
        std::free(data_);
    }

    bool resize(size_t size, uint8_t fill = 0);

    uint8_t* data() {
        return data_;
    }

    const uint8_t data() const {
        return data_;
    }

    size_t size() const {
        return size_;
    }

    bool isEmpty() const {
        return !size_;
    }

    size_t toHex(char* str, size_t size) const;
    String toHex() const;

    static Buffer fromHex(const String& str) {
        return fromHex(str.c_str(), str.length());
    }

    static Buffer fromHex(const char* str) {
        return fromHex(str, std::strlen(str));
    }

    static Buffer fromHex(const char* str, size_t len);

    Buffer& operator=(Buffer buf) {
        swap(*this, buf);
        return *this;
    }

private:
    uint8_t* data_;
    size_t size_;

    friend void swap(Buffer& buf1, Buffer& buf2) {
        using std::swap;
        swap(buf1.data_, buf2.data_);
        swap(buf1.size_, buf2.size_);
    }
};

} // namespace particle
