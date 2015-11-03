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
#include "protocol.h"
#include "lightssl_message_channel.h"
#include "coap_channel.h"

namespace particle {
namespace protocol {

class LightSSLProtocol : public Protocol
{
	CoAPChannel<LightSSLMessageChannel> channel;
	bool initialized;

	static void handle_seed(const uint8_t* data, size_t len)
	{

	}

public:

	LightSSLProtocol() : Protocol(channel), initialized(false) {}

	void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor)
	{
		LightSSLMessageChannel::Callbacks channelCallbacks;
		channelCallbacks.millis = callbacks.millis;
		channelCallbacks.handle_seed = handle_seed;
		channelCallbacks.receive = callbacks.receive;
		channelCallbacks.send = callbacks.send;
		channel.init(keys.core_private, keys.server_public, (const uint8_t*)id, channelCallbacks);
        Protocol::init(callbacks, descriptor);
		initialized = true;
	}

	bool is_initialized() { return initialized; }

	int presence_announcement(uint8_t* buf, const uint8_t* id)
	{
		return -1;
	}


};



}}
