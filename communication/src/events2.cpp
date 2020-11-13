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

int Events::beginEvent(int handle, const char* name, int type, int size, unsigned flags,
        spark_protocol_event_status_fn statusFn, void* userData) {
    if (handle <= 0) {
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
                e.bytesAvail = e.bufSize;
            } else {
                e.bytesAvail = MAX_BLOCK_SIZE;
            }
            e.hasMoreData = true;
        }
        e.contentType = (spark_protocol_content_type)type;
        e.statusFn = statusFn;
        e.userData = userData;
        if (!outEvents_.append(std::move(e))) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        if (!size) {
            // Send an empty event immediately
            const int r = sendEvent(&outEvents_.last());
            if (r < 0) {
                outEvents_.takeLast();
                return r;
            }
        }
        return ++lastEventHandle_;
    } else {
        const int index = eventIndexForHandle(inEvents_, handle);
        if (index < 0) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
        Event* e = &inEvents_.at(index);
        e->statusFn = statusFn;
        e->userData = userData;
        return 0;
    }
}

void Events::endEvent(int handle) {
    int index = 0;
    if ((index = eventIndexForHandle(outEvents_, handle)) >= 0) {
        outEvents_.removeAt(index);
    } else if ((index = eventIndexForHandle(inEvents_, handle)) >= 0) {
        inEvents_.removeAt(index);
    }
}

