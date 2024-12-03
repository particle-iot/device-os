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
#include "spark_wiring_vector.h"

#include "coap_api.h"

#include "ref_count.h"

struct coap_payload;

namespace particle {

class Buffer;
class Variant;

class CoapMessagePtr;

/**
 * A cloud event.
 */
class CloudEvent: public Stream {
public:
    /**
     * Event status.
     */
    enum Status {
        /**
         * The initial status of a newly created or received event.
         *
         * An event with this status is accessible for reading and writing.
         */
        NEW,
        /**
         * The event is being sent to the Cloud.
         *
         * An event with this status is accessible only for reading.
         */
        SENDING,
        /**
         * The event was successfully sent to the Cloud.
         *
         * An event with this status is accessible for reading and writing.
         */
        SENT,
        /**
         * An error occured while creating the event or sending it to the Cloud.
         *
         * This status indicates a recoverable error. The failed operation with the event can be
         * retried when the condition that caused the error is resolved.
         *
         * An event with this status is accessible for reading and writing.
         *
         * @see `error()`.
         */
        FAILED,
        /**
         * An irrecoverable error occured while creating the event.
         *
         * An event with this status is not accessible for reading or writing.
         *
         * @see `error()`.
         */
        INVALID
    };

    /**
     * Signature of a callback invoked when the status of an event changes.
     *
     * @param event Event instance.
     */
    typedef void OnStatusChange(CloudEvent event);

    /**
     * Maximum supported size of event data.
     */
    static const size_t MAX_DATA_SIZE = COAP_MAX_PAYLOAD_SIZE;

    /**
     * Default constructor.
     *
     * Constructs an empty event.
     */
    CloudEvent();

    /**
     * Copy constructor.
     *
     * Creates a shallow copy of the event that references the same data.
     *
     * @param event Event to copy.
     */
    CloudEvent(const CloudEvent& event);

    /**
     * Move constructor.
     *
     * @param event Event to move from.
     */
    CloudEvent(CloudEvent&& event);

    /**
     * Destructor.
     *
     * Destroying an event that is being sent to the Cloud does not cancel the sending operation.
     *
     * @see `cancel()`.
     */
    ~CloudEvent();

    /**
     * Set the event name.
     *
     * @param name Event name.
     * @return This event instance.
     */
    CloudEvent& name(const char* name);

    /**
     * Get the event name.
     *
     * @return Event name.
     */
    const char* name() const;

    /**
     * Set the content type of the event data.
     *
     * @param type Content type.
     * @return This event instance.
     */
    CloudEvent& contentType(ContentType type);

    /**
     * Get the content type of the event data.
     *
     * @return Content type.
     */
    ContentType contentType() const;

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @return This event instance.
     */
    CloudEvent& data(const char* data) {
        return this->data(data, std::strlen(data));
    }

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @param data Data size.
     * @return This event instance.
     */
    CloudEvent& data(const char* data, size_t size);

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @param data Data size.
     * @param type Content type.
     * @return This event instance.
     */
    CloudEvent& data(const char* data, size_t size, ContentType type) {
        this->data(data, size);
        contentType(type);
        return *this;
    }

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @return This event instance.
     */
    CloudEvent& data(const Buffer& data) {
        return this->data(data.data(), data.size());
    }

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @param type Content type.
     * @return This event instance.
     */
    CloudEvent& data(const Buffer& data, ContentType type) {
        return this->data(data.data(), data.size(), type);
    }

    /**
     * Set the event data.
     *
     * @param data Event data.
     * @return This event instance.
     */
    CloudEvent& data(const Variant& data);

    /**
     * Get the event data.
     *
     * This method returns a copy of the event data in a dynamically allocated buffer.
     *
     * @return Event data.
     */
    Buffer data() const;

    /**
     * Get the event data.
     *
     * @return Event data.
     */
    String dataAsString() const;

    /**
     * Get the event data.
     *
     * @return Event data.
     */
    Variant dataAsVariant() /* FIXME: const */; // TODO: Rename?

    /**
     * Load the event data from a file.
     *
     * @param path File path.
     * @return This event instance.
     */
    CloudEvent& loadData(const char* path);

    /**
     * Save the event data to a file.
     *
     * @param path File path.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int saveData(const char* path);

    /**
     * Check if the event has payload data.
     *
     * @return `true` if the event has payload data, otherwise `false`.
     */
    bool hasData() const {
        return size() > 0;
    }

    /**
     * Set the maximum size of event data that can be store on the heap.
     *
     * The data exceeding the specified size will be stored in a temporary file.
     *
     * This method has no effect if the event already contains any data.
     *
     * The default value is 1024 bytes.
     *
     * @param size Data size.
     * @return This event instance.
     */
    CloudEvent& maxHeapSize(size_t size);

    /**
     * Get the maximum size of event data that can be store on the heap.
     *
     * @return Data size.
     */
    size_t maxHeapSize() const;

    /**
     * Set a callback to be invoked when the status of the event changes.
     *
     * @param callback Callback.
     * @return This event instance.
     */
    CloudEvent& onStatusChange(std::function<OnStatusChange> callback);

    /**
     * Get the status of the event.
     *
     * @return Event status.
     */
    Status status() const;

    /**
     * Check if the event is valid.
     *
     * @return `true` if the event is valid, otherwise `false`.
     */
    bool valid() const {
        return status() != Status::INVALID;
    }

    /**
     * Check if the event is being sent to the Cloud.
     *
     * @return `true` if the event is being sent to the Cloud, otherwise `false`.
     */
    bool sending() {
        return status() == Status::SENDING;
    }

    /**
     * Check if the event was sent to the Cloud successfully.
     *
     * @return `true` if the event was sent successfully, otherwise `false`.
     */
    bool sent() const {
        return status() == Status::SENT;
    }

    bool ok() const {
        auto s = status();
        return s != Status::FAILED && s != Status::INVALID;
    }

    int error() const;

    /**
     * Cancel sending the event.
     *
     * Calling this method is no-op if the event is not currently being sent to the Cloud.
     */
    void cancel();

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
    typedef void OnEventReceived(CloudEvent event);

    // The methods below are called by CloudClass

    int publish();

    static int subscribe(const char* prefix, std::function<OnEventReceived> callback);
    static void unsubscribeAll();

private:
    struct Data;
    struct Subscription;

    RefCountPtr<Data> d_;

    static Vector<Subscription> s_subscriptions;

    explicit CloudEvent(RefCountPtr<Data> data);

    int send();
    coap_payload* getValidPayload();
    void setStatus(Status status, int err = 0);

    // Note: Make sure to log an error message when transitioning to a recoverable failed state
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

    static int receiveRequestApp(CoapMessagePtr msg);
    static int receiveRequestSystem(coap_message* msg, const char* path, int method, int req_id, void* arg);

    static void sendComplete(int err, void* arg);

    friend class ::CloudClass;
};

} // namespace particle
