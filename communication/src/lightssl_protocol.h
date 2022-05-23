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

#if HAL_PLATFORM_CLOUD_TCP

#include <string.h>
#include "protocol_defs.h"
#include "message_channel.h"
#include "messages.h"
#include "spark_descriptor.h"
#include "protocol.h"
#include "lightssl_message_channel.h"
#include "coap_channel.h"
#include "mbedtls_util.h"

namespace particle {
namespace protocol {

class LightSSLProtocol : public Protocol
{
	CoAPChannel<LightSSLMessageChannel> channel;

	static void handle_seed(const uint8_t* data, size_t len)
	{

	}

public:
	static const unsigned DEFAULT_DISCONNECT_COMMAND_TIMEOUT = 5000;

	LightSSLProtocol() : Protocol(channel) {}

	void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor) override
	{
		set_protocol_flags(REQUIRE_HELLO_RESPONSE);
		mbedtls_default_rng(nullptr, &next_token, sizeof(next_token));
		LightSSLMessageChannel::Callbacks channelCallbacks;
		channelCallbacks.millis = callbacks.millis;
		channelCallbacks.handle_seed = handle_seed;
		channelCallbacks.receive = callbacks.receive;
		channelCallbacks.send = callbacks.send;
		channel.init(keys.core_private, keys.server_public, (const uint8_t*)id, channelCallbacks, &channel.next_id_ref());
        Protocol::init(callbacks, descriptor);
        initialize_ping(15000,10000);
	}

	size_t build_hello(Message& message, uint16_t flags) override
	{
		product_details_t deets;
		deets.size = sizeof(deets);
		get_product_details(deets);

		size_t len = Messages::hello(message.buf(), 0 /* message_id */, flags, PLATFORM_ID, system_version,
				deets.product_id, deets.product_version, nullptr /* device_id */, 0 /* device_id_len */,
				get_max_transmit_message_size(), max_binary_size, ota_chunk_size, false /* confirmable */);
		return len;
	}

	virtual int command(ProtocolCommands::Enum command, uint32_t value, const void* data) override;

	virtual int get_status(protocol_status* status) const override
	{
		SPARK_ASSERT(status);
		status->flags = 0;
		return 0;
	}

	int wait_confirmable(uint32_t timeout);

};



}}

#endif // HAL_PLATFORM_CLOUD_TCP
