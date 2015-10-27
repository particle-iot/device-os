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

namespace particle
{
namespace protocol
{


class Message
{
	uint8_t* buffer;
	size_t buffer_length;
	size_t message_length;

public:

	Message(uint8_t* buf, size_t buflen, size_t msglen) : buffer(buf), buffer_length(buflen), message_length(msglen) {}
	size_t max_size() const { return buffer_length; }
	uint8_t* buf() const { return buffer; }
	size_t length() const { return message_length; }
};

/**
 * A message channel represents a way to send and receive messages with an endpoint.
 */
class MessageChannel
{

public:
	int establish();

	/**
	 * Fetch the next message from the channel.
	 * If no message is ready, a message of size 0 is returned.
	 *
	 * @return an error value !=0 on error.
	 */
	int next_message(Message& message);

	/**
	 * Send the given message to the endpoint
	 * @return an error value !=0 on error.
	 */
	int send(Message& msg);
};


}}
