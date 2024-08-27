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
