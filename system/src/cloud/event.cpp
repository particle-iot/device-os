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
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t n = CHECK(coap_read_payload(payload_.get(), data, size, nullptr /* reserved */));
    return n;
}

int Event::peek(char* data, size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t oldPos = CHECK(coap_get_payload_pos(payload_.get(), nullptr /* reserved */));
    size_t n = CHECK(coap_read_payload(payload_.get(), data, size, nullptr /* reserved */));
    CHECK(coap_set_payload_pos(payload_.get(), oldPos, COAP_SEEK_SET, nullptr /* reserved */));
    return n;
}

int Event::write(const char* data, size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t n = CHECK(coap_write_payload(payload_.get(), data, size, nullptr /* reserved */));
    return n;
}

int Event::seek(size_t pos) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t newPos = CHECK(coap_set_payload_pos(payload_.get(), pos, COAP_SEEK_SET, nullptr /* reserved */));
    return newPos;
}

int Event::tell() {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t pos = CHECK(coap_get_payload_pos(payload_.get(), nullptr /* reserved */));
    return pos;
}

int Event::resize(size_t size) {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    CHECK(coap_set_payload_size(payload_.get(), size, nullptr /* reserved */));
    return 0;
}

int Event::size() {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    CHECK(initPayload());
    size_t size = CHECK(coap_get_payload_size(payload_.get(), nullptr /* reserved */));
    return size;
}

int Event::prepareForPublish() {
    CHECK(checkStatus(CLOUD_EVENT_STATUS_NEW));
    if (!*name_) {
        return error(SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    CHECK(initPayload());
    CHECK(coap_set_payload_pos(payload_.get(), 0 /* pos */, COAP_SEEK_SET, nullptr /* reserved */));
    status_ = CLOUD_EVENT_STATUS_SENDING;
    return 0;
}

void Event::publishComplete(int error) {
    if (status_ != CLOUD_EVENT_STATUS_SENDING) {
        return;
    }
    if (error < 0) {
        status_ = CLOUD_EVENT_STATUS_FAILED;
        error_ = error;
    } else {
        status_ = CLOUD_EVENT_STATUS_SENT;
    }
}

int Event::initPayload() {
    if (payload_) {
        return 0;
    }
    coap_payload* apiPayload = nullptr;
    CHECK(coap_create_payload(&apiPayload, nullptr /* reserved */));
    payload_.reset(apiPayload);
    return 0;
}

} // namespace particle::system::cloud
