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
    char function_arg[MAX_FUNCTION_ARG_LENGTH];

    ProtocolError function_result(MessageChannel& channel, const void* result, SparkReturnType::Enum, token_t token)
    {
    		Message message;
    		channel.create(message, Messages::function_return_size);
        size_t length = Messages::function_return(message.buf(), 0, token, long(result));
        message.set_length(length);
        return channel.send(message);
    }

public:
	ProtocolError handle_function_call(int token, Message& message, MessageChannel& channel,
		    int (*call_function)(const char *function_key, const char *arg, SparkDescriptor::FunctionResultCallback callback, void* reserved))
	{
	    // copy the function key
	    char function_key[13];
	    memset(function_key, 0, 13);
	    uint8_t* queue = message.buf();
	    int function_key_length = queue[7] & 0x0F;
	    memcpy(function_key, queue + 8, function_key_length);

	    // How long is the argument?
	    int q_index = 8 + function_key_length;
	    int query_length = queue[q_index] & 0x0F;
	    if (13 == query_length)
	    {
	      ++q_index;
	      query_length = 13 + queue[q_index];
	    }
	    else if (14 == query_length)
	    {
	      ++q_index;
	      query_length = queue[q_index] << 8;
	      ++q_index;
	      query_length |= queue[q_index];
	      query_length += 269;
	    }

	    bool has_function = false;

	    // allocated memory bounds check
	    if (MAX_FUNCTION_ARG_LENGTH > query_length)
	    {
	        // save a copy of the argument
	        memcpy(function_arg, queue + q_index + 1, query_length);
	        function_arg[query_length] = 0; // null terminate string
	        has_function = true;
	    }

	    Message response;
	    channel.response(message, response, 16);
	    // send ACK
	    size_t response_length = Messages::coded_ack(response.buf(), has_function ? 0x00 : RESPONSE_CODE(4,00), queue[2], queue[3]);
	    response.set_length(response_length);
	    ProtocolError error = channel.send(response);
	    if (error) return error;

	    // call the given user function
	    auto callback = [=,&channel] (const void* result, SparkReturnType::Enum resultType )
	    		{ return this->function_result(channel, result, resultType, token); };
	    call_function(function_key, function_arg, callback, NULL);
	    return NO_ERROR;
	}
};


}}
