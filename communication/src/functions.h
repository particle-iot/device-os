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

#include <string.h>
#include "protocol_defs.h"
#include "message_channel.h"
#include "messages.h"
#include "spark_descriptor.h"


namespace particle
{
namespace protocol
{

class Functions
{
    char function_arg[MAX_FUNCTION_ARG_LENGTH+1]; // add one for null terminator

    ProtocolError function_result(MessageChannel& channel, const void* result, SparkReturnType::Enum, token_t token)
    {
        Message message;
        channel.create(message, Messages::function_return_size);
        size_t length = Messages::function_return(message.buf(), 0, token, long(result), channel.is_unreliable());
        message.set_length(length);
        return channel.send(message);
    }

public:
    ProtocolError handle_function_call(token_t token, message_id_t message_id, Message& message, MessageChannel& channel,
            int (*call_function)(const char *function_key, const char *arg, SparkDescriptor::FunctionResultCallback callback, void* reserved))
    {
        // copy the function key
        char function_key[MAX_FUNCTION_KEY_LENGTH+1]; // add one for null terminator
        memset(function_key, 0, sizeof(function_key));
        uint8_t* queue = message.buf();
        uint8_t queue_offset = 8;
        size_t function_key_length = queue[7] & 0x0F;
        if (function_key_length == MAX_OPTION_DELTA_LENGTH+1)
        {
            function_key_length = MAX_OPTION_DELTA_LENGTH+1 + queue[8];
            queue_offset++;
        }
        // else if (function_key_length == MAX_OPTION_DELTA_LENGTH+2)
        // {
        //     // MAX_OPTION_DELTA_LENGTH+2 not supported and not required for function_key_length
        // }
        // allocated memory bounds check
        if (function_key_length > MAX_FUNCTION_KEY_LENGTH)
        {
            function_key_length = MAX_FUNCTION_KEY_LENGTH;
            // already memset to 0 (null terminator padded to end)
        }
        memcpy(function_key, queue + queue_offset, function_key_length);

        // How long is the argument?
        size_t q_index = queue_offset + function_key_length;
        size_t function_arg_length = queue[q_index] & 0x0F;
        if (function_arg_length == MAX_OPTION_DELTA_LENGTH+1)
        {
            ++q_index;
            function_arg_length = MAX_OPTION_DELTA_LENGTH+1 + queue[q_index];
        }
        else if (function_arg_length == MAX_OPTION_DELTA_LENGTH+2)
        {
            ++q_index;
            function_arg_length = queue[q_index] << 8;
            ++q_index;
            function_arg_length |= queue[q_index];
            function_arg_length += 269;
        }

        bool has_function = true;
        // allocated memory bounds check
        if (function_arg_length > MAX_FUNCTION_ARG_LENGTH)
        {
            function_arg_length = MAX_FUNCTION_ARG_LENGTH;
            has_function = false;
            // in case we got here due to inconceivable error, memset with null terminators
            memset(function_arg, 0, sizeof(function_arg));
        }
        // save a copy of the argument
        memcpy(function_arg, queue + q_index + 1, function_arg_length);
        function_arg[function_arg_length] = 0; // null terminate string

        Message response;
        channel.response(message, response, 16);
        // send ACK
        size_t response_length = Messages::coded_ack(response.buf(), has_function ? 0x00 : RESPONSE_CODE(4,00), 0, 0);
        response.set_id(message_id);
        response.set_length(response_length);
        ProtocolError error = channel.send(response);
        if (error) {
            return error;
        }

        // call the given user function
        auto callback = [=,&channel] (const void* result, SparkReturnType::Enum resultType )
            { return this->function_result(channel, result, resultType, token); };
        call_function(function_key, function_arg, callback, NULL);
        return NO_ERROR;
    }
};


}}