int Events::readEventData(int handle, char* data, size_t size) {
    const int index = eventIndexForHandle(inEvents_, handle);
    if (index < 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    auto e = &inEvents_.at(index);
    if (!e->bytesAvail) {
        if (e->hasMoreData) {
            return SYSTEM_ERROR_WOULD_BLOCK;
        }
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    if (size > e->bytesAvail) {
        size = e->bytesAvail;
    }
    memcpy(data, e->buf.get() + e->bufOffs, size);
    e->bufOffs += size;
    e->dataOffs += size;
    e->bytesAvail -= size;
    if (!e->bytesAvail && e->hasMoreData) {
        CHECK(sendResponse(CoapType::CON, CoapCode::CONTINUE, -1 /* id */, e->token));
    }
    return size;
}

int Events::writeEventData(int handle, const char* data, size_t size, bool hasMore) {
    const int index = eventIndexForHandle(outEvents_, handle);
    if (index < 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    auto e = &outEvents_.at(index);
    if (!e->bytesAvail) {
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
            e->bytesAvail = e->bufSize;
        }
        if (size > e->bytesAvail) {
            size = e->bytesAvail;
        }
        memcpy(e->buf.get() + e->bufOffs, data, size);
        e->bufOffs += size;
        e->dataOffs += size;
        e->bytesAvail -= size;
    }
    if (!hasMore || !e->bytesAvail) {
        e->hasMoreData = hasMore;
        e->bytesAvail = 0;
        CHECK(sendEvent(e));
    }
    return size;
}

int Events::eventDataBytesAvailable(int handle) {
    int index = 0;
    if ((index = eventIndexForHandle(outEvents_, handle)) >= 0) {
        auto e = &outEvents_.at(index);
        size_t n = e->bytesAvail;
        if (e->dataSize >= 0 && e->dataSize - e->dataOffs < n) {
            n = e->dataSize - e->dataOffs;
        }
        return n;
    } else if ((index = eventIndexForHandle(inEvents_, handle)) >= 0) {
        auto e = &outEvents_.at(index);
        return e->bytesAvail;
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Events::addSubscription(const char* prefix, spark_protocol_subscription_fn fn, void* userData) {
    Subscription s = {};
    s.prefix = prefix;
    if (!s.prefix) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    s.prefixLen = strlen(prefix);
    s.fn = fn;
    s.userData = userData;
    if (!subscr_.append(std::move(s))) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // TODO: Update the checksum
    return 0;
}

void Events::clearSubscriptions() {
    subscr_.clear();
    subscrChecksum_ = 0;
}

int Events::sendSubscriptions() {
    return 0; // TODO
}

int Events::processEventRequest(Message* msg) {
    const auto timeRecv = protocol_->millis();
    CoapMessageDecoder dec;
    CHECK(dec.decode((const char*)msg->buf(), msg->length()));
    if (dec.tokenSize() != sizeof(token_t)) {
        return SYSTEM_ERROR_PROTOCOL; // Invalid token size
    }
    CString eventName;
    size_t eventNameLen = 0;
    size_t dataSize = 0;
    size_t blockSize = 0;
    unsigned uriOptCount = 0;
    unsigned blockIndex = 0;
    bool hasBlockOpt = false;
    bool hasSizeOpt = false;
    bool hasMoreData = false;
    auto it = dec.options();
    while (it.next()) {
        switch ((CoapOption)it.option()) { // :(
        case CoapOption::URI_PATH: {
            if (uriOptCount > 0) {
                eventNameLen += it.size();
            } else if (it.size() != 1 || (*it.data() != 'E' && *it.data() != 'e')) {
                return SYSTEM_ERROR_PROTOCOL; // Unexpected Uri-Path
            }
            ++uriOptCount;
            break;
        }
        case CoapOption::BLOCK1: {
            if (hasBlockOpt) {
                return SYSTEM_ERROR_PROTOCOL; // Duplicate option
            }
            const auto val = it.toUInt();
            const auto szx = val & 0x07; // SZX
            static_assert(MAX_BLOCK_SIZE == 1024 || MAX_BLOCK_SIZE == 512, "Unsupported MAX_BLOCK_SIZE");
            if (szx == 6) {
                blockSize = 1024;
            } else if (szx == 5) {
                blockSize = 512;
            } else {
                return SYSTEM_ERROR_PROTOCOL; // Unsupported block size
            }
            hasMoreData = val & 0x08; // M
            blockIndex = val >> 4; // NUM
            hasBlockOpt = true;
            break;
        }
        case CoapOption::SIZE1: {
            if (hasSizeOpt) {
                return SYSTEM_ERROR_PROTOCOL; // Duplicate option
            }
            dataSize = it.toUInt();
            hasSizeOpt = true;
            break;
        }
        // TODO: Content-Format
        default:
            break;
        }
    }
    if (!eventNameLen) {
        return SYSTEM_ERROR_PROTOCOL; // Event name is missing
    }
    if (hasBlockOpt && ((hasMoreData && dec.payloadSize() != blockSize) || (!hasMoreData && dec.payloadSize() > blockSize))) {
        return SYSTEM_ERROR_PROTOCOL; // Invalid payload size
    }
    auto eventNameBuf = (char*)malloc(eventNameLen + uriOptCount - 1);
    if (!eventNameBuf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    eventName = CString::wrap(eventNameBuf);
    uriOptCount = 0;
    it = dec.options();
    while (it.next()) {
        if (it.option() == CoapOption::URI_PATH) {
            if (uriOptCount > 0) {
                if (uriOptCount > 1) {
                    *eventNameBuf++ = '/';
                }
                memcpy(eventNameBuf, it.data(), it.size());
                eventNameBuf += it.size();
            }
            ++uriOptCount;
        }
    }
    *eventNameBuf = '\0';
    Event* event = nullptr;
    if (!hasBlockOpt || !blockIndex) {
        Event e = {};
        e.handle = lastEventHandle_ + 1;
        e.name = std::move(eventName);
        if (hasBlockOpt && hasMoreData) {
            if (hasSizeOpt) {
                e.dataSize = dataSize;
                e.bufSize = std::min<size_t>(e.dataSize, MAX_BLOCK_SIZE);
            } else {
                e.dataSize = -1;
                e.bufSize = MAX_BLOCK_SIZE;
            }
            e.dataSize = hasSizeOpt ? (int)dataSize : -1;
        } else {
            e.dataSize = dec.payloadSize();
            e.bufSize = e.dataSize;
        }
        if (e.bufSize) {
            e.buf.reset(new(std::nothrow) char[e.bufSize]);
            if (!e.buf) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        if (!inEvents_.append(std::move(e))) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        event = &inEvents_.last();
        ++lastEventHandle_;
    } else {
        for (auto& e: inEvents_) {
            if (strcmp(e.name, eventName) == 0) {
                event = &e;
                break;
            }
        }
        if (!event) {
            return SYSTEM_ERROR_INTERNAL; // FIXME
        }
    }
    if (hasBlockOpt) {
        if (blockIndex != event->blockIndex) {
            return SYSTEM_ERROR_PROTOCOL;
        }
        event->hasMoreData = hasMoreData;
    }
    event->bufOffs = 0;
    event->bytesAvail = dec.payloadSize();
    memcpy(event->buf.get(), dec.payload(), dec.payloadSize());
    memcpy(&event->token, dec.token(), dec.tokenSize());
    event->lastBlockTime = timeRecv;
    if (!event->hasMoreData) {
        CHECK(sendResponse(CoapType::ACK, CoapCode::CHANGED, dec.id(), event->token));
    } else {
        CHECK(sendEmptyAck(dec.id()));
        ++event->blockIndex;
    }
    if (!hasBlockOpt || !blockIndex) {
        const Subscription* subscr = nullptr;
        for (const auto& s: subscr_) {
            if (eventNameLen <= s.prefixLen && strncmp(s.prefix, event->name, eventNameLen) == 0) {
                subscr = &s;
                break;
            }
        }
        if (subscr) {
            subscr->fn(event->handle, event->name, PROTOCOL_CONTENT_TYPE_PLAIN_TEXT, event->dataSize, subscr->userData);
        } else {
            inEvents_.takeLast(); // FIXME
        }
    } else if (event->statusFn) {
        event->statusFn(event->handle, PROTOCOL_EVENT_STATUS_READABLE, event->userData);
    }
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
        event->bytesAvail = event->bufSize;
        if (event->statusFn) {
            event->statusFn(event->handle, PROTOCOL_EVENT_STATUS_WRITABLE, event->userData);
        }
    } else if (event->statusFn) {
        event->statusFn(event->handle, PROTOCOL_EVENT_STATUS_SENT, event->userData);
    }
    return 0;
}

int Events::processTimeouts() {
    return 0; // TODO
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
        // RFC 7959, 2.2. Structure of a Block Option
        static_assert(MAX_BLOCK_SIZE == 1024 || MAX_BLOCK_SIZE == 512, "Unsupported MAX_BLOCK_SIZE");
        const unsigned szx = (event->bufSize == 1024) ? 6 /* 1024 bytes */ : 5 /* 512 bytes */;
        unsigned block1 = (event->blockIndex << 4) | szx;
        if (event->hasMoreData) {
            block1 |= 0x08;
        }
        enc.option(CoapOption::BLOCK1, block1);
    }
    // TODO: Content-Format
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
    event->lastBlockTime = protocol_->millis();
    event->token = token;
    if (event->hasMoreData) {
        ++event->blockIndex;
    }
    return 0;
}

int Events::sendResponse(CoapType type, CoapCode code, int id, token_t token) {
    const auto channel = &protocol_->getChannel();
    Message msg;
    int r = channel->create(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(type);
    enc.code(code);
    enc.id(0); // Will be set by the message channel
    enc.token((const char*)&token, sizeof(token));
    r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    if (id >= 0) {
        msg.set_id(id);
    }
    r = channel->send(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_IO;
    }
    return 0;
}

int Events::sendEmptyAck(CoapMessageId id) {
    const auto channel = &protocol_->getChannel();
    Message msg;
    int r = channel->create(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(CoapType::ACK);
    enc.code(CoapCode::EMPTY);
    enc.id(0); // Will be set by the message channel
    r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    msg.set_id(id);
    r = channel->send(msg);
    if (r != ProtocolError::NO_ERROR) {
        return SYSTEM_ERROR_IO;
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
