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
#include <mutex>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

#include "spark_wiring_cloud_event.h"
#include "spark_wiring_cloud.h"
#include "spark_wiring_error.h"

#include "system_cloud.h" // For MAX_EVENT_NAME_LENGTH
#include "system_task.h"
#include "concurrent_hal.h"

#include "coap_defs.h"
#include "coap_util.h"

#include "c_string.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

namespace {

const size_t DEFAULT_MAX_PAYLOAD_HEAP_SIZE = COAP_BLOCK_SIZE;

const size_t MAX_EVENT_DATA_IN_FLIGHT = 32 * 1024;

class EventLock {
public:
    ~EventLock() {
        if (mutex_) {
            os_mutex_recursive_destroy(mutex_);
        }
    }

    void lock() {
        if (mutex_) {
            os_mutex_recursive_lock(mutex_);
        }
    }

    void unlock() {
        if (mutex_) {
            os_mutex_recursive_unlock(mutex_);
        }
    }

    static EventLock& instance() {
        static EventLock lock;
        return lock;
    }

private:
    os_mutex_recursive_t mutex_;

    EventLock() :
            mutex_(nullptr) {
        os_mutex_recursive_create(&mutex_);
    }
};

class RateLimiter {
public:
    bool take(size_t size) {
        std::lock_guard lock(EventLock::instance());
        size = sizeInFullBlocks(size);
        if (dataInFlight_ + size > MAX_EVENT_DATA_IN_FLIGHT) {
            return false;
        }
        dataInFlight_ += size;
        return true;
    }

    bool canTake(size_t size) {
        std::lock_guard lock(EventLock::instance());
        return dataInFlight_ + sizeInFullBlocks(size) <= MAX_EVENT_DATA_IN_FLIGHT;
    }

    void give(size_t size) {
        std::lock_guard lock(EventLock::instance());
        dataInFlight_ -= sizeInFullBlocks(size);
        if (dataInFlight_ < 0) {
            dataInFlight_ = 0;
        }
    }

    static RateLimiter& instance() {
        static RateLimiter limiter;
        return limiter;
    }

private:
    int dataInFlight_;

    RateLimiter() :
            dataInFlight_(0) {
    }

    static size_t sizeInFullBlocks(size_t size) {
        return std::max<size_t>(((size + COAP_BLOCK_SIZE - 1) / COAP_BLOCK_SIZE) * COAP_BLOCK_SIZE, COAP_BLOCK_SIZE);
    }
};

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
    std::atomic<Status> status;
    ContentType contentType;
    size_t maxHeapSize;
    size_t pos;
    int requestId;
    int sendResult;
    int error;

