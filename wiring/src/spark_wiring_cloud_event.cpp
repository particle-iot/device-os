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
#include <cstdio>

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

namespace {

const size_t DEFAULT_MAX_PAYLOAD_HEAP_SIZE = COAP_BLOCK_SIZE;

int getUriPath(coap_message* msg, char* path, size_t size) {
    auto p = path;
    auto end = path + size;
    coap_option* opt = nullptr;
    CHECK(coap_get_option(msg, &opt, COAP_OPTION_URI_PATH, nullptr /* reserved */));
    if (opt) {
        int optNum = 0;
        do {
            if (p < end) {
                *p = '/';
            }
            ++p;
            p += CHECK(coap_get_string_option_value(opt, p, (p < end) ? end - p : 0, nullptr /* reserved */));
            CHECK(coap_get_next_option(msg, &opt, &optNum, nullptr /* reserved */));
        } while (opt && optNum == COAP_OPTION_URI_PATH);
    } else {
        if (p < end) {
            *p = '/';
        }
        ++p;
    }
    if (p < end) {
        *p = '\0';
    } else if (end != path) {
        *(end - 1) = '\0';
    }
    return p - path;
}

} // namespace

struct CloudEvent::Data: public RefCount {
    CString name;
    CoapPayloadPtr payload;
    std::function<OnStatusChange> onStatusChange;
    Status status;
    size_t maxHeapSize;
    size_t pos;
    int contentType;
    int requestId;
    int sendResult;
    int error;

    Data() :
            status(Status::NEW),
            maxHeapSize(DEFAULT_MAX_PAYLOAD_HEAP_SIZE),
            pos(0),
            contentType((int)ContentType::TEXT),
            requestId(COAP_INVALID_REQUEST_ID),
            sendResult(0),
            error(0) {
    }
};

struct CloudEvent::Subscription {
    std::function<OnEventReceived> callback;
    CString prefix;
    size_t prefixLen;

    Subscription() :
            prefixLen(0) {
    }
};

Vector<CloudEvent::Subscription> CloudEvent::s_subscriptions;

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
            LOG(ERROR, "coap_write_payload() failed: %d", r);
            setInvalid(r);
        }
        return *this;
    }
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_set_payload_size() failed: %d", r);
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
            LOG(ERROR, "encodeToCBOR() failed: %d", r);
            setInvalid(r);
        }
        return *this;
    }
    size_t size = r;
    r = coap_set_payload_size(payload, size, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_set_payload_size() failed: %d", r);
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
        LOG(ERROR, "coap_get_payload_size() failed: %d", r);
        return Buffer();
    }
    size_t size = r;
    Buffer buf;
    if (!buf.resize(size)) {
        return Buffer();
    }
    r = coap_read_payload(d_->payload.get(), buf.data(), size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_read_payload() failed: %d", r);
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
        LOG(ERROR, "coap_get_payload_size() failed: %d", r);
        return String();
    }
    size_t size = r;
    String str;
    if (!str.resize(size)) {
        return String();
    }
    r = coap_read_payload(d_->payload.get(), &str.operator[](0), size, 0 /* pos */, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_read_payload() failed: %d", r);
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
        LOG(ERROR, "decodeFromCBOR() failed: %d", r);
        return Variant();
    }
    return v;
}

CloudEvent& CloudEvent::maxHeapSize(size_t size) {
    if (!isWritable() || d_->payload) {
        return *this;
    }
    d_->maxHeapSize = std::min(size, MAX_DATA_SIZE);
    return *this;
}

