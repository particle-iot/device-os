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

#include <algorithm>
#include <cstring>

#include "event.h"

#include "check.h"

namespace particle::system::cloud {

namespace {

const size_t MAX_PAYLOAD_SIZE = 1024; // FIXME

} // namespace

int Event::name(const char* name) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    size_t len = std::strlen(name);
    if (!len || len > CLOUD_EVENT_MAX_NAME_LENGTH) {
        return error(SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    std::memcpy(name_, name, len + 1);
    return 0;
}

int Event::read(char* data, size_t size) {
    size_t n = CHECK(peek(data, size));
    pos_ += n;
    return n;
}

int Event::peek(char* data, size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    if (pos_ == data_.size()) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    size_t n = std::min(data_.size() - pos_, size);
    std::memcpy(data, data_.data() + pos_, n);
    return n;
}

int Event::write(const char* data, size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    if (pos_ + size > MAX_PAYLOAD_SIZE) {
        return error(SYSTEM_ERROR_TOO_LARGE);
    }
    if (pos_ + size > data_.size() && !data_.resize(pos_ + size)) {
        return error(SYSTEM_ERROR_NO_MEMORY);
    }
    std::memcpy(data_.data() + pos_, data, size);
    pos_ += size;
    return size;
}

int Event::seek(size_t pos) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    pos_ = (pos < data_.size()) ? pos : data_.size();
    return pos_;
}

int Event::tell() const {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    return pos_;
}

int Event::resize(size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    if (!data_.resize(size)) {
        return error(SYSTEM_ERROR_NO_MEMORY);
    }
    if (pos_ > data_.size()) {
        pos_ = data_.size();
    }
    return 0;
}

int Event::size() const {
    return data_.size();
}

int Event::prepareForPublish() {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    if (!*name_) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    status_ = CLOUD_EVENT_STATUS_SENDING;
    return 0;
}

void Event::publishComplete(int error) {
    if (error < 0) {
        status_ = CLOUD_EVENT_STATUS_FAILED;
        error_ = error;
    } else {
        status_ = CLOUD_EVENT_STATUS_SENT;
    }
}

} // namespace particle::system::cloud
