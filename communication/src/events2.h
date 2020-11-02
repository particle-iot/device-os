/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "spark_protocol_functions.h"

#include "coap_defs.h"

#include "c_string.h"

#include "spark_wiring_vector.h"

#include <memory>
#include <cstdint>

namespace particle {

namespace protocol {

class Protocol;
class Message;
class CoapMessageDecoder;
class CoapMessageEncoder;

// FIXME: Move these declarations to spark_cloud_functions.h
typedef enum spark_protocol_event_status {
    PROTOCOL_EVENT_STATUS_SENT = 0x01,
    PROTOCOL_EVENT_STATUS_ERROR = 0x02,
    PROTOCOL_EVENT_STATUS_READABLE = 0x04,
    PROTOCOL_EVENT_STATUS_WRITABLE = 0x08
} spark_protocol_event_status;

typedef void(*spark_protocol_event_status_fn)(int handle, int status, int error, void* user_data);

typedef void(*spark_protocol_subscription_fn)(int handle, const char* name, spark_protocol_content_type type,
        size_t size, void* user_data);

class Events {
public:
    explicit Events(Protocol* protocol);
    ~Events() = default;

    int beginEvent(const char* name, spark_protocol_content_type type, int size, unsigned flags,
            spark_protocol_event_status_fn statusFn, void* userData);
    int beginEvent(int handle, spark_protocol_event_status_fn statusFn, void* userData);
    int endEvent(int handle, int error);
    int readEventData(int handle, char* data, size_t size);
    int writeEventData(int handle, const char* data, size_t size, bool hasMore);
    int eventDataBytesAvailable(int handle);

    int addSubscription(const char* prefix, spark_protocol_subscription_fn fn, void* userData);
    void clearSubscriptions();
    uint32_t subscriptionsChecksum();
    int sendSubscriptions();

    int processEventRequest(Message* msg);
    int processAck(Message* msg);
    int processTimeouts();

    void reset(bool clearSubscriptions = true, bool invokeCallbacks = false);

private:
    struct Event {
        CString name; // Event name
        std::unique_ptr<char[]> buf; // Buffer for the event data
        spark_protocol_event_status_fn statusFn; // Event status callback
        spark_protocol_content_type contentType; // Content type
        token_t token; // Message token
        void* userData; // Callback context
        size_t dataOffs; // Current offset in the event data
        size_t bufSize; // Buffer size
        size_t bufOffs; // Current offset in the buffer
        unsigned blockIndex; // Block index
        unsigned flags; // Event status flags
        int dataSize; // Total size of the event data
        int handle; // Event handle
    };

    struct Subscription {
        CString prefix; // Event name prefix
        spark_protocol_subscription_fn subscrFn; // Subscription callback
        void* userData; // Callback context
    };

    Vector<Event> inEvents_; // Inbound events
    Vector<Event> outEvents_; // Outbound events
    Vector<Subscription> subscr_; // Event subscriptions
    Protocol* protocol_;
    uint32_t subscrChecksum_; // CRC-32 of the event subscriptions
    int lastEventHandle_; // Last used event handle

    int sendEvent(Event* event, bool hasMore);

    static int eventIndexForHandle(const Vector<Event>& events, int handle);
};

inline Events::Events(Protocol* protocol) :
        protocol_(protocol),
        lastEventHandle_(0) {
    reset();
}

} // namespace protocol

} // namespace particle
