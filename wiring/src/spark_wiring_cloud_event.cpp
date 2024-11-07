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

#include "coap_util.h"

#include "c_string.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

struct CloudEvent::Data: public RefCount {
    CString name;
    CoapPayloadPtr payload;
    std::function<OnStatusChange> onStatusChange;
    ContentType contentType;
    Status status;
    size_t pos;
    int publishResult;
    int error;
    bool sendFailed; // TODO: Use a separate status?

    Data() :
            contentType(ContentType::TEXT),
            status(Status::NEW),
            pos(0),
            publishResult(0),
            error(0),
            sendFailed(false) {
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
        setError(Error::INVALID_ARGUMENT);
        return *this;
    }
    CString nameCopy(name, nameLen);
    if (!nameCopy) {
        setError(Error::NO_MEMORY);
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
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
    int r = coap_write_payload(payload, data, size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        setError(r);
        return *this;
    }
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        setError(r);
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
        setError(r);
        return *this;
    }
    size_t size = r;
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        setError(r);
        return *this;
    }
    return *this;
}

Buffer CloudEvent::data() const {
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

String CloudEvent::dataAsString() const {
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

Variant CloudEvent::dataAsVariant() {
    if (!d_ || !d_->payload) {
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

CloudEvent& CloudEvent::size(size_t size) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
    int r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        setError(r);
        return *this;
    }
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
    auto payload = getValidPayload();
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

int CloudEvent::publish() {
    auto status = this->status();
    if (status == Status::SENDING) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (status == Status::FAILED) {
        if (!d_ || !d_->sendFailed) {
            return error(); // Irrecoverable error
        }
        d_->sendFailed = false;
    }
    if (!d_->name) {
        return setError(SYSTEM_ERROR_INVALID_STATE);
    }
    setStatus(Status::SENDING);
    int r = publishImpl();
    if (r < 0) {
        d_->sendFailed = true;
        return setError(r);
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
    if (d_->contentType != ContentType::TEXT) {
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
    // The CoAP API now own the message
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
            setError(r);
            return nullptr;
        }
        d_->payload.reset(p);
    }
    return d_->payload.get();
}

bool CloudEvent::isWritable() const {
    return d_ && (d_->status == Status::NEW || d_->status == Status::SENT);
}

void CloudEvent::setStatus(Status status) {
    if (!d_ || d_->status == status) {
        return;
    }
    d_->status = status;
    if (d_->onStatusChange) {
        d_->onStatusChange(*this);
    }
}

int CloudEvent::setError(int error) {
    if (!d_) {
        return Error::NO_MEMORY;
    }
    if (d_->status != Status::FAILED) {
        d_->error = error;
        setStatus(Status::FAILED);
    }
    return d_->error;
}

// Called in the system thread
void CloudEvent::publishComplete(int err, void* arg) {
    auto d = RefCountPtr<Data>::wrap(static_cast<Data*>(arg));
    d->publishResult = err;
    // Run a callback in the application thread to update the status of the event
    int r = application_thread_invoke([](void* arg) {
        CloudEvent event(RefCountPtr<Data>::wrap(static_cast<Data*>(arg)));
        if (event.d_->publishResult < 0) {
            event.d_->sendFailed = true;
            event.setError(event.d_->publishResult);
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
