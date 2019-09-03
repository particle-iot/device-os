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

#if HAL_PLATFORM_CLOUD_UDP

#include <string.h>
#include "protocol_defs.h"
#include "message_channel.h"
#include "messages.h"
#include "spark_descriptor.h"
#include "protocol.h"
#include "dtls_message_channel.h"
#include "coap_channel.h"
#include "eckeygen.h"
#include <limits>
#include "logging.h"

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
	static const unsigned DEFAULT_DISCONNECT_COMMAND_TIMEOUT = 60000;

    // todo - this a duplicate of LightSSLProtocol - factor out

	DTLSProtocol() : Protocol(channel) {}

	void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor) override;

	size_t build_hello(Message& message, uint8_t flags) override
	{
		product_details_t deets;
		deets.size = sizeof(deets);
		get_product_details(deets);
		size_t len = Messages::hello(message.buf(), 0,
				flags, PLATFORM_ID, deets.product_id,
				deets.product_version, true,
				device_id, sizeof(device_id));
		return len;
	}

	virtual int command(ProtocolCommands::Enum command, uint32_t value, const void* param) override
	{
		switch (command)
		{
		case ProtocolCommands::SLEEP:
		case ProtocolCommands::DISCONNECT: {
			int r = ProtocolError::NO_ERROR;
			unsigned timeout = DEFAULT_DISCONNECT_COMMAND_TIMEOUT;
			if (param) {
				const auto p = (const spark_disconnect_command*)param;
				if (p->timeout != 0) {
					timeout = p->timeout;
				}
				if (p->disconnect_reason != CLOUD_DISCONNECT_REASON_NONE) {
					r = send_goodbye((cloud_disconnect_reason)p->disconnect_reason, (system_reset_reason)p->reset_reason,
							p->sleep_duration);
				}
			}
			if (r == ProtocolError::NO_ERROR) {
				r = wait_confirmable(timeout);
			}
			ack_handlers.clear();
			return r;
		}
		case ProtocolCommands::TERMINATE: {
			// TODO: Clear pending messages and reset the pinger
			ack_handlers.clear();
			return ProtocolError::NO_ERROR;
		}
		case ProtocolCommands::WAKE:
		case ProtocolCommands::PING: {
			int r = ProtocolError::NO_ERROR;
			if (!pinger.is_expecting_ping_ack()) {
				LOG(INFO, "Forcing a cloud ping");
				r = pinger.process(std::numeric_limits<system_tick_t>::max(), [this] {
					return ping(true);
				});
			}
			return r;
		}
		default:
			return ProtocolError::UNKNOWN;
		}
	}

	int get_status(protocol_status* status) const override {
		SPARK_ASSERT(status);
		status->flags = 0;
		if (channel.has_unacknowledged_client_requests()) {
			status->flags |= PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES;
		}
		return NO_ERROR;
	}


	/**
	 * Ensures that all outstanding sent coap messages have been acknowledged.
	 */
	int wait_confirmable(uint32_t timeout);
};



}}

#endif // HAL_PLATFORM_CLOUD_UDP
