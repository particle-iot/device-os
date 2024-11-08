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

#include <functional>
#include <utility>
#include <cstring>

#include "spark_wiring_cloud.h"
#include "spark_wiring_stream.h"

#include "coap_api.h"

#include "ref_count.h"

struct coap_payload;

namespace particle {

class Buffer;
class Variant;

class CloudEvent: public Stream {
public:
    /**
     * Event status.
     */
    enum Status {
        /**
         * The initial status of a newly created event.
         *
         * The event data is accessible for reading and writing.
         */
        NEW,
        /**
         * The event is being sent to the Cloud.
         *
         * The event data is accessible for reading only.
         */
        SENDING,
        /**
         * The event was successfully sent to the Cloud.
         *
         * The event data is accessible for reading and writing.
         */
        SENT,
        /**
         * An error occured while creating the event or sending it to the Cloud.
         *
         * The failed operation can be retried when the condition that caused the error is resolved.
         *
         * The event data is accessible for reading and writing.
         *
         * @see `error()`.
         */
        FAILED,
        /**
         * An irrecoverable error occured while creating the event.
         *
         * The event data is not accessible for reading or writing.
         *
         * @see `error()`.
         */
        INVALID
    };

    typedef void OnStatusChange(CloudEvent event);

    static const size_t MAX_DATA_SIZE = COAP_MAX_PAYLOAD_SIZE;

    CloudEvent();

    explicit CloudEvent(const char* name) :
            CloudEvent() {
        this->name(name);
    }

    CloudEvent(const char* name, const char* data) :
            CloudEvent(name) {
        this->data(data);
    }

    CloudEvent(const char* name, const char* data, size_t size, ContentType type) :
            CloudEvent(name) {
        this->data(data, size, type);
    }

    CloudEvent(const char* name, const Buffer& data, ContentType type) :
            CloudEvent(name) {
        this->data(data, type);
    }

    CloudEvent(const char* name, const Variant& data) :
            CloudEvent(name) {
        this->data(data);
    }

    CloudEvent(const CloudEvent& event);
    CloudEvent(CloudEvent&& event);

    ~CloudEvent();

    CloudEvent& name(const char* name);
    const char* name() const;

    CloudEvent& contentType(ContentType type);
    ContentType contentType() const;

    CloudEvent& data(const char* data) {
        return this->data(data, std::strlen(data));
    }

    CloudEvent& data(const char* data, size_t size);

    CloudEvent& data(const char* data, size_t size, ContentType type) {
        this->data(data, size);
        contentType(type);
        return *this;
    }

    CloudEvent& data(const Buffer& data) {
        return this->data(data.data(), data.size());
    }

    CloudEvent& data(const Buffer& data, ContentType type) {
        return this->data(data.data(), data.size(), type);
    }

    CloudEvent& data(const Variant& data);

    Buffer data() const;
    String dataAsString() const;
    Variant dataAsVariant() /* FIXME: const */; // TODO: Rename?

    CloudEvent& onStatusChange(std::function<OnStatusChange> callback);

    Status status() const;

    bool valid() const {
        return status() != Status::INVALID;
    }

    bool sending() {
        return status() == Status::SENDING;
    }

    bool sent() const {
        return status() == Status::SENT;
    }

    bool ok() const {
        auto s = status();
        return s != Status::FAILED && s != Status::INVALID;
    }

    int error() const;

    // TODO: saveData(), loadData()

    void reset();

    int read() override {
        char c;
        size_t n = read(&c, 1);
        if (n != 1) {
            return -1;
        }
        return (unsigned char)c;
    }

    size_t readBytes(char* data, size_t size) override {
        return read(data, size);
    }

    int peek() override {
        char c;
        size_t n = peek(&c, 1);
        if (n != 1) {
            return -1;
        }
        return (unsigned char)c;
    }

    int available() override;

    // Convenience overloads not available in Stream
    int read(char* data, size_t size);
    int peek(char* data, size_t size);

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t* data, size_t size) override {
        int r = write((const char*)data, size);
        if (r < 0) {
            return 0;
        }
        return r;
    }

    // Convenience overloads not available in Print
    int write(const char* data) {
        return write(data, std::strlen(data));
    }

    int write(const char* data, size_t size);

    void flush() override {
    }

    int size(size_t size);
    size_t size() const;

    int pos(size_t pos);
    size_t pos() const;

    CloudEvent& operator=(CloudEvent event);

    bool operator==(const CloudEvent& event) const {
        return d_.get() == event.d_.get();
    }

    bool operator!=(const CloudEvent& event) const {
        return d_.get() != event.d_.get();
    }

    friend void swap(CloudEvent& event1, CloudEvent& event2) {
        using std::swap;
        swap(event1.d_, event2.d_);
    }

protected:
    int publish(); // Called by CloudClass

private:
    struct Data;

    RefCountPtr<Data> d_;

    explicit CloudEvent(RefCountPtr<Data> data);

    int publishImpl();

    coap_payload* getValidPayload();

    void setStatus(Status status, int err = 0);

    int setFailed(int err) {
        setStatus(Status::FAILED, err);
        return error();
    }

    int setInvalid(int err) {
        setStatus(Status::INVALID, err);
        return error();
    }

    bool isReadable() const {
        return status() != Status::INVALID;
    }

    bool isWritable() const {
        auto s = status();
        return s != Status::SENDING && s != Status::INVALID;
    }

    static void publishComplete(int err, void* arg);

    friend class ::CloudClass;
};

} // namespace particle
