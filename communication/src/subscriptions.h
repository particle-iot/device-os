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

namespace particle
{
namespace protocol
{

#include "protocol_defs.h"
#include "events.h"
#include "message_channel.h"
#include <stdint.h>

class Subscriptions
{
public:
	typedef uint32_t (*calculate_crc_fn)(const unsigned char *buf, uint32_t buflen);

private:
	FilteringEventHandler event_handlers[MAX_SUBSCRIPTIONS];

protected:

	ProtocolError send_subscription(MessageChannel& channel, const char* filter, const char* device_id, SubscriptionScope::Enum scope)
	{
	    size_t msglen;
	    Message message;
	    channel.create(message);
        if (device_id)
       	  msglen = subscription(message.buf(), 0, filter, device_id);
        else
          msglen = subscription(message.buf(), 0, filter, scope);
        message.set_length(msglen);
        ProtocolError result = channel.send(message);
        return result;
	}

	inline ProtocolError send_subscription(MessageChannel& channel, const FilteringEventHandler& handler)
	{
		return send_subscription(channel, handler.filter, handler.device_id[0] ? handler.device_id : nullptr, handler.scope);
	}

public:

	Subscriptions()
	{
		memset(&event_handlers, 0, sizeof(event_handlers));
	}

	uint32_t compute_subscriptions_checksum(calculate_crc_fn calculate_crc)
	{
		uint32_t checksum = 0;
		for_each([&checksum, calculate_crc](FilteringEventHandler& handler){
			uint32_t chk[4];
			chk[0] = checksum;
			chk[1] = calculate_crc((const uint8_t*)handler.device_id, sizeof(handler.device_id));
			chk[2] = calculate_crc((const uint8_t*)handler.filter, sizeof(handler.filter));
			chk[3] = calculate_crc((const uint8_t*)&handler.scope, sizeof(handler.scope));
			checksum = calculate_crc((const uint8_t*)chk, sizeof(chk));
			return NO_ERROR;
		});
		return checksum;
	}

	ProtocolError handle_event(Message& message,
			void (*call_event_handler)(uint16_t size,
					FilteringEventHandler* handler, const char* event,
					const char* data, void* reserved),
					MessageChannel& channel)
	{
		const unsigned len = message.length();
		uint8_t* queue = message.buf();
		if (CoAP::type(queue)==CoAPType::CON && channel.is_unreliable())
		{
			Message response;
			if (channel.response(message, response, 5)==NO_ERROR)
			{
				size_t len = Messages::empty_ack(response.buf(), 0, 0);
				response.set_length(len);
				response.set_id(message.get_id());
				ProtocolError error = channel.send(response);
				if (error)
					return error;
			}
		}

		// end of CoAP message
		unsigned char *end = queue + len;
		// start of event name option (location path) - 6 bytes
		// 4 bytes coap header, 2 bytes for the location path of the message
		// plus the size of the token.
		unsigned char *event_name = queue + 6 + (queue[0] & 0xF);
		size_t event_name_length = CoAP::option_decode(&event_name);
		if (0 == event_name_length)
		{
			// error, malformed CoAP option
			return MALFORMED_MESSAGE;
		}

		unsigned char *next_src = event_name + event_name_length;
		unsigned char *next_dst = next_src;
		while (next_src < end && 0x00 == (*next_src & 0xf0))
		{
			// there's another Uri-Path option, i.e., event name with slashes
			size_t option_len = CoAP::option_decode(&next_src);
			*next_dst++ = '/';
			if (next_dst != next_src)
			{
				// at least one extra byte has been used to encode a CoAP Uri-Path option length
				memmove(next_dst, next_src, option_len);
			}
			next_src += option_len;
			next_dst += option_len;
		}
		event_name_length = next_dst - event_name;

		if (next_src < end && 0x30 == (*next_src & 0xf0))
		{
			// Max-Age option is next, which we ignore
			size_t next_len = CoAP::option_decode(&next_src);
			next_src += next_len;
		}

		unsigned char *data = NULL;
		if (next_src < end && 0xff == *next_src)
		{
			// payload is next
			data = next_src + 1;
			// null terminate data string
			*end = 0;
		}
		// null terminate event name string
		event_name[event_name_length] = 0;

		const int NUM_HANDLERS = sizeof(event_handlers)
				/ sizeof(FilteringEventHandler);
		for (int i = 0; i < NUM_HANDLERS; i++)
		{
			if (NULL == event_handlers[i].handler)
			{
				break;
			}
			const size_t MAX_FILTER_LENGTH = sizeof(event_handlers[i].filter);
			const size_t filter_length = strnlen(event_handlers[i].filter,
					MAX_FILTER_LENGTH);

			if (event_name_length < filter_length)
			{
				// does not match this filter, try the next event handler
				continue;
			}

			const int cmp = memcmp(event_handlers[i].filter, event_name,
					filter_length);
			if (0 == cmp)
			{
				// don't call the handler directly, use a callback for it.
				if (!call_event_handler)
				{
					if (event_handlers[i].handler_data)
					{
						EventHandlerWithData handler =
								(EventHandlerWithData) event_handlers[i].handler;
						handler(event_handlers[i].handler_data,
								(char *) event_name, (char *) data);
					}
					else
					{
						event_handlers[i].handler((char *) event_name,
								(char *) data);
					}
				}
				else
				{
					call_event_handler(sizeof(FilteringEventHandler),
							&event_handlers[i], (const char*) event_name,
							(const char*) data, NULL);
				}
			}
			// else continue the for loop to try the next handler
		}
		return NO_ERROR;
	}

