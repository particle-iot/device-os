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

#include <cstring>
#include <cstdint>

#include "protocol_defs.h"
#include "events.h"
#include "message_channel.h"
#include "spark_descriptor.h"

#include "spark_wiring_vector.h"

namespace particle
{
namespace protocol
{

class Subscriptions
{
public:
	typedef uint32_t (*calculate_crc_fn)(const unsigned char *buf, uint32_t buflen);

private:
	FilteringEventHandler event_handlers[MAX_SUBSCRIPTIONS];
	Vector<message_handle_t> subscription_msg_ids;

protected:
	ProtocolError send_subscription_impl(MessageChannel& channel, const char* filter, size_t filter_len, int flags);

public:

	Subscriptions()
	{
		memset(&event_handlers, 0, sizeof(event_handlers));
	}

	uint32_t compute_subscriptions_checksum(calculate_crc_fn calculate_crc)
	{
		uint32_t checksum = 0;
		for_each([&checksum, calculate_crc](FilteringEventHandler& handler){
			uint32_t chk[3];
			chk[0] = checksum;
			chk[1] = calculate_crc((const uint8_t*)handler.filter, sizeof(handler.filter));
			chk[2] = calculate_crc((const uint8_t*)&handler.flags, sizeof(handler.flags));
			checksum = calculate_crc((const uint8_t*)chk, sizeof(chk));
			return NO_ERROR;
		});
		return checksum;
	}

	ProtocolError handle_event(Message& message, SparkDescriptor::CallEventHandlerCallback callback, MessageChannel& channel, bool& handled);

	template<typename F> ProtocolError for_each(F callback)
	{
		const int NUM_HANDLERS = sizeof(event_handlers)
				/ sizeof(FilteringEventHandler);
		ProtocolError error = NO_ERROR;
		for (unsigned i = 0; i < NUM_HANDLERS; i++)
		{
			if (nullptr != event_handlers[i].handler || (event_handlers[i].flags & SubscriptionFlag::LARGE_EVENT))
			{
				error = callback(event_handlers[i]);
				if (error)
					break;
			}
		}
		return error;
	}

	void remove_event_handlers(const char* event_name)
	{
		if (NULL == event_name)
		{
			memset(event_handlers, 0, sizeof(event_handlers));
		}
		else
		{
			const int NUM_HANDLERS = sizeof(event_handlers)
					/ sizeof(FilteringEventHandler);
			int dest = 0;
			for (int i = 0; i < NUM_HANDLERS; i++)
			{
				if (!strncmp(event_name, event_handlers[i].filter, sizeof(event_handlers[i].filter)))
				{
					memset(&event_handlers[i], 0, sizeof(event_handlers[i]));
				}
				else
				{
					if (dest != i)
					{
						memcpy(event_handlers + dest, event_handlers + i,
								sizeof(event_handlers[i]));
						memset(event_handlers + i, 0,
								sizeof(event_handlers[i]));
					}
					dest++;
				}
			}
		}
	}

	/**
	 * Determines if the given handler exists.
	 */
	bool event_handler_exists(const char *event_name, EventHandler handler, void *handler_data, int flags)
	{
		const int NUM_HANDLERS = sizeof(event_handlers)
				/ sizeof(FilteringEventHandler);
		for (int i = 0; i < NUM_HANDLERS; i++)
		{
			// XXX: For a subscription registered via the new event API, simply look for a full name
			// match. The existing logic for the classic API doesn't look intentional but let's keep
			// it for backward compatibility
			if (flags & SubscriptionFlag::LARGE_EVENT)
			{
				if (strncmp(event_name, event_handlers[i].filter, sizeof(event_handlers[i].filter)) == 0) {
					return true;
				}
			}
			else if (event_handlers[i].handler == handler && event_handlers[i].handler_data == handler_data)
			{
				const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
				const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
				if (!strncmp(event_handlers[i].filter, event_name, FILTER_LEN))
				{
					return true;
				}
			}
		}
		return false;
	}

	/**
	 * Adds the given handler.
	 */
	ProtocolError add_event_handler(const char *event_name, EventHandler handler, void *handler_data, int flags)
	{
		if (event_handler_exists(event_name, handler, handler_data, flags))
			return NO_ERROR;

		const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
		for (int i = 0; i < NUM_HANDLERS; i++)
		{
			if (!event_handlers[i].handler && !(event_handlers[i].flags & SubscriptionFlag::LARGE_EVENT))
			{
				const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
				const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
				memcpy(event_handlers[i].filter, event_name, FILTER_LEN);
				memset(event_handlers[i].filter + FILTER_LEN, 0, MAX_FILTER_LEN - FILTER_LEN);
				event_handlers[i].handler = handler;
				event_handlers[i].handler_data = handler_data;
				event_handlers[i].flags = flags;
				return NO_ERROR;
			}
		}
		return INSUFFICIENT_STORAGE;
	}

	ProtocolError send_subscriptions(MessageChannel& channel)
	{
		subscription_msg_ids.clear();
		ProtocolError result = for_each([&](const FilteringEventHandler& handler) {
			return send_subscription_impl(channel, handler.filter, strnlen(handler.filter, sizeof(handler.filter)), handler.flags);
		});
		return result;
	}

	ProtocolError send_subscription(MessageChannel& channel, const char* filter, int flags)
	{
		subscription_msg_ids.clear();
		return send_subscription_impl(channel, filter, std::strlen(filter), flags);
	}

	const Vector<message_handle_t>& subscription_message_ids() const
	{
		return subscription_msg_ids;
	}
};

} // namespace protocol

} // namespace particle
