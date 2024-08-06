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
    e.option(CoapOption::URI_PATH, "e"); // 11
    e.option(CoapOption::URI_PATH, filter, filterLen); // 11
    if (flags & SubscriptionFlag::BINARY_DATA) {
        e.option(CoapOption::URI_QUERY, "b"); // 15
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

ProtocolError Subscriptions::handle_event(Message& msg, SparkDescriptor::CallEventHandlerCallback callback, MessageChannel& channel) {
    CoapMessageDecoder d;
    int r = d.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        return ProtocolError::MALFORMED_MESSAGE;
    }

    if (d.type() == CoapType::CON && channel.is_unreliable()) {
        int r = sendEmptyAckOrRst(channel, msg, CoapType::ACK);
        if (r < 0) {
            LOG(ERROR, "Failed to send ACK: %d", r);
            return ProtocolError::COAP_ERROR;
        }
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
    if (!nameLen) {
        return ProtocolError::NO_ERROR; // Ignore an event without a name
    }

    for (size_t i = 0; i < MAX_SUBSCRIPTIONS; ++i) {
        if (!event_handlers[i].handler) {
            break;
        }
        size_t filterLen = strnlen(event_handlers[i].filter, sizeof(event_handlers[i].filter));
        if (nameLen < filterLen) {
            continue;
        }
        if (!std::memcmp(event_handlers[i].filter, name, filterLen)) {
            if (!(event_handlers[i].flags & SubscriptionFlag::BINARY_DATA) && !isCoapTextContentFormat(contentFmt)) {
                continue; // Do not invoke a handler that only accepts text data
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
            callback(sizeof(FilteringEventHandler), &event_handlers[i], name, data, dataSize, contentFmt);
        }
    }
    return ProtocolError::NO_ERROR;
}

} // namespace particle::protocol
