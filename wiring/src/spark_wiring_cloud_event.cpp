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

#include <cstring>

#include "spark_wiring_cloud_event.h"
#include "spark_wiring_variant.h"
#include "spark_wiring_error.h"

#include "coap_api.h"
#include "coap_util.h"

#include "c_string.h"
#include "scope_guard.h"

#define CHECK_AND_SET_ERROR(_expr) \
        ({ \
            const auto _r = _expr; \
            if (_r < 0) { \
                return this->setError(_r); \
            } \
            _r; \
        })

namespace particle {

struct CloudEvent::Data: public RefCount {
    CString name;
    CoapPayloadPtr payload;
    std::function<OnStatusChange> onStatusChange;
    ContentType contentType;
    Status status;
    size_t pos;
    int error;

    Data() :
            contentType(ContentType::TEXT),
            status(Status::NEW),
            pos(0),
            error(0) {
    }
};

CloudEvent::CloudEvent() :
        d_(makeRefCountPtr<Data>()) {
}

CloudEvent::CloudEvent(const CloudEvent& event) :
        d_(event.d_) {
}

CloudEvent::CloudEvent(CloudEvent&& event) {
    swap(*this, event);
}

CloudEvent::~CloudEvent() {
}

CloudEvent& CloudEvent::name(const char* name) {
    if (!isWritable()) {
        return *this;
    }
    CString nameCopy(name);
    if (!nameCopy) {
        return setError(Error::NO_MEMORY);
    }
    d_->name = std::move(nameCopy);
    return *this;
}

const char* CloudEvent::name() const {
    if (!d_ || !d_->name) {
        return "";
    }
    return d_->name;
}

CloudEvent& CloudEvent::contentType(ContentType type) {
    if (!isWritable()) {
        return *this;
    }
    d_->contentType = type;    
    return *this;
}

ContentType CloudEvent::contentType() const {
    if (!d_) {
        return ContentType::TEXT;
    }
    return d_->contentType;
}

CloudEvent& CloudEvent::data(const char* data, size_t size) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getMutablePayload();
    if (!payload) {
        return *this;
    }
    CHECK_AND_SET_ERROR(coap_write_payload(payload, data, size, 0 /* pos */, nullptr /* reserved */));
    CHECK_AND_SET_ERROR(coap_set_payload_size(payload, size, nullptr /* reserved */));
    d_->pos = size;
    return *this;
}

CloudEvent& CloudEvent::data(const Variant& data) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getMutablePayload();
    if (!payload) {
        return *this;
    }
    d_->pos = 0;
    size_t size = CHECK_AND_SET_ERROR(encodeToCBOR(data, *this));
    CHECK_AND_SET_ERROR(coap_set_payload_size(payload, size, nullptr /* reserved */));
    return *this;
}

String CloudEvent::data() const {
    if (!d_ || !d_->payload) {
        return String();
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return String();
    }
    size_t size = r;
    String str;
    if (!str.resize(size)) {
        return String();
    }
    r = coap_read_payload(d_->payload.get(), &str.operator[](0), size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        return String();
    }
    return str;
}

Buffer CloudEvent::dataAsBuffer() const {
    if (!d_ || !d_->payload) {
        return Buffer();
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return Buffer();
    }
    size_t size = r;
    Buffer buf;
    if (!buf.resize(size)) {
        return Buffer();
    }
    r = coap_read_payload(d_->payload.get(), buf.data(), size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        return Buffer();
    }
    return buf;
}

Variant CloudEvent::dataAsVariant() {
    if (!d_ || !d_->payload) {
        return Variant();
    }
    auto origPos = d_->pos;
    d_->pos = 0;
    SCOPE_GUARD({
        d_->pos = origPos;
    });
    Variant v;
    int r = decodeFromCBOR(v, *this);
    if (r < 0) {
        return Variant();
    }
    return v;
}

CloudEvent& CloudEvent::size(size_t size) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getMutablePayload();
    if (!payload) {
        return *this;
    }
    CHECK_AND_SET_ERROR(coap_set_payload_size(payload, size, nullptr /* reserved */));
    if (d_->pos > size) {
        d_->pos = size;
    }
    return *this;
}

size_t CloudEvent::size() const {
    if (!d_ || !d_->payload) {
        return 0;
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r;
}

CloudEvent& CloudEvent::onStatusChange(std::function<OnStatusChange> callback) {
    if (!isWritable()) {
        return *this;
    }
    d_->onStatusChange = std::move(callback);
    return *this;
}

CloudEvent::Status CloudEvent::status() const {
    if (!d_) {
        return Status::FAILED;
    }
    return d_->status;
}

int CloudEvent::read() {
    char c;
    size_t n = readBytes(&c, 1);
    if (n != 1) {
        return -1;
    }
    return (unsigned char)c;
}

size_t CloudEvent::readBytes(char* data, size_t size) {
    if (!d_ || !d_->payload) {
        return 0;
    }
    int r = coap_read_payload(d_->payload.get(), data, size, d_->pos, nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    d_->pos += r;
    return r;
}

int CloudEvent::peek() {
    if (!d_ || !d_->payload) {
        return 0;
    }
    char c;
    int r = coap_read_payload(d_->payload.get(), &c, 1, d_->pos, nullptr /* reserved */);
    if (r < 0) {
        return -1;
    }
    return (unsigned char)c;
}

int CloudEvent::available() {
    if (!d_ || !d_->payload) {
        return 0;
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r - d_->pos;
}

size_t CloudEvent::write(const uint8_t* data, size_t size) {
    if (!isWritable()) {
        return 0;
    }
    auto payload = getMutablePayload();
    if (!payload) {
        return 0;
    }
    int r = coap_write_payload(payload, (const char*)data, size, d_->pos, nullptr /* reserved */);
    if (r < 0) {
        setError(r);
        return 0;
    }
    d_->pos += r;
    return r;
}

CloudEvent& CloudEvent::pos(size_t pos) {
    if (!isWritable()) {
        return *this;
    }
    d_->pos = pos;
    return *this;
}

size_t CloudEvent::pos() const {
    if (!d_) {
        return 0;
    }
    return d_->pos;
}

int CloudEvent::error() const {
    if (!d_) {
        return Error::NO_MEMORY;
    }
    return d_->error;
}

void CloudEvent::clearError() {
    if (!d_ || d_->status != Status::FAILED) {
        return;
    }
    d_->error = 0;
    d_->status = Status::NEW;
}

CloudEvent& CloudEvent::operator=(CloudEvent event) {
    swap(*this, event);
    return *this;
}

coap_payload* CloudEvent::getMutablePayload() {
    if (!d_) {
        return nullptr;
    }
    if (!d_->payload) {
        coap_payload* p = nullptr;
        int r = coap_create_payload(&p, nullptr /* reserved */);
        if (r < 0) {
            setError(r);
            return nullptr;
        }
        d_->payload.reset(p);
    }
    return d_->payload.get();
}

CloudEvent& CloudEvent::setError(int error) {
    if (d_ && d_->status != Status::FAILED) {
        d_->error = error;
        d_->status = Status::FAILED;
    }
    return *this;
}

bool CloudEvent::isWritable() const {
    if (!d_ || d_->status != Status::NEW) {
        return false;
    }
    return true;
}

} // namespace particle
