/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "appender.h"

#include <string>

namespace particle {

namespace test {

class StringAppender: public Appender {
public:
    using Appender::append;

    bool append(const uint8_t* data, size_t size) override {
        data_.append((const char*)data, size);
        return true;
    }

    const std::string& data() const {
        return data_;
    }

private:
    std::string data_;
};

} // namespace test

} // namespace particle
