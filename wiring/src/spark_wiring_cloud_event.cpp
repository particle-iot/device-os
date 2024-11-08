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

#include "system_cloud.h" // For MAX_EVENT_NAME_LENGTH
#include "system_task.h"

#include "coap_defs.h"
#include "coap_util.h"

#include "c_string.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

struct CloudEvent::Data: public RefCount {
    CString name;
    CoapPayloadPtr payload;
    std::function<OnStatusChange> onStatusChange;
    Status status;
    size_t pos;
    int contentType;
    int publishResult;
    int error;

    Data() :
            status(Status::NEW),
            pos(0),
            contentType((int)ContentType::TEXT),
            publishResult(0),
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

CloudEvent::CloudEvent(RefCountPtr<Data> data) :
        d_(std::move(data)) {
}

CloudEvent::~CloudEvent() {
}

CloudEvent& CloudEvent::name(const char* name) {
    if (!isWritable()) {
        return *this;
    }
    size_t nameLen = std::strlen(name);
    if (!nameLen || nameLen > protocol::MAX_EVENT_NAME_LENGTH) {
        LOG(ERROR, "Invalid event name length");
        setFailed(Error::INVALID_ARGUMENT);
        return *this;
    }
    CString nameCopy(name, nameLen);
    if (!nameCopy) {
        setInvalid(Error::NO_MEMORY);
        return *this;
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
    d_->contentType = (int)type;
    return *this;
}

ContentType CloudEvent::contentType() const {
    if (!d_) {
        return ContentType::TEXT;
    }
    return (ContentType)d_->contentType;
}

CloudEvent& CloudEvent::data(const char* data, size_t size) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
    int r = coap_write_payload(payload, data, size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        if (r == Error::COAP_TOO_LARGE_PAYLOAD) {
            LOG(ERROR, "Event data is too large");
            setFailed(r);
        } else {
            setInvalid(r);
        }
        return *this;
    }
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        setInvalid(r);
        return *this;
    }
    d_->pos = size;
    return *this;
}

CloudEvent& CloudEvent::data(const Variant& data) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
    // TODO: Don't use this stream object directly
    d_->pos = 0;
    int r = encodeToCBOR(data, *this);
    if (r < 0) {
        if (r == Error::COAP_TOO_LARGE_PAYLOAD) {
            LOG(ERROR, "Event data is too large");
            setFailed(r);
        } else {
            setInvalid(r);
        }
        return *this;
    }
    size_t size = r;
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        setInvalid(r);
        return *this;
    }
    d_->contentType = (int)protocol::CoapContentFormat::PARTICLE_JSON_AS_CBOR;
    return *this;
}

