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

#include <cstdint>
#include <cstddef>
#include "protocol_defs.h"

namespace particle
{
namespace protocol
{


class Message
{
	friend class AbstractMessageChannel;

	uint8_t* buffer;
	size_t buffer_length;
	size_t message_length;

	size_t trim_capacity()
	{
		size_t trimmed = buffer_length-message_length;
		buffer_length = message_length;
		return trimmed;
	}

	size_t buffer_available() const { return buffer_length-message_length; }

	bool splinter(Message& target, size_t size_required)
	{
		size_t available = buffer_available();
		if (available<size_required)
			return false;

		int excess = trim_capacity();
		target.set_buffer(buf()+length(), excess);
		return true;
	}

public:
	Message() : Message(nullptr, 0, 0) {}

	Message(uint8_t* buf, size_t buflen, size_t msglen) : buffer(buf), buffer_length(buflen), message_length(msglen) {}
	size_t capacity() const { return buffer_length; }
	uint8_t* buf() const { return buffer; }
	size_t length() const { return message_length; }

	void set_length(size_t length) { if (length<=buffer_length) message_length = length; }
	void set_buffer(uint8_t* buffer, size_t length) { this->buffer = buffer; buffer_length = length; message_length = 0; }
};

/**
 * A message channel represents a way to send and receive messages with an endpoint.
 *
 * Note that the implementation may use a shared message buffer for all
 * message operations. The only operation that does not invalidate an existing
 * message is MessageChannel::response() since this allocates the new message at the end of the existing one.
 *
 */
class MessageChannel
{

public:
	virtual ~MessageChannel() {}

	virtual ProtocolError establish();

	/**
	 * Retrieves a new message object containing the message buffer.
	 */
	virtual ProtocolError create(Message& message, size_t minimum_size=0);

	/**
	 * Fetch the next message from the channel.
	 * If no message is ready, a message of size 0 is returned.
	 *
	 * @return an error value !=0 on error.
	 */
	virtual ProtocolError receive(Message& message);

	/**
	 * Send the given message to the endpoint
	 * @return an error value !=0 on error.
	 */
	virtual ProtocolError send(Message& msg);

	/**
	 * Fill out a message struct to contain storage for a response.
	 */
	virtual ProtocolError response(Message& original, Message& response, size_t required);
};

class AbstractMessageChannel : public MessageChannel
{

public:

	/**
	 * Fill out a message struct to contain storage for a response.
	 */
	ProtocolError response(Message& original, Message& response, size_t required) override
	{
		return original.splinter(response, required) ? NO_ERROR : INSUFFICIENT_STORAGE;
	}

};


}}
