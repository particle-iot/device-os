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

#include "subscriptions.h"

#include "spark_descriptor.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "coap_util.h"
#include "logging.h"

namespace particle::protocol {

ProtocolError Subscriptions::send_subscription_impl(MessageChannel& channel, const char* filter, size_t filterLen, int flags) {
    Message msg;
    auto err = channel.create(msg);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }

    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(channel.is_unreliable() ? CoapType::CON : CoapType::NON);
    e.code(CoapCode::GET);
    e.id(0); // Will be assigned and serialized by the message channel
    // Subscription messages have an empty token
    e.option(CoapOption::URI_PATH /* 11 */, "e");
    e.option(CoapOption::URI_PATH /* 11 */, filter, filterLen);
    if (flags & SubscriptionFlag::CBOR_DATA) { // Mutually exclusive with BINARY_DATA
        e.option(CoapOption::URI_QUERY /* 15 */, "c");
    } else if (flags & SubscriptionFlag::BINARY_DATA) {
        e.option(CoapOption::URI_QUERY /* 15 */, "b");
    }
    if (flags & SubscriptionFlag::LARGE_EVENT) {
        e.option(CoapOption::URI_QUERY /* 15 */, "l");
    }
    int r = e.encode();
    if (r < 0) {
        return ProtocolError::INTERNAL; // Should not happen
    }
    if (r > (int)msg.capacity()) {
        return ProtocolError::INSUFFICIENT_STORAGE;
    }
    msg.set_length(r);

    err = channel.send(msg);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }
    subscription_msg_ids.append(msg.get_id());

    return ProtocolError::NO_ERROR;
}

ProtocolError Subscriptions::handle_event(Message& msg, SparkDescriptor::CallEventHandlerCallback callback, MessageChannel& channel, bool& handled) {
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        return ProtocolError::MALFORMED_MESSAGE;
    }

    char name[MAX_EVENT_NAME_LENGTH + 1];
    size_t nameLen = 0;
    int contentFmt = (int)CoapContentFormat::TEXT_PLAIN;
    bool skipUriPrefix = true;

    auto it = d.options();
    while (it.next()) {
        switch (it.option()) {
        case (int)CoapOption::URI_PATH: {
            if (skipUriPrefix) {
                skipUriPrefix = false;
                continue; // Skip the "e/" or "E/" part
            }
            nameLen += appendUriPath(name, sizeof(name), nameLen, it);
            if (nameLen >= sizeof(name)) {
                nameLen = sizeof(name) - 1; // Truncate the event name
            }
            break;
        }
        case (int)CoapOption::CONTENT_FORMAT: {
            contentFmt = it.toUInt();
        }
        default:
            break;
        }
    }

    FilteringEventHandler* oldHandlers[MAX_SUBSCRIPTIONS] = {}; // Legacy subscription handlers found for the event
    size_t oldHandlerCount = 0; // Number of legacy subscription handlers found
    bool newHandlerFound = false; // Whether a new subscription handler is found

    for (size_t i = 0; i < MAX_SUBSCRIPTIONS; ++i) {
        auto& eventHandler = event_handlers[i];
        if (!eventHandler.handler && !(eventHandler.flags & SubscriptionFlag::LARGE_EVENT)) {
            break; // End of the handlers list
        }
        size_t filterLen = strnlen(eventHandler.filter, sizeof(eventHandler.filter));
        if (nameLen < filterLen || std::memcmp(eventHandler.filter, name, filterLen) != 0) {
            continue; // Event name mismatch
        }
        if ((eventHandler.flags & SubscriptionFlag::CBOR_DATA) && contentFmt != CoapContentFormat::APPLICATION_CBOR) {
            continue; // Encoding mismatch
        }
        if (eventHandler.flags & SubscriptionFlag::LARGE_EVENT) {
            newHandlerFound = true;
            break; // The request will be handled by the new CoAP implementation
        }
        if (!(eventHandler.flags & (SubscriptionFlag::BINARY_DATA | SubscriptionFlag::CBOR_DATA)) && !isCoapTextContentFormat(contentFmt)) {
            continue; // Encoding mismatch (old event API)
        }
        oldHandlers[oldHandlerCount++] = &eventHandler;
    }

    if (newHandlerFound) {
        handled = false;
        return ProtocolError::NO_ERROR;
    }

    // Acknowledge the request
    if (d.type() == CoapType::CON && channel.is_unreliable()) {
        int r = sendEmptyAck(channel, msg);
        if (r < 0) {
            LOG(ERROR, "Failed to send ACK: %d", r);
            return ProtocolError::COAP_ERROR;
        }
    }

    if (!oldHandlerCount) {
        handled = false;
        return ProtocolError::NO_ERROR;
    }

    char* data = nullptr;
    size_t dataSize = d.payloadSize();
    if (dataSize > 0) {
        data = const_cast<char*>(d.payload());
        // Historically, the event handler callback expected a null-terminated string. Keeping that
        // behavior for now
        if (msg.length() >= msg.capacity()) {
            std::memmove(data - 1, data, dataSize); // Overwrites the payload marker
            --data;
        }
        data[dataSize] = '\0';
    }

    for (size_t i = 0; i < oldHandlerCount; ++i) {
        callback(sizeof(FilteringEventHandler), oldHandlers[i], name, data, dataSize, contentFmt);
    }

    handled = true;
    return ProtocolError::NO_ERROR;
}

} // namespace particle::protocol
