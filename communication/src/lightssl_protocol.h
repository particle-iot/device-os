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

#include "protocol_selector.h"

#if HAL_PLATFORM_CLOUD_TCP && PARTICLE_PROTOCOL

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

	static void handle_seed(const uint8_t* data, size_t len)
	{

	}

public:

	LightSSLProtocol() : Protocol(channel) {}

	void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor) override
	{
		set_protocol_flags(REQUIRE_HELLO_RESPONSE);

		LightSSLMessageChannel::Callbacks channelCallbacks;
		channelCallbacks.millis = callbacks.millis;
		channelCallbacks.handle_seed = handle_seed;
		channelCallbacks.receive = callbacks.receive;
		channelCallbacks.send = callbacks.send;
		channel.init(keys.core_private, keys.server_public, (const uint8_t*)id, channelCallbacks, &channel.next_id_ref());
        Protocol::init(callbacks, descriptor);
        initialize_ping(15000,10000);
	}

	size_t build_hello(Message& message, uint8_t flags) override
	{
		product_details_t deets;
		deets.size = sizeof(deets);
		get_product_details(deets);

		size_t len = Messages::hello(message.buf(), 0,
				flags, PLATFORM_ID, deets.product_id,
				deets.product_version, false, nullptr, 0);
		return len;
	}

	virtual int command(ProtocolCommands::Enum command, uint32_t data) override;

	int wait_confirmable(uint32_t timeout=5000);

};



}}

#endif // HAL_PLATFORM_CLOUD_TCP && PARTICLE_PROTOCOL
