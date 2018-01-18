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

class Variables
{


public:

    ProtocolError decode_variable_request(char variable_key[MAX_VARIABLE_KEY_LENGTH+1], Message& message)
    {
        uint8_t* queue = message.buf();
        uint8_t queue_offset = 8;

        // copy the variable key
        size_t variable_key_length;
        if (queue[7] == 0x0d) {
            variable_key_length = 0x0d + (queue[8] & 0xFF);
            queue_offset++;
        } else {
            variable_key_length = queue[7] & 0x0F;
        }
        if (variable_key_length > MAX_VARIABLE_KEY_LENGTH) {
            variable_key_length = MAX_VARIABLE_KEY_LENGTH;
        }

        memcpy(variable_key, queue + queue_offset, variable_key_length);
        memset(variable_key + variable_key_length, 0, MAX_VARIABLE_KEY_LENGTH+1 - variable_key_length);
        return NO_ERROR;
    }

    ProtocolError handle_variable_request(char* variable_key, Message& message, MessageChannel& channel, token_t token, message_id_t message_id,
        SparkReturnType::Enum (*variable_type)(const char *variable_key),
        const void *(*get_variable)(const char *variable_key))
    {
        uint8_t* queue = message.buf();
        message.set_id(message_id);
        // get variable value according to type using the descriptor
        SparkReturnType::Enum var_type = variable_type(variable_key);
        size_t response = 0;

        if(SparkReturnType::BOOLEAN == var_type)
        {
            const bool *bool_val = (const bool *)get_variable(variable_key);
            response = Messages::variable_value(queue, message_id, token, *bool_val);
        }
        else if(SparkReturnType::INT == var_type)
        {
            const int *int_val = (const int *)get_variable(variable_key);
            response = Messages::variable_value(queue, message_id, token, *int_val);
        }
        else if(SparkReturnType::STRING == var_type)
        {
            const char *str_val = (const char *)get_variable(variable_key);

            // 2-byte leading length, 16 potential padding bytes
            int max_length = message.capacity();
            int str_length = strlen(str_val);
            if (str_length > max_length) {
                str_length = max_length;
            }
            response = Messages::variable_value(queue, message_id, token, str_val, str_length);
        }
        else if(SparkReturnType::DOUBLE == var_type)
        {
            double *double_val = (double *)get_variable(variable_key);
            response = Messages::variable_value(queue, message_id, token, *double_val);
        }

        message.set_length(response);
        return channel.send(message);
    }
};


}}
