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

#include <cstddef>

#include "coap_defs.h"

namespace particle::protocol::v2 {

class CoapToken {
public:
    CoapToken() :
            size_(0) {
    }

    CoapToken(const char* data, size_t size);

    const char* data() const {
        return data_;
    }

    size_t size() const {
        return size_;
    }

    bool isEmpty() const {
        return !size_;
    }

    CoapToken& increment();

    int compareWith(const CoapToken& token) const;

    static CoapToken generate(size_t size);

    bool operator==(const CoapToken& token) const {
        return compareWith(token) == 0;
    }

    bool operator!=(const CoapToken& token) const {
        return compareWith(token) != 0;
    }

    bool operator<(const CoapToken& token) const {
        return compareWith(token) < 0;
    }

    bool operator<=(const CoapToken& token) const {
        return compareWith(token) <= 0;
    }

    bool operator>(const CoapToken& token) const {
        return compareWith(token) > 0;
    }

    bool operator>=(const CoapToken& token) const {
        return compareWith(token) >= 0;
    }

private:
    char data_[MAX_COAP_TOKEN_SIZE];
    size_t size_;
};

} // particle::protocol::v2
