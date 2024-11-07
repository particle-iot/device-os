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
    enum Status {
        NEW,
        SENDING,
        SENT,
        FAILED
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

    CloudEvent& size(size_t size);
    size_t size() const;

    CloudEvent& onStatusChange(std::function<OnStatusChange> callback);

    Status status() const;

    bool isSent() const {
        return status() == Status::SENT;
    }

    bool ok() const {
        return status() != Status::FAILED;
    }
    
    // TODO: saveData(), loadData()

    int read() override;
    size_t readBytes(char* data, size_t size) override;
    int peek() override;
    int available() override;

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t* data, size_t size) override;

    size_t write(const char* data) {
        return write((const uint8_t*)data, std::strlen(data));
    }

    size_t write(const char* data, size_t size) {
        return write((const uint8_t*)data, size);
    }

    void flush() override {
    }

    CloudEvent& pos(size_t pos);
    size_t pos() const;

    int error() const;

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
    bool isWritable() const;

    void setStatus(Status status);
    int setError(int error);

    static void publishComplete(int err, void* arg);

    friend class ::CloudClass;
};

} // namespace particle
