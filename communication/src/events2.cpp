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

#undef LOG_COMPILE_TIME_LEVEL // FIXME

#include "logging.h"

LOG_SOURCE_CATEGORY("comm.event")

#include "events2.h"

#include "protocol.h"
#include "message_channel.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"

#include "check.h"

#include <algorithm>

namespace particle {

namespace protocol {

const size_t MAX_SUBSCRIPTION_COUNT = 10; // FIXME: Move to protocol_defs.h

/**
 * Maximum block size in bytes.
 */
const size_t MAX_BLOCK_SIZE = 1024;

/**
 * Maximum CoAP overhead per event message:
 *
 * - Message header: 4 bytes;
 * - Token: 1 byte;
 * - Uri-Path: 68 bytes;
 * - Content-Format: 3 bytes;
 * - Block1: 6 bytes;
 * - Size1: 6 bytes;
 * - Payload marker: 1 byte.
 */
const size_t EVENT_COAP_OVERHEAD = 89;

static_assert(MBEDTLS_SSL_MAX_CONTENT_LEN >= MAX_BLOCK_SIZE + EVENT_COAP_OVERHEAD,
        "MBEDTLS_SSL_MAX_CONTENT_LEN is too small");

using std::swap; // For ADL

int Events::beginEvent(const char* name, spark_protocol_content_type type, int size, unsigned flags,
            spark_protocol_event_status_fn statusFn, void* userData) {
    Event e = {};
    e.handle = lastEventHandle_ + 1;
    e.name = name;
    if (!e.name) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    e.flags = flags;
    e.dataSize = size;
    if (e.dataSize) {
        if (e.dataSize > 0) {
            // Allocate a buffer for the event data if its size is known in advance
            e.bufSize = std::min<size_t>(e.dataSize, MAX_BLOCK_SIZE);
            e.buf.reset(new(std::nothrow) char[e.bufSize]);
            if (!e.buf) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        e.canReadWrite = true;
        e.hasMoreData = true;
    }
    e.contentType = type;
    e.statusFn = statusFn;
    e.userData = userData;
    if (!outEvents_.append(std::move(e))) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (!size) {
        // An empty event can be sent immediately
        const int r = sendEvent(&outEvents_.last());
        if (r < 0) {
            outEvents_.takeLast();
            return r;
        }
    }
    return ++lastEventHandle_;
}

int Events::beginEvent(int handle, spark_protocol_event_status_fn statusFn, void* userData) {
    return 0;
}

int Events::endEvent(int handle, int error) {
    int index = 0;
    if ((index = eventIndexForHandle(outEvents_, handle)) >= 0) {
        outEvents_.removeAt(index);
    } else if ((index = eventIndexForHandle(inEvents_, handle)) >= 0) {
        inEvents_.removeAt(index);
    }
    return 0;
}

int Events::readEventData(int handle, char* data, size_t size) {
    return 0;
}

int Events::writeEventData(int handle, const char* data, size_t size, bool hasMore) {
    const int index = eventIndexForHandle(outEvents_, handle);
    if (index < 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    auto e = &outEvents_.at(index);
    if (!e->canReadWrite) {
        if (!e->dataSize || !e->hasMoreData) {
            return SYSTEM_ERROR_INVALID_STATE; // The event data has been fully sent
        }
        return SYSTEM_ERROR_WOULD_BLOCK;
    }
    if (size > 0) {
        if (e->dataSize >= 0 && ((e->dataOffs + size > (size_t)e->dataSize) ||
                (hasMore && e->dataOffs + size == (size_t)e->dataSize) ||
                (!hasMore && e->dataOffs + size < (size_t)e->dataSize))) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        if (!e->bufSize) {
            e->bufSize = hasMore ? MAX_BLOCK_SIZE : size;
            e->buf.reset(new(std::nothrow) char[e->bufSize]);
            if (!e->buf) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        size = std::min(size, e->bufSize - e->bufOffs);
        memcpy(e->buf.get() + e->bufOffs, data, size);
        e->bufOffs += size;
        e->dataOffs += size;
    }
    if (!hasMore || (size > 0 && e->bufOffs == e->bufSize)) {
        e->hasMoreData = hasMore;
        e->canReadWrite = false;
        CHECK(sendEvent(e));
    }
    return size;
}

int Events::eventDataBytesAvailable(int handle) {
    size_t result = 0;
    int index = 0;
    if ((index = eventIndexForHandle(outEvents_, handle)) >= 0) {
        auto e = &outEvents_.at(index);
        if (e->canReadWrite) {
            result = e->bufSize ? e->bufSize - e->bufOffs : MAX_BLOCK_SIZE;
            if (e->dataSize >= 0 && e->dataSize - e->dataOffs < result) {
                result = e->dataSize - e->dataOffs;
            }
        }
    } else if ((index = eventIndexForHandle(inEvents_, handle)) >= 0) {
        // TODO
    } else {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    return result;
}

int Events::addSubscription(const char* prefix, spark_protocol_subscription_fn fn, void* userData) {
    return 0;
}

void Events::clearSubscriptions() {
}

uint32_t Events::subscriptionsChecksum() {
    return 0;
}

int Events::sendSubscriptions() {
    return 0;
}

int Events::processEventRequest(Message* msg) {
    return 0;
}

int Events::processAck(Message* msg) {
    CoapMessageDecoder dec;
    CHECK(dec.decode((const char*)msg->buf(), msg->length()));
    if (dec.tokenSize() != sizeof(token_t)) {
        return 0;
    }
    token_t token = 0;
    memcpy(&token, dec.token(), dec.tokenSize());
    Event* event = nullptr;
    for (auto& e: outEvents_) {
        if (e.token == token) {
            event = &e;
            break;
        }
    }
    if (!event) {
        return 0;
    }
    if (event->hasMoreData) {
        event->bufOffs = 0;
        event->canReadWrite = true;
        if (event->statusFn) {
            event->statusFn(event->handle, PROTOCOL_EVENT_STATUS_WRITABLE, event->userData);
        }
    } else if (event->statusFn) {
        event->statusFn(event->handle, PROTOCOL_EVENT_STATUS_SENT, event->userData);
    }
    return 0;
}

int Events::processTimeouts() {
    return 0;
}

void Events::reset(bool clearSubscriptions, bool invokeCallbacks) {
    Vector<Event> inEvents;
    swap(inEvents_, inEvents);
    Vector<Event> outEvents;
    swap(outEvents_, inEvents);
    if (clearSubscriptions) {
        subscr_.clear();
        subscrChecksum_ = 0;
    }
    // Note: Do not reinitialize lastEventHandle_. Event handles must be unique across different sessions
    if (invokeCallbacks) {
        for (const auto& e: inEvents) {
            if (e.statusFn) {
                e.statusFn(e.handle, SYSTEM_ERROR_CANCELLED, e.userData);
            }
        }
        for (const auto& e: outEvents) {
            if (e.statusFn) {
                e.statusFn(e.handle, SYSTEM_ERROR_CANCELLED, e.userData);
            }
        }
    }
}

int Events::sendEvent(Event* event) {
    const auto channel = &protocol_->getChannel();
    Message msg;
    int r = channel->create(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    CoapType type = CoapType::CON;
    if ((event->flags & EventType::NO_ACK) || (!channel->is_unreliable() && !(event->flags & EventType::WITH_ACK))) {
        type = CoapType::NON;
    }
    enc.type(type);
    enc.code(CoapCode::POST);
    enc.id(0); // Will be set by the message channel
    const auto token = protocol_->get_next_token();
    enc.token((const char*)&token, sizeof(token));
    enc.option(CoapOption::URI_PATH, "E");
    enc.option(CoapOption::URI_PATH, event->name);
    if (event->hasMoreData || event->blockIndex > 0) {
        // Encode Block1 option (see RFC 7959, 2.2. Structure of a Block Option)
        static_assert(MAX_BLOCK_SIZE == 1024 || MAX_BLOCK_SIZE == 512, "Unsupported MAX_BLOCK_SIZE");
        const unsigned szx = (event->bufSize == 1024) ? 6 /* 1024 bytes */ : 5 /* 512 bytes */;
        unsigned block1 = (event->blockIndex << 4) | szx;
        if (event->hasMoreData) {
            block1 |= 0x08;
        }
        enc.option(CoapOption::BLOCK1, block1);
    }
    enc.payload(event->buf.get(), event->bufOffs);
    r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    r = channel->send(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_IO;
    }
    event->timeSent = protocol_->millis();
    event->token = token;
    if (event->hasMoreData) {
        ++event->blockIndex;
    }
    return 0;
}

int Events::eventIndexForHandle(const Vector<Event>& events, int handle) {
    for (int i = 0; i < events.size(); ++i) {
        if (events.at(i).handle == handle) {
            return i;
        }
    }
    return -1;
}

} // namespace protocol

} // namespace particle
