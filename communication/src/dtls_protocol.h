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
#include "dtls_message_channel.h"
#include "coap_channel.h"
#include "eckeygen.h"

namespace particle {
namespace protocol {


class DTLSProtocol : public Protocol
{
	CoAPChannel<CoAPReliableChannel<DTLSMessageChannel, decltype(SparkCallbacks::millis)>> channel;

	static void handle_seed(const uint8_t* data, size_t len)
	{

	}

	uint8_t device_id[12];

public:
    // todo - this a duplicate of LightSSLProtocol - factor out

	DTLSProtocol() : Protocol(channel) {}

	void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor) override;

	size_t build_hello(Message& message, bool ota_updated) override
	{
		product_details_t deets;
		deets.size = sizeof(deets);
		get_product_details(deets);
		size_t len = Messages::hello(message.buf(), 0,
				ota_updated, PLATFORM_ID, deets.product_id,
				deets.product_version, true,
				device_id, sizeof(device_id));
		return len;
	}

	virtual void command(ProtocolCommands::Enum command, uint32_t data) override
	{
		switch (command)
		{
		case ProtocolCommands::SLEEP:
			sleep();
			break;
		case ProtocolCommands::WAKE:
			wake();
			break;
		}
	}



	/**
	 * Ensures that all outstanding sent coap messages have been acknowledged.
	 */
	void sleep(uint32_t timeout=60000);

	void wake()
	{
		ping();
	}
};



}}
