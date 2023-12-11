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

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>
#include <cstring>
#include "system_error.h"

namespace particle {

class Buffer {
public:
    explicit Buffer(size_t size = 0);
    Buffer(const char* data, size_t size);
    Buffer(const uint8_t* data, size_t size);
    Buffer(const Buffer& other);
    Buffer(Buffer&& other);

    char* data();
    const char* data() const;

    size_t size() const;

    operator char*();
    operator const char*() const;

    Buffer& operator=(Buffer other);

    bool operator==(const Buffer& other) const;
    bool operator!=(const Buffer& other) const;

private:
    friend void swap(Buffer& lhs, Buffer& rhs) {
        using std::swap;
        swap(lhs.buffer_, rhs.buffer_);
        swap(lhs.size_, rhs.size_);
    }

private:
    std::unique_ptr<char[]> buffer_;
    size_t size_;
};

inline Buffer::Buffer(size_t size)
        : size_(size) {
    if (size_ > 0) {
        buffer_ = std::make_unique<char[]>(size_);
        if (!buffer_) {
            size_ = 0;
        }
    }
}

inline Buffer::Buffer(const char* data, size_t size)
        : Buffer(size) {
    if (buffer_ && data && size > 0) {
        memcpy(buffer_.get(), data, size);
    }
}

inline Buffer::Buffer(const uint8_t* data, size_t size)
        : Buffer((const char*)data, size) {

}
inline Buffer::Buffer(const Buffer& other)
        : Buffer(other.size()) {
    if (size_ > 0 && other.size() > 0) {
        memcpy(buffer_.get(), other.buffer_.get(), size_);
    }
}

inline Buffer::Buffer(Buffer&& other)
        : Buffer() {
    swap(*this, other);
}

inline char* Buffer::data() {
    return buffer_.get();
}

inline const char* Buffer::data() const {
    return buffer_.get();
}

inline size_t Buffer::size() const {
    return size_;
}

inline Buffer::operator char*() {
    return data();
}

inline Buffer::operator const char*() const {
    return data();
}

inline Buffer& Buffer::operator=(Buffer other) {
    swap(*this, other);
    return *this;
}

inline bool Buffer::operator==(const Buffer& other) const {
    if (size() != other.size()) {
        return false;
    }
    if (size() > 0 && data() && other.data()) {
        return !memcmp(buffer_.get(), other.buffer_.get(), size());
    }
    return size() == 0 && other.size() == 0;
}

inline bool Buffer::operator!=(const Buffer& other) const {
    return !(*this == other);
}

} // particle