	template<typename F> ProtocolError for_each(F callback)
	{
		const int NUM_HANDLERS = sizeof(event_handlers)
				/ sizeof(FilteringEventHandler);
		ProtocolError error = NO_ERROR;
		for (unsigned i = 0; i < NUM_HANDLERS; i++)
		{
			if (nullptr != event_handlers[i].handler)
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
				if (!strcmp(event_name, event_handlers[i].filter))
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
	bool event_handler_exists(const char *event_name, EventHandler handler,
			void *handler_data, SubscriptionScope::Enum scope, const char* id)
	{
		const int NUM_HANDLERS = sizeof(event_handlers)
				/ sizeof(FilteringEventHandler);
		for (int i = 0; i < NUM_HANDLERS; i++)
		{
			if (event_handlers[i].handler == handler
					&& event_handlers[i].handler_data == handler_data
					&& event_handlers[i].scope == scope)
			{
				const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
				const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
				if (!strncmp(event_handlers[i].filter, event_name, FILTER_LEN))
				{
					const size_t MAX_ID_LEN =
							sizeof(event_handlers[i].device_id) - 1;
					const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
					if (id_len)
						return !strncmp(event_handlers[i].device_id, id, id_len);
					else
						return !event_handlers[i].device_id[0];
				}
			}
		}
		return false;
	}

	/**
	 * Adds the given handler.
	 */
	ProtocolError add_event_handler(const char *event_name, EventHandler handler,
			void *handler_data, SubscriptionScope::Enum scope, const char* id)
	{
		if (event_handler_exists(event_name, handler, handler_data, scope, id))
			return NO_ERROR;

		const int NUM_HANDLERS = sizeof(event_handlers) / sizeof(FilteringEventHandler);
		for (int i = 0; i < NUM_HANDLERS; i++)
		{
			if (NULL == event_handlers[i].handler)
			{
				const size_t MAX_FILTER_LEN = sizeof(event_handlers[i].filter);
				const size_t FILTER_LEN = strnlen(event_name, MAX_FILTER_LEN);
				memcpy(event_handlers[i].filter, event_name, FILTER_LEN);
				memset(event_handlers[i].filter + FILTER_LEN, 0, MAX_FILTER_LEN - FILTER_LEN);
				event_handlers[i].handler = handler;
				event_handlers[i].handler_data = handler_data;
				event_handlers[i].device_id[0] = 0;
				const size_t MAX_ID_LEN = sizeof(event_handlers[i].device_id) - 1;
				const size_t id_len = id ? strnlen(id, MAX_ID_LEN) : 0;
				memcpy(event_handlers[i].device_id, id, id_len);
				event_handlers[i].device_id[id_len] = 0;
				event_handlers[i].scope = scope;
				return NO_ERROR;
			}
		}
		return INSUFFICIENT_STORAGE;
	}

	inline ProtocolError send_subscriptions(MessageChannel& channel)
	{
		ProtocolError result = for_each([&](const FilteringEventHandler& handler){return send_subscription(channel, handler);});
		if (result==NO_ERROR) {
			//
		}
		return result;
	}

	inline ProtocolError send_subscription(MessageChannel& channel, const char* filter, const char* device_id)
	{
		return send_subscription(channel, filter, device_id, SubscriptionScope::MY_DEVICES);
	}

	inline ProtocolError send_subscription(MessageChannel& channel, const char* filter, SubscriptionScope::Enum scope)
	{
		return send_subscription(channel, filter, nullptr, scope);
	}

};

}
}