size_t CloudEvent::maxHeapSize() const {
    if (!d_) {
        return DEFAULT_MAX_PAYLOAD_HEAP_SIZE;
    }
    return d_->maxHeapSize;
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

void CloudEvent::cancel() {
    if (!d_ || d_->status != Status::SENDING) {
        return;
    }
    coap_cancel_request(d_->requestId, nullptr /* reserved */);
    d_->requestId = COAP_INVALID_REQUEST_ID;
    setFailed(Error::CANCELLED);
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
        LOG(ERROR, "coap_get_payload_size() failed: %d", r);
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
    if (r < 0 && r != Error::END_OF_STREAM) {
        LOG(ERROR, "coap_read_payload() failed: %d", r);
    }
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
        LOG(ERROR, "coap_write_payload() failed: %d", r);
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
        LOG(ERROR, "coap_set_payload_size() failed: %d", r);
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
        LOG(ERROR, "coap_get_payload_size() failed: %d", r);
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
            LOG(ERROR, "coap_get_payload_size() failed: %d", r);
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

CloudEvent& CloudEvent::operator=(CloudEvent event) {
    swap(*this, event);
    return *this;
}

int CloudEvent::publish() {
    auto status = this->status();
    if (status == Status::INVALID) {
        return error();
    }
    if (status == Status::SENDING) {
        LOG(ERROR, "Event is being sent already");
        return Error::INVALID_STATE;
    }
    if (!d_->name) {
        LOG(ERROR, "Event name is missing");
        return setFailed(Error::INVALID_STATE);
    }
    setStatus(Status::SENDING);
    int r = send();
    if (r < 0) {
        LOG(ERROR, "Failed to send event: %d", r);
        return setFailed(r);
    }
    return 0;
}

int CloudEvent::subscribe(const char* prefix, std::function<OnEventReceived> callback) {
    Subscription sub;
    sub.callback = std::move(callback);
    sub.prefixLen = std::strlen(prefix);
    sub.prefix = CString(prefix, sub.prefixLen);
    if (!sub.prefix || !s_subscriptions.append(std::move(sub))) {
        return Error::NO_MEMORY;
    }
    NAMED_SCOPE_GUARD(removeSubGuard, { // Removes the newly added subscription on an error
        s_subscriptions.takeLast();
    });
    if (s_subscriptions.size() == 1) {
        // Register a handler for event requests
        int r = coap_add_request_handler("/E", COAP_METHOD_POST, COAP_MESSAGE_FULL, receiveRequestSystem, nullptr /* arg */,
                nullptr /* reserved */);
        if (r < 0) {
            LOG(ERROR, "coap_add_request_handler() failed: %d", r);
            return r;
        }
    }
    removeSubGuard.dismiss();
    return 0;
}

void CloudEvent::unsubscribeAll() {
    if (s_subscriptions.isEmpty()) {
        return;
    }
    coap_remove_request_handler("/E", COAP_METHOD_POST, nullptr /* reserved */);
    s_subscriptions.clear();
}

int CloudEvent::send() {
    char uriPath[COAP_MAX_URI_PATH_LENGTH];
    int r = std::snprintf(uriPath, sizeof(uriPath), "E/%s", (const char*)d_->name);
    if (r < 0 || (size_t)r >= sizeof(uriPath)) {
        return Error::INTERNAL;
    }
    CoapMessagePtr msg;
    auto reqId = CHECK(coap_begin_request(&msg, uriPath, COAP_METHOD_POST, 0 /* timeout */, 0 /* flags */, nullptr /* reserved */));
    if (d_->payload) {
        CHECK(coap_set_payload(msg.get(), d_->payload.get(), nullptr /* reserved */));
    }
    if (d_->contentType != (int)ContentType::TEXT) {
        CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_CONTENT_FORMAT, (unsigned)d_->contentType, nullptr /* reserved */));
    }
    CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_NO_RESPONSE, 26, nullptr /* reserved */)); // RFC 7967, 2.1
    CHECK(coap_end_request(msg.get(), nullptr /* resp_cb */,
            [](int reqId, void* arg) { // ack_cb
                sendComplete(0 /* err */, arg);
                return 0;
            },
            [](int err, int reqId, void* arg) { // error_cb
                sendComplete(err, arg);
            }, d_.get(), nullptr /* reserved */));
    // The system now owns the message
    msg.release();
    // Keep the reference around until either the ACK or error callback is called
    d_->addRef();
    d_->requestId = reqId;
    return 0;
}

