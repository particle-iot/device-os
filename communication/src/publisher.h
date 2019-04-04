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

#include "protocol_defs.h"
#include "events.h"
#include "message_channel.h"
#include "messages.h"

#include "completion_handler.h"
#include "communication_diagnostic.h"

namespace particle
{
namespace protocol
{

class Protocol;

class Publisher
{
public:
	explicit Publisher(Protocol* protocol) :
			protocol(protocol)
	{
	}

	inline bool is_system(const char* event_name)
	{
		return !strncmp(event_name, "spark", 5) || !strncmp(event_name, "particle", 8);
	}

	bool is_rate_limited(bool is_system_event, system_tick_t millis)
	{
		if (is_system_event)
		{
			static uint16_t lastMinute = 0;
			static uint8_t eventsThisMinute = 0;

			uint16_t currentMinute = uint16_t(millis >> 16);
			if (currentMinute == lastMinute)
			{      // == handles millis() overflow
				if (eventsThisMinute == 255)
					return true;
			}
			else
			{
				lastMinute = currentMinute;
				eventsThisMinute = 0;
			}
			eventsThisMinute++;
		}
		else
		{
			static system_tick_t recent_event_ticks[5] =
			{ (system_tick_t) -1000, (system_tick_t) -1000,
					(system_tick_t) -1000, (system_tick_t) -1000,
					(system_tick_t) -1000 };
			static int evt_tick_idx = 0;

			system_tick_t now = recent_event_ticks[evt_tick_idx] = millis;
			evt_tick_idx++;
			evt_tick_idx %= 5;
			if (now - recent_event_ticks[evt_tick_idx] < 1000)
			{
				// exceeded allowable burst of 4 events per second
				return true;
			}
		}
		return false;
	}

	ProtocolError send_event(MessageChannel& channel, const char* event_name,
			const char* data, int ttl, EventType::Enum event_type, int flags,
			system_tick_t time, CompletionHandler handler)
	{
		bool is_system_event = is_system(event_name);
		bool rate_limited = is_rate_limited(is_system_event, time);
		if (rate_limited) {
			g_rateLimitedEventsCounter++;
			return BANDWIDTH_EXCEEDED;
		}

		Message message;
		channel.create(message);
		bool confirmable = channel.is_unreliable();
		if (flags & EventType::NO_ACK) {
			confirmable = false;
		} else if (flags & EventType::WITH_ACK) {
			confirmable = true;
		}
		size_t msglen = Messages::event(message.buf(), 0, event_name, data, ttl,
				event_type, confirmable);
		message.set_length(msglen);
		const ProtocolError result = channel.send(message);
		if (result == NO_ERROR) {
			// Register completion handler only if acknowledgement was requested explicitly
			if ((flags & EventType::WITH_ACK) && message.has_id()) {
			    add_ack_handler(message.get_id(), std::move(handler));
			} else {
			    handler.setResult();
			}
		}
		return result;
	}

private:
	Protocol* protocol;

	void add_ack_handler(message_id_t msg_id, CompletionHandler handler);
};

}}
