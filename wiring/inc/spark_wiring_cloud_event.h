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

#include "spark_wiring_variant.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_vector.h"

#include "coap_api.h"

#include "ref_count.h"

class CloudClass;

struct coap_payload;

namespace particle {

class Buffer;
class CoapMessagePtr;

typedef Variant EventData;

enum class ContentType: int;

/**
 * Subscription options.
 */
class SubscribeOptions {
public:
    /**
     * Default constructor.
     *
     * Constructs an options object with default parameters.
     */
    SubscribeOptions() :
            struct_(false) {
    }

    /**
     * Enable/disable encoding of event data in a structured data format.
     *
     * This option instructs the Cloud to encode all events sent to the device for this subscription
     * in a compact structured data format.
     *
     * The exact format used is implementation-specific. The application is expected to parse the
     * data in this format using the methods of the `CloudEvent` class, such as `dataStructured()`.
     *
     * By default, this option is disabled.
     *
     * @param enabled Whether the option is enabled.
     * @return This options object.
     */
    SubscribeOptions& structured(bool enabled) {
        struct_ = enabled;
        return *this;
    }

    /**
     * Check if encoding of event data in a structured data format is enabled.
     *
     * @return `true` if the option is enabled, otherwise `false`.
     */
    bool structured() const {
        return struct_;
    }

private:
    bool struct_;
};

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
     *
     * Events containing up to 16384 bytes of data are supported.
     */
    static constexpr size_t MAX_SIZE = COAP_MAX_PAYLOAD_SIZE;

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
    CloudEvent& data(const String& data) {
        this->data(data.c_str(), data.length());
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
     * The event data will be encoded in a compact structured format that will be expanded as JSON
     * in the Cloud once the event is published.
     *
     * @param data Event data.
     * @return This event instance.
     */
    CloudEvent& data(const EventData& data);

    /**
     * Get the event data.
     *
     * This method returns a copy of the event data in a dynamically allocated buffer.
     *
     * @return Event data.
     */
    Buffer data() const;

    /**
     * Get the event data as a `String`.
     *
     * @return Event data.
     */
    String dataString() const;

    /**
     * Parse the structured event data.
     *
     * The event data is expected to be encoded in the structured data format. See the documentation
     * for `SubscribeOptions::structured(bool)` for details.
     *
     * @return Parsed event data.
     */
    EventData dataStructured() const;

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
     * Set the size of the event data.
     *
     * @param size New size.
     * @return 0 on success, otherwise an error code defined by `Error::Type`.
     */
    int setSize(size_t size);

    /**
     * Get the size of the event data.
     *
     * @return Data size.
     */
    size_t size() const;

    /**
     * Check if the event data is empty.
     *
     * @return `true` if the event data is empty, otherwise `false`.
     */
    bool isEmpty() const {
        return !size();
    }

    /**
     * Set the current position in the event data.
     *
     * @param pos New position.
     * @return On success, the current position in the event data. Otherwise, an error code defined
     *         by `Error::Type`.
     */
    int seek(size_t pos);

    /**
     * Get the current position in the event data.
     *
     * @return Current position.
     */
    size_t pos() const;

    /**
     * Set the maximum size of event data that can be stored on the heap.
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
    CloudEvent& maxDataInRam(size_t size);

    /**
     * Get the maximum size of event data that can be store on the heap.
     *
     * @return Data size.
     */
    size_t maxDataInRam() const;

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
     * Check if this is a newly created event.
     *
     * This method is a shorthand for `event.status() == CloudEvent::NEW`.
     *
     * @return `true` if this is a new event, otherwise `false`.
     */
    bool isNew() const {
        return status() == Status::NEW;
    }

    /**
     * Check if the event is being sent to the Cloud.
     *
     * This method is a shorthand for `event.status() == CloudEvent::SENDING`.
     *
     * @return `true` if the event is being sent to the Cloud, otherwise `false`.
     */
    bool isSending() const {
        return status() == Status::SENDING;
    }

    /**
     * Check if the event was sent to the Cloud successfully.
     *
     * This method is a shorthand for `event.status() == CloudEvent::SENT`.
     *
     * @return `true` if the event was sent successfully, otherwise `false`.
     */
    bool isSent() const {
        return status() == Status::SENT;
    }

    /**
     * Check if the event is not in a failed or invalid state.
     *
     * This method is a shorthand for `event.status() != CloudEvent::FAILED && event.status() != CloudEvent::INVALID`.
     *
     * If the returned value is `false`, the error code of the last failed operation can be obtained
     * via `error()`.
     *
     * @return `true` if the event is not in a failed or invalid state, otherwise `false`.
     */
    bool isOk() const {
        auto s = status();
        return s != Status::FAILED && s != Status::INVALID;
    }

    /**
     * Check if the event is valid.
     *
     * This method is a shorthand for `event.status() != CloudEvent::INVALID`.
     *
     * @return `true` if the event is valid, otherwise `false`.
     */
    bool isValid() const {
        return status() != Status::INVALID;
    }

    /**
     * Get the error code.
     *
     * @return 0 if the event is not in a failed or invalid state, otherwise an error code defined
     *         by `Error::Type`.
     */
    int error() const;

    /**
     * Reset the status of the event.
     *
     * This method resets the status of the event back to `NEW` if the current status is `SENT` or
     * `FAILED`. Otherwise, it has no effect.
     *
     * It is normally not necessary to call this method before publishing a failed event again.
     */
    void resetStatus();

    /**
     * Cancel sending the event.
     *
     * This method has no effect if the event is not currently being sent to the Cloud.
     *
     * A cancelled event is invalidated and cannot be published again.
     */
    void cancel();

    /**
     * Clear and reinitialize the event instance.
     *
     * Calling this method has the same effect as creating a new event instance:
     *
     * ```cpp
     * // These two statements are equivalent
     * event.clear();
     * event = CloudEvent();
     * ```
     */
    void clear();

    /**
     * Methods reimplemented from base `Stream` class.
     */
    ///@{
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

    void flush() override {
    }
    ///@}

    /**
     * Convenience overloads not available in the base `Stream` class.
     */
    ///@{
    int read(char* data, size_t size);
    int peek(char* data, size_t size);

    int write(const char* data) {
        return write(data, std::strlen(data));
    }

    int write(const char* data, size_t size);
    ///@}

    /**
     * Assignment operator.
     *
     * @param event Event instance to assign from.
     * @return This event instance.
     */
    CloudEvent& operator=(CloudEvent event);

    /**
     * Comparison operators.
     *
     * Two event instances are considered equal if they reference the same underlying event data.
     */
    ///@{
    bool operator==(const CloudEvent& event) const {
        return d_.get() == event.d_.get();
    }

    bool operator!=(const CloudEvent& event) const {
        return d_.get() != event.d_.get();
    }
    ///@}

    /**
     * Check if an event with a given size would be within the limit for the amount of event data in
     * flight once it's attempted to be published.
     *
     * @param size Size of the event data.
     * @return `true` if the event can be published, otherwise `false`.
     */
    static bool canPublish(size_t size);

    friend void swap(CloudEvent& event1, CloudEvent& event2) {
        using std::swap;
        swap(event1.d_, event2.d_);
    }

protected:
    typedef void OnEventReceived(CloudEvent event);

    int publish();

    static int subscribe(const char* prefix, std::function<OnEventReceived> callback,
            const SubscribeOptions& opts = SubscribeOptions());
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
    bool testAndSetStatus(Status expectedStatus, Status newStatus, int err = 0);

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
    static int receiveRequestSystem(coap_message* msg, const char* path, int method, int reqId, void* arg);

    static void sendComplete(int err, int reqId, void* arg);

    friend class ::CloudClass;
};

} // namespace particle
