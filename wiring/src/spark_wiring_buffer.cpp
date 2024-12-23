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

#include "spark_wiring_buffer.h"

#include "str_util.h"

namespace particle {

Buffer Buffer::slice(size_t pos, size_t size) const {
    if (pos > (size_t)d_.size()) {
        pos = d_.size();
    }
    if ((size_t)d_.size() - pos < size) {
        size = d_.size() - pos;
    }
    Buffer buf;
    if (!buf.resize(size)) {
        return Buffer();
    }
    std::memcpy(buf.d_.data(), d_.data() + pos, size);
    return buf;
}

Buffer Buffer::concat(const Buffer& buf) const {
    Buffer b;
    if (!b.resize(d_.size() + buf.d_.size())) {
        return Buffer();
    }
    std::memcpy(b.d_.data(), d_.data(), d_.size());
    std::memcpy(b.d_.data() + d_.size(), buf.d_.data(), buf.d_.size());
    return b;
}

String Buffer::toHex() const {
    String s;
    if (!s.resize(d_.size() * 2)) {
        return String();
    }
    toHex(&s.operator[](0), s.length() + 1); // Overwrites the term. null
    return s;
}

size_t Buffer::toHex(char* out, size_t size) const {
    return particle::toHex(d_.data(), d_.size(), out, size);
}

Buffer Buffer::fromHex(const char* str, size_t len) {
    Buffer buf;
    if (!buf.resize(len / 2)) {
        return Buffer();
    }
    size_t n = particle::fromHex(str, len, buf.d_.data(), buf.d_.size());
    buf.resize(n);
    return buf;
}

} // namespace particle