    Data() :
            status(Status::NEW),
            contentType(ContentType::TEXT),
            maxHeapSize(DEFAULT_MAX_PAYLOAD_HEAP_SIZE),
            pos(0),
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

CloudEvent& CloudEvent::data(const EventData& data) {
    if (!isWritable()) {
        return *this;
    }
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
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
    r = coap_set_payload_size(payload, d_->pos, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_set_payload_size() failed: %d", r);
        setInvalid(r);
        return *this;
    }
    d_->contentType = ContentType::STRUCTURED;
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

String CloudEvent::dataString() const {
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

EventData CloudEvent::dataStructured() const {
    if (!isReadable() || !d_->payload) {
        return EventData();
    }
    // Use a separate stream object so that this method remains constant
    CoapPayloadInputStream stream(d_->payload.get());
    EventData d;
    int r = decodeFromCBOR(d, stream);
    if (r < 0) {
        LOG(ERROR, "decodeFromCBOR() failed: %d", r);
        return EventData();
    }
    return d;
}

CloudEvent& CloudEvent::loadData(const char* path) {
    if (!isWritable()) {
        return *this;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG(ERROR, "open() failed: %d", errno);
        setFailed(Error::FILE);
        return *this;
    }
    NAMED_SCOPE_GUARD(closeFileGuard, {
        int r = close(fd);
        if (r < 0) {
            LOG(ERROR, "close() failed: %d", errno);
        }
    });
    auto payload = getValidPayload();
    if (!payload) {
        return *this;
    }
    char buf[128];
    size_t offs = 0;
    for (;;) {
        int r = ::read(fd, buf, sizeof(buf));
        if (r < 0) {
            LOG(ERROR, "read() failed: %d", errno);
            setFailed(Error::FILE);
            return *this;
        }
        if (r == 0) {
            break; // EOF
        }
        size_t n = r;
        r = coap_write_payload(payload, buf, n, offs, nullptr /* reserved */);
        if (r < 0) {
            if (r == Error::COAP_TOO_LARGE_PAYLOAD) {
                LOG(ERROR, "Event data is too large");
                setFailed(r);
                return *this;
            }
            LOG(ERROR, "coap_write_payload() failed: %d", r);
            setInvalid(r);
            return *this;
        }
        offs += n;
    }
    closeFileGuard.dismiss();
    int r = close(fd);
    if (r < 0) {
        LOG(ERROR, "close() failed: %d", errno);
        setFailed(Error::FILE);
        return *this;
    }
    r = coap_set_payload_size(payload, offs, nullptr /* reserved */);
    if (r < 0) {
        LOG(ERROR, "coap_set_payload_size() failed: %d", r);
        setInvalid(r);
        return *this;
    }
    d_->pos = offs;
    return *this;
}

int CloudEvent::saveData(const char* path) {
    if (!isReadable()) {
        return error();
    }
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
    if (fd < 0) {
        LOG(ERROR, "open() failed: %d", errno);
        return Error::FILE;
    }
    NAMED_SCOPE_GUARD(closeFileGuard, {
        int r = close(fd);
        if (r < 0) {
            LOG(ERROR, "close() failed: %d", errno);
        }
    });
    if (d_->payload) {
        char buf[128];
        size_t offs = 0;
        for (;;) {
            int r = coap_read_payload(d_->payload.get(), buf, sizeof(buf), offs, nullptr /* reserved */);
            if (r < 0) {
                if (r == Error::END_OF_STREAM) {
                    break;
                }
                LOG(ERROR, "coap_read_payload() failed: %d", r);
                return r;
            }
            size_t n = r;
            r = ::write(fd, buf, n);
            if (r < 0) {
                LOG(ERROR, "read() failed: %d", errno);
                return Error::FILE;
            }
            offs += n;
        }
    }
    closeFileGuard.dismiss();
    int r = close(fd);
    if (r < 0) {
        LOG(ERROR, "close() failed: %d", errno);
        return Error::FILE;
    }
    return 0;
}

int CloudEvent::setSize(size_t size) {
    if (!isWritable()) {
        int err = error();
        if (err < 0) {
            return err;
        }
        if (status() == Status::SENDING) {
            return Error::BUSY;
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

int CloudEvent::seek(size_t pos) {
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

CloudEvent& CloudEvent::maxDataInRam(size_t size) {
    if (!isWritable() || d_->payload) {
        return *this;
    }
    d_->maxHeapSize = std::min(size, MAX_SIZE);
    return *this;
}

size_t CloudEvent::maxDataInRam() const {
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

void CloudEvent::resetStatus() {
    if (!d_ || (d_->status != Status::SENT && d_->status != Status::FAILED)) {
        return;
    }
    d_->error = 0;
    // Update the status without invoking the status change callback
    d_->status = Status::NEW;
}

void CloudEvent::cancel() {
    if (!d_ || d_->status != Status::SENDING) {
        return;
    }
    size_t size = this->size();
    int r = coap_cancel_request(d_->requestId, nullptr /* reserved */);
    if (r == COAP_RESULT_CANCELLED) {
        // The request was found and cancelled. Release the reference added in send() as the
        // completion callback will no longer be called for this request
        d_->release();
    }
    // TODO: For now, transition to an invalid state as an event in a failed state can be sent again
    // and that would create a race condition between cancellation and normal completion of the event
    // in sendComplete()
    if (testAndSetStatus(Status::SENDING, Status::INVALID, Error::CANCELLED)) {
        // The completion callback has either not been scheduled or not yet called in the app thread
        RateLimiter::instance().give(size);
    }
}

void CloudEvent::clear() {
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
        if (status() == Status::SENDING) {
            return Error::BUSY;
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

CloudEvent& CloudEvent::operator=(CloudEvent event) {
    swap(*this, event);
    return *this;
}

bool CloudEvent::canPublish(size_t size) {
    return RateLimiter::instance().canTake(size);
}

int CloudEvent::publish() {
    auto status = this->status();
    if (status == Status::INVALID) {
        return error();
    }
    if (status == Status::SENDING) {
        LOG(ERROR, "Event is being sent already");
        return Error::BUSY;
    }
    if (!d_->name) {
        LOG(ERROR, "Event name is missing");
        return setFailed(Error::INVALID_ARGUMENT);
    }
    size_t size = this->size();
    if (!RateLimiter::instance().take(size)) {
        LOG(ERROR, "Limit for event data in flight is reached");
        return setFailed(Error::LIMIT_EXCEEDED);
    }
    NAMED_SCOPE_GUARD(rateLimiterGuard, {
        RateLimiter::instance().give(size);
    });
    int r = send();
    if (r < 0) {
        LOG(ERROR, "Failed to send event: %d", r);
        return setFailed(r);
    }
    setStatus(Status::SENDING);
    rateLimiterGuard.dismiss();
    return 0;
}

int CloudEvent::subscribe(const char* prefix, std::function<OnEventReceived> callback, const SubscribeOptions& /* opts */) {
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
    if (d_->contentType != ContentType::TEXT) {
        CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_CONTENT_FORMAT, (unsigned)d_->contentType, nullptr /* reserved */));
    }
    CHECK(coap_add_uint_option(msg.get(), COAP_OPTION_NO_RESPONSE, 26, nullptr /* reserved */)); // RFC 7967, 2.1
    CHECK(coap_end_request(msg.get(), nullptr /* resp_cb */,
            [](int reqId, void* arg) { // ack_cb
                sendComplete(0 /* err */, reqId, arg);
                return 0;
            },
            sendComplete /* error_cb */, d_.get(), nullptr /* reserved */));
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
    if (!d_) {
        return;
    }
    testAndSetStatus(d_->status, status, err);
}

bool CloudEvent::testAndSetStatus(Status expectedStatus, Status newStatus, int err) {
    if (!d_) {
        return false;
    }
    bool changed = d_->status.compare_exchange_strong(expectedStatus, newStatus);
    if (!changed || newStatus == expectedStatus) {
        return false;
    }
    d_->error = err;
    if (d_->onStatusChange) {
        d_->onStatusChange(*this);
    }
    if (newStatus == Status::INVALID) {
        // TODO: Do not free the payload object for now as it might be accessed in the completion
        // callback in another thread
        // d_->payload.reset();
        d_->onStatusChange = nullptr;
    }
    return true;
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
    unsigned contentFmt = COAP_FORMAT_TEXT_PLAIN;
    coap_option* opt = nullptr;
    CHECK(coap_get_option(msg.get(), &opt, COAP_OPTION_CONTENT_FORMAT, nullptr /* reserved */));
    if (opt) {
        CHECK(coap_get_uint_option_value(opt, &contentFmt, nullptr /* reserved */));
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
        if (contentFmt == COAP_FORMAT_CBOR) {
            d->contentType = ContentType::STRUCTURED;
        } else {
            d->contentType = (ContentType)contentFmt; // ContentType values are the CoAP's content format IDs
        }
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
void CloudEvent::sendComplete(int err, int /* reqId */, void* arg) {
    auto d = RefCountPtr<Data>::wrap(static_cast<Data*>(arg));
    d->sendResult = err;
    // Run a callback in the application thread to update the status of the event
    int r = application_thread_invoke([](void* arg) {
        CloudEvent event(RefCountPtr<Data>::wrap(static_cast<Data*>(arg)));
        auto newStatus = Status::SENT;
        int err = event.d_->sendResult;
        if (err < 0) {
            LOG(ERROR, "Failed to send event: %d", err);
            newStatus = Status::FAILED;
        }
        size_t size = event.size();
        if (!event.testAndSetStatus(Status::SENDING, newStatus, err)) {
            return; // The event was cancelled
        }
        RateLimiter::instance().give(size);
    }, d.get(), nullptr /* reserved */);
    // FIXME: application_thread_invoke() doesn't really handle errors as of now
    if (r == 0) {
        // Keep the reference around until the application callback is called
        d.unwrap();
    }
}

} // namespace particle
