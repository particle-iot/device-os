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

template<size_t max, size_t prefix=0, size_t suffix=0>
class BufferMessageChannel : public AbstractMessageChannel
{
protected:
    unsigned char queue[max];

public:

	virtual ProtocolError create(Message& message, size_t minimum_size=0) override
	{
		if (minimum_size>sizeof(queue)-prefix-suffix) {
            WARN("Insufficient storage for message size %d", minimum_size);
			return INSUFFICIENT_STORAGE;
        }
		message.clear();
		message.set_buffer(queue+prefix, sizeof(queue)-suffix-prefix);
		message.set_length(0);
		return NO_ERROR;
	}

	/**
	 * Fill out a message struct to contain storage for a response.
	 */
	ProtocolError response(Message& original, Message& response, size_t required) override
	{
		return original.splinter(response, required, prefix+suffix) ? NO_ERROR : INSUFFICIENT_STORAGE;
	}

};

}}
