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

/**
 * Handler for cloud events.
 */
class Events {
public:
    explicit Events(Protocol* protocol);
    ~Events() = default;

    int beginEvent(int handle, const char* name, int type, int size, unsigned flags,
            spark_protocol_event_status_fn statusFn, void* userData);
    void endEvent(int handle);
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
        system_tick_t lastBlockTime; // Time when the last block of the event data was sent or received
        token_t token; // Message token
        void* userData; // Callback context
        size_t dataOffs; // Current offset in the event data
        size_t bufSize; // Buffer size
        size_t bufOffs; // Current offset in the buffer
        size_t bytesAvail; // Number of bytes in the buffer available for reading or writing
        unsigned blockIndex; // CoAP block index
        unsigned flags; // Event flags
        int dataSize; // Total size of the event data
        int handle; // Event handle
        bool hasMoreData; // Whether the sender of the event has more data to send
    };

    struct Subscription {
        CString prefix; // Event name prefix
        spark_protocol_subscription_fn fn; // Subscription callback
        void* userData; // Callback context
        size_t prefixLen; // Length of the event name prefix
    };

    Vector<Event> inEvents_; // Inbound events
    Vector<Event> outEvents_; // Outbound events
    Vector<Subscription> subscr_; // Event subscriptions
    Protocol* protocol_; // Protocol instance
    uint32_t subscrChecksum_; // CRC-32 of the event subscriptions
    int lastEventHandle_; // Last used event handle

    int sendEvent(Event* event);
    int sendResponse(CoapType type, CoapCode code, int id, token_t token);
    int sendEmptyAck(CoapMessageId id);

    static int eventIndexForHandle(const Vector<Event>& events, int handle);
};

inline Events::Events(Protocol* protocol) :
        protocol_(protocol),
        lastEventHandle_(0) {
    reset();
}

inline uint32_t Events::subscriptionsChecksum() {
    return subscrChecksum_;
}

} // namespace protocol

} // namespace particle