coap_payload* CloudEvent::getValidPayload() {
    if (!d_) {
        return nullptr;
    }
    if (!d_->payload) {
        CoapPayloadPtr p;
        int r = coap_create_payload(&p, d_->maxHeapSize, nullptr /* reserved */);
        if (r < 0) {
            LOG(ERROR, "coap_create_payload() failed: %d", r);
            setInvalid(r);
            return nullptr;
        }
        d_->payload = std::move(p);
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

int CloudEvent::receiveRequestApp(CoapMessagePtr msg) {
    // Get the event name
    char path[COAP_MAX_URI_PATH_LENGTH + 1];
    size_t pathLen = CHECK(getUriPath(msg.get(), path, sizeof(path)));
    if (pathLen <= 3 /* strlen("/E/") */ || pathLen > COAP_MAX_URI_PATH_LENGTH) {
        return Error::BAD_DATA;
    }
    auto name = path + 3;
    auto nameLen = pathLen - 3;

    // Get the content format option
    unsigned contentType = COAP_FORMAT_TEXT_PLAIN;
    coap_option* opt = nullptr;
    CHECK(coap_get_option(msg.get(), &opt, COAP_OPTION_CONTENT_FORMAT, nullptr /* reserved */));
    if (opt) {
        CHECK(coap_get_uint_option_value(opt, &contentType, nullptr /* reserved */));
    }

    // Invoke the subscription handlers
    for (auto& sub: s_subscriptions) {
        if (sub.prefixLen > nameLen || std::memcmp((const char*)sub.prefix, name, sub.prefixLen) != 0) {
            continue;
        }

        // Create a separate event instance for each of the matching subscription handlers
        auto d = makeRefCountPtr<Data>();
        if (!d) {
            return Error::NO_MEMORY;
        }
        d->name = CString(name, nameLen);
        if (!d->name) {
            return Error::NO_MEMORY;
        }
        d->contentType = contentType;

        // Payload objects are reference counted. If there are multiple matching subscription handlers,
        // all created event instances will reference the same payload object
        CHECK(coap_get_payload(msg.get(), &d->payload, nullptr /* reserved */));

        CloudEvent ev(std::move(d));
        sub.callback(std::move(ev));
    }
    return 0;
}

// Called in the system thread
int CloudEvent::receiveRequestSystem(coap_message* apiMsg, const char* path, int method, int reqId, void* arg) {
    CoapMessagePtr msg(apiMsg);
    // Run the subscription handlers in the application thread
    int r = application_thread_invoke([](void* arg) {
        CoapMessagePtr msg(static_cast<coap_message*>(arg));
        int r = receiveRequestApp(std::move(msg));
        if (r < 0) {
            LOG(ERROR, "Failed to handle received event: %d", r);
        }
    }, msg.get(), nullptr /* reserved */);
    // FIXME: application_thread_invoke() doesn't really handle errors as of now
    if (r == 0) {
        // Keep the reference around until the application callback is called
        msg.release();
    }
    return 0;
}

// Called in the system thread
void CloudEvent::sendComplete(int err, void* arg) {
    auto d = RefCountPtr<Data>::wrap(static_cast<Data*>(arg));
    d->sendResult = err;
    // Run a callback in the application thread to update the status of the event
    int r = application_thread_invoke([](void* arg) {
        CloudEvent event(RefCountPtr<Data>::wrap(static_cast<Data*>(arg)));
        if (event.d_->status != Status::SENDING) {
            return; // The event was cancelled
        }
        event.d_->requestId = COAP_INVALID_REQUEST_ID;
        if (event.d_->sendResult < 0) {
            LOG(ERROR, "Failed to send event: %d", event.d_->sendResult);
            event.setFailed(event.d_->sendResult);
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