Buffer CloudEvent::data() const {
    if (!isReadable() || !d_->payload) {
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

String CloudEvent::dataAsString() const {
    if (!isReadable() || !d_->payload) {
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

Variant CloudEvent::dataAsVariant() {
    if (!isReadable() || !d_->payload) {
        return Variant();
    }
    // TODO: Don't use this stream object directly
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

CloudEvent& CloudEvent::onStatusChange(std::function<OnStatusChange> callback) {
    if (!isWritable()) {
        return *this;
    }
    d_->onStatusChange = std::move(callback);
    return *this;
}

CloudEvent::Status CloudEvent::status() const {
    if (!d_) {
        return Status::INVALID;
    }
    return d_->status;
}

int CloudEvent::error() const {
    if (!d_) {
        return Error::NO_MEMORY;
    }
    return d_->error;
}

void CloudEvent::reset() {
    d_ = makeRefCountPtr<Data>();
}

int CloudEvent::available() {
    if (!isReadable() || !d_->payload) {
        return 0;
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r - d_->pos;
}

int CloudEvent::read(char* data, size_t size) {
    int r = peek(data, size);
    if (r < 0) {
        return r;
    }
    d_->pos += r;
    return r;
}

int CloudEvent::peek(char* data, size_t size) {
    if (!isReadable()) {
        return error();
    }
    if (!d_->payload) {
        if (size > 0) {
            return Error::END_OF_STREAM;
        }
        return 0;
    }
    int r = coap_read_payload(d_->payload.get(), data, size, d_->pos, nullptr /* reserved */);
    return r;
}

int CloudEvent::write(const char* data, size_t size) {
    if (!isWritable()) {
        int err = error();
        if (err < 0) {
            return err;
        }
        return Error::INVALID_STATE;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return error();
    }
    int r = coap_write_payload(payload, data, size, d_->pos, nullptr /* reserved */);
    if (r < 0) {
        if (r == Error::COAP_TOO_LARGE_PAYLOAD) {
            LOG(ERROR, "Event data is too large");
            return setFailed(r);
        }
        return setInvalid(r);
    }
    d_->pos += r;
    return r;
}

int CloudEvent::size(size_t size) {
    if (!isWritable()) {
        int err = error();
        if (err < 0) {
            return err;
        }
        return Error::INVALID_STATE;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return error();
    }
    int r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        if (r == Error::COAP_TOO_LARGE_PAYLOAD) {
            LOG(ERROR, "Event data is too large");
            return setFailed(r);
        }
        return setInvalid(r);
    }
    if (d_->pos > size) {
        d_->pos = size;
    }
    return 0;
}

size_t CloudEvent::size() const {
    if (!isReadable() || !d_->payload) {
        return 0;
    }
    int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
    if (r < 0) {
        return 0;
    }
    return r;
}

int CloudEvent::pos(size_t pos) {
    // The current position is used both when reading and writing so using the least restrictive
    // status check here
    if (!isReadable()) {
        return error();
    }
    size_t size = 0;
    if (d_->payload) {
        int r = coap_get_payload_size(d_->payload.get(), nullptr /* reserved */);
        if (r < 0) {
            return r;
        }
        size = r;
    }
    if (pos > size) {
        pos = size;
    }
    d_->pos = pos;
    return pos;
}

size_t CloudEvent::pos() const {
    if (!isReadable()) {
        return 0;
    }
    return d_->pos;
}

int CloudEvent::publish() {
    auto status = this->status();
    if (status == Status::INVALID) {
        return error();
    }
    if (status == Status::SENDING) {
        return Error::INVALID_STATE;
    }
    if (!d_->name) {
        LOG(ERROR, "Event name is missing");
        return setFailed(Error::INVALID_STATE);
    }
    setStatus(Status::SENDING);
    int r = publishImpl();
    if (r < 0) {
        LOG(ERROR, "Failed to publish event: %d", r);
        return setFailed(r);
    }
    return 0;
}

CloudEvent& CloudEvent::operator=(CloudEvent event) {
    swap(*this, event);
    return *this;
}

int CloudEvent::publishImpl() {
    char uriPath[COAP_MAX_URI_PATH_LENGTH];
    int r = std::snprintf(uriPath, sizeof(uriPath), "E/%s", (const char*)d_->name);
    if (r < 0 || (size_t)r >= sizeof(uriPath)) {
        return Error::INTERNAL;
    }
    coap_message* apiMsg = nullptr;
    CHECK(coap_begin_request(&apiMsg, uriPath, COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    CoapMessagePtr msg(apiMsg);
    if (d_->payload) {
        CHECK(coap_set_payload(msg.get(), d_->payload.get(), nullptr /* reserved */));
    }
    if (d_->contentType != (int)ContentType::TEXT) {
        CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_CONTENT_FORMAT, (unsigned)d_->contentType, nullptr /* reserved */));
    }
    CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_NO_RESPONSE, 26, nullptr /* reserved */)); // RFC 7967, 2.1
    CHECK(coap_end_request(msg.get(), nullptr /* resp_cb */,
            [](int reqId, void* arg) { // ack_cb
                publishComplete(0 /* err */, arg);
                return 0;
            },
            [](int err, int reqId, void* arg) { // error_cb
                publishComplete(err, arg);
            }, d_.get(), nullptr /* reserved */));
    // The system now owns the message
    msg.release();
    // Keep the reference around until either the ACK or error callback is called
    d_->addRef();
    return 0;
}

coap_payload* CloudEvent::getValidPayload() {
    if (!d_) {
        return nullptr;
    }
    if (!d_->payload) {
        coap_payload* p = nullptr;
        int r = coap_create_payload(&p, nullptr /* reserved */);
        if (r < 0) {
            setInvalid(r);
            return nullptr;
        }
        d_->payload.reset(p);
    }
    return d_->payload.get();
}

void CloudEvent::setStatus(Status status, int err) {
    if (!d_ || d_->status == status) {
        return;
    }
    d_->error = err;
    d_->status = status;
    if (d_->onStatusChange) {
        d_->onStatusChange(*this);
    }
    if (status == Status::INVALID) {
        d_->payload.reset();
        d_->onStatusChange = nullptr;
    }
}

// Called in the system thread
void CloudEvent::publishComplete(int err, void* arg) {
    auto d = RefCountPtr<Data>::wrap(static_cast<Data*>(arg));
    d->publishResult = err;
    // Run a callback in the application thread to update the status of the event
    int r = application_thread_invoke([](void* arg) {
        CloudEvent event(RefCountPtr<Data>::wrap(static_cast<Data*>(arg)));
        if (event.d_->publishResult < 0) {
            event.setFailed(event.d_->publishResult);
        } else {
            event.setStatus(Status::SENT);
        }
    }, d.get(), nullptr /* reserved */);
    // FIXME: application_thread_invoke() doesn't really handle errors as of now
    if (r == 0) {
        // Keep the reference around until the application callback is called
        d.unwrap();
    }
}

} // namespace particle
