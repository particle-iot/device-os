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

#include "service_debug.h"
#include "device_keys.h"
#include "message_channel.h"
#include "buffer_message_channel.h"

namespace particle
{
namespace protocol
{

/**
 * This implements the lightweight and RSA encrypted handshake, AES session encryption over a TCP Stream.
 *
 * The buffer provided to the message starts at offset 2 to allow a 2-byte length to be added.
 * The buffer length extends to the maximum capacity minus 16 so there is room for PKCS#1v5 padding.
 */
class DTLSMessageChannel: public BufferMessageChannel<PROTOCOL_BUFFER_SIZE>
{


public:

	struct Callbacks
	{
		system_tick_t (*millis)();
		void (*handle_seed)(const uint8_t* seed, size_t length);
		int (*send)(const unsigned char *buf, uint32_t buflen, void* handle);
		int (*receive)(unsigned char *buf, uint32_t buflen, void* handle);
	};


	void init(const uint8_t* core_private, const uint8_t* server_public,
			const uint8_t* device_id, Callbacks& callbacks);

	virtual ProtocolError establish() override
	{
        return NO_ERROR;
	}

	/**
	 * Retrieve first the 2 byte length from the stream, which determines
	 */
	ProtocolError receive(Message& message) override
    {
        return NO_ERROR;
    }

	/**
	 * Sends the given message. The message length is prepended to the message
	 * and the message padded with PKCS#1 padding before being sent using
	 * the send callback.
	 */
	ProtocolError send(Message& message) override
    {
        return NO_ERROR;
    }


};


}}