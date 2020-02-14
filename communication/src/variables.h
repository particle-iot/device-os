/**
 ******************************************************************************
 Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#pragma once

#include "spark_descriptor.h"
#include "protocol_defs.h"
#include "coap.h"

#include <cstddef>

namespace particle {

namespace protocol {

class Protocol;
class Message;

class Variables
{
public:
    explicit Variables(Protocol* protocol);

    ProtocolError handle_request(Message& message, token_t token, message_id_t id);

private:
    struct Context;

    Protocol* protocol_;

    ProtocolError handle_request(Message& message, token_t token, message_id_t id, const char* key);
    ProtocolError handle_request_compat(Message& message, token_t token, message_id_t id, const char* key);

    ProtocolError decode_request(Message& message, char* key);
    ProtocolError encode_response(Message& message, token_t token, const void* value, size_t value_size, SparkReturnType::Enum value_type);
    size_t encode_response(uint8_t* buffer, token_t token, const void* value, size_t value_size);

    ProtocolError send_response(token_t token, const void* value, size_t value_size, SparkReturnType::Enum value_type);
    ProtocolError send_error_response(token_t token, uint8_t code);
    ProtocolError send_error_response(Message& message, token_t token, uint8_t code);

    ProtocolError send_empty_ack(Message& message, message_id_t id);
    ProtocolError send_error_ack(Message& message, token_t token, message_id_t id, uint8_t code);

    static void get_variable_callback(int result, int type, void* data, size_t size, void* context); // SparkDescriptor::GetVariableCallback
};

inline Variables::Variables(Protocol* protocol) :
        protocol_(protocol) {
}

} // namespace protocol

} // namespace particle
