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

#include "variables.h"

#include "protocol.h"
#include "messages.h"

#include "endian_util.h"

#include <memory>
#include <cstring>

namespace particle {

namespace protocol {

struct Variables::Context {
    Context(Variables* self, token_t token) :
            self(self),
            token(token) {
    }

    Variables* self;
    token_t token;
};

ProtocolError Variables::handle_request(Message& message, token_t token, message_id_t id) {
    char key[MAX_VARIABLE_KEY_LENGTH + 1];
    auto result = decode_request(message, key);
    if (result != ProtocolError::NO_ERROR) {
        return send_error_ack(message, token, id, CoAPCode::BAD_REQUEST);
    }
    if (protocol_->get_descriptor().get_variable_async) {
        result = handle_request(message, token, id, key);
    } else {
        // Use the compatibility callback
        result = handle_request_compat(message, token, id, key);
    }
    return result;
}

ProtocolError Variables::handle_request(Message& message, token_t token, message_id_t id, const char* key) {
    // Allocate a context for the request
    std::unique_ptr<Context> ctx(new(std::nothrow) Context(this, token));
    if (!ctx) {
        return send_error_ack(message, token, id, CoAPCode::INTERNAL_SERVER_ERROR);
    }
    // Acknowledge the request
    const auto result = send_empty_ack(message, id);
    if (result != ProtocolError::NO_ERROR) {
        return result;
    }
    // Get the value asynchronously
    const auto& descriptor = protocol_->get_descriptor();
    descriptor.get_variable_async(key, get_variable_callback, ctx.release()); // Transfer the ownership over the context object
    return ProtocolError::NO_ERROR;
}

ProtocolError Variables::handle_request_compat(Message& message, token_t token, message_id_t id, const char* key) {
    const auto& descriptor = protocol_->get_descriptor();
    const auto value = descriptor.get_variable(key);
    if (!value) {
        return send_error_ack(message, token, id, CoAPCode::NOT_FOUND);
    }
    const auto value_type = descriptor.variable_type(key);
    size_t value_size = 0;
    switch (value_type) {
    case SparkReturnType::BOOLEAN: {
        value_size = sizeof(bool);
        break;
    }
    case SparkReturnType::INT: {
        value_size = sizeof(uint32_t);
        break;
    }
    case SparkReturnType::DOUBLE: {
        value_size = sizeof(double);
        break;
    }
    case SparkReturnType::STRING: {
        value_size = strlen((const char*)value);
        break;
    }
    default:
        // Unsupported variable type
        return send_error_ack(message, token, id, CoAPCode::INTERNAL_SERVER_ERROR);
    }
    // Acknowledge the request
    const auto result = send_empty_ack(message, id);
    if (result != ProtocolError::NO_ERROR) {
        return result;
    }
    // Send a separate response
    return send_response(token, value, value_size, value_type);
}

ProtocolError Variables::decode_request(Message& message, char* key) {
    uint8_t* queue = message.buf();
    uint8_t queue_offset = 8;
    // copy the variable key
    size_t key_length;
    if (queue[7] == 0x0d) {
        key_length = 0x0d + (queue[8] & 0xFF);
        queue_offset++;
    } else {
        key_length = queue[7] & 0x0F;
    }
    if (key_length > MAX_VARIABLE_KEY_LENGTH) {
        key_length = MAX_VARIABLE_KEY_LENGTH;
    }
    memcpy(key, queue + queue_offset, key_length);
    memset(key + key_length, 0, MAX_VARIABLE_KEY_LENGTH - key_length + 1);
    return ProtocolError::NO_ERROR;
}

ProtocolError Variables::encode_response(Message& message, token_t token, const void* value, size_t value_size,
        SparkReturnType::Enum value_type) {
    const auto max_value_size = protocol_->get_max_variable_value_size();
    if (value_size > max_value_size) {
        value_size = max_value_size; // Truncate the value data
    }
    size_t msg_size = 0;
    switch (value_type) {
    case SparkReturnType::BOOLEAN: {
        if (value_size < sizeof(bool)) {
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        const uint8_t v = *(const bool*)value ? 1 : 0;
        msg_size = encode_response(message.buf(), token, &v, sizeof(v));
        break;
    }
    case SparkReturnType::INT: {
        if (value_size < sizeof(uint32_t)) {
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        const uint32_t v = nativeToBigEndian(*(const uint32_t*)value);
        msg_size = encode_response(message.buf(), token, &v, sizeof(v));
        break;
    }
    case SparkReturnType::DOUBLE: {
        if (value_size < sizeof(double)) {
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        msg_size = encode_response(message.buf(), token, value, sizeof(double));
        break;
    }
    case SparkReturnType::STRING: {
        msg_size = encode_response(message.buf(), token, value, value_size);
        break;
    }
    default:
        return ProtocolError::NOT_IMPLEMENTED;
    }
    message.set_length(msg_size);
    return ProtocolError::NO_ERROR;
}

size_t Variables::encode_response(uint8_t* buffer, token_t token, const void* value, size_t value_size) {
    auto& channel = protocol_->get_channel();
    return Messages::separate_response_with_payload(buffer, 0 /* message_id */, token, CoAPCode::CONTENT,
            (const uint8_t*)value, value_size, channel.is_unreliable());
}

ProtocolError Variables::send_response(token_t token, const void* value, size_t value_size, SparkReturnType::Enum value_type) {
    Message msg;
    auto& channel = protocol_->get_channel();
    ProtocolError result = channel.create(msg);
    if (result != ProtocolError::NO_ERROR) {
        return result;
    }
    result = encode_response(msg, token, value, value_size, value_type);
    if (result != ProtocolError::NO_ERROR) {
        return send_error_response(msg, token, CoAPCode::INTERNAL_SERVER_ERROR);
    }
    return channel.send(msg);
}

ProtocolError Variables::send_error_response(token_t token, uint8_t code) {
    Message msg;
    auto& channel = protocol_->get_channel();
    ProtocolError result = channel.create(msg);
    if (result != ProtocolError::NO_ERROR) {
        return result;
    }
    return send_error_response(msg, token, code);
}

ProtocolError Variables::send_error_response(Message& message, token_t token, uint8_t code) {
    auto& channel = protocol_->get_channel();
    const size_t size = Messages::separate_response(message.buf(), 0 /* message_id */, token, code, channel.is_unreliable());
    message.set_length(size);
    return channel.send(message);
}

ProtocolError Variables::send_empty_ack(Message& message, message_id_t id) {
    const auto buf = message.buf();
    const size_t size = Messages::coded_ack(buf, CoAPCode::NONE, 0 /* message_id_msb */, 0 /* message_id_lsb */);
    message.set_length(size);
    message.set_id(id);
    return protocol_->get_channel().send(message);
}

ProtocolError Variables::send_error_ack(Message& message, token_t token, message_id_t id, uint8_t code) {
    const auto buf = message.buf();
    const size_t size = Messages::coded_ack(buf, token, code, 0 /* message_id_msb */, 0 /* message_id_lsb */);
    message.set_length(size);
    message.set_id(id);
    return protocol_->get_channel().send(message);
}

void Variables::get_variable_callback(int result, int type, void* data, size_t size, void* context) {
    const auto p = (Context*)context;
    if (result != ProtocolError::NO_ERROR) {
        const auto code = CoAP::codeForProtocolError((ProtocolError)result);
        p->self->send_error_response(p->token, code);
    } else {
        p->self->send_response(p->token, data, size, (SparkReturnType::Enum)type);
    }
    free(data);
    delete p;
}

} // namespace protocol

} // namespace particle
