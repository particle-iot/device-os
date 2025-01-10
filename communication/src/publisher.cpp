/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include <algorithm>
#include <cstring>

#include "publisher.h"

#include "protocol.h"
#include "coap_message_encoder.h"

namespace particle {

namespace protocol {

void Publisher::add_ack_handler(message_id_t msg_id, CompletionHandler handler) {
    protocol->add_ack_handler(msg_id, std::move(handler), SEND_EVENT_ACK_TIMEOUT);
}

ProtocolError Publisher::send_event(MessageChannel& channel, const char* event_name, const char* data, size_t data_size,
        int content_type, int ttl, int flags, system_tick_t time, CompletionHandler handler) {
    bool is_system_event = is_system(event_name);
    bool rate_limited = is_rate_limited(is_system_event, time);
    if (rate_limited) {
        g_rateLimitedEventsCounter++;
        return BANDWIDTH_EXCEEDED;
    }

    Message msg;
    auto err = channel.create(msg);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }

    bool confirmable = channel.is_unreliable();
    if (flags & EventType::NO_ACK) {
        confirmable = false;
    } else if (flags & EventType::WITH_ACK) {
        confirmable = true;
    }

    CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
    e.type(confirmable ? CoapType::CON : CoapType::NON);
    e.code(CoapCode::POST);
    e.id(0); // Will be assigned and serialized by the message channel
    // Event messages have an empty token
    e.option(CoapOption::URI_PATH, "E"); // 11
    size_t name_len = strnlen(event_name, MAX_EVENT_NAME_LENGTH);
    e.option(CoapOption::URI_PATH, event_name, name_len); // 11
    if (content_type != CoapContentFormat::TEXT_PLAIN) {
        e.option(CoapOption::CONTENT_FORMAT, content_type); // 12
    }
    if (ttl != 60) {
        // XXX: Max-Age is not supposed to be used in a request message
        e.option(CoapOption::MAX_AGE, ttl); // 14
    }
    if (data_size > 0) {
        auto max_data_size = std::min(e.maxPayloadSize(), protocol->get_max_event_data_size());
        if (data_size > max_data_size) {
            LOG(WARN, "Event data size exceeds limit of %d bytes", (int)max_data_size);
            data_size = max_data_size;
        }
        e.payload(data, data_size);
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

    // Register completion handler only if acknowledgement was requested explicitly
    if ((flags & EventType::WITH_ACK) && msg.has_id()) {
        add_ack_handler(msg.get_id(), std::move(handler));
    } else {
        handler.setResult();
    }

    return ProtocolError::NO_ERROR;
}

} // protocol

} // particle
