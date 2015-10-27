#pragma once

#include "protocol_defs.h"

namespace particle { namespace protocol {

class Pinger
{
	bool expecting_ping_ack;

public:

	/**
	 * Handle ping messages
	 */
	template <typename Callback> int process(system_tick_t millis_since_last_message, Callback ping)
	{
		if (expecting_ping_ack)
		{
			if (10000 < millis_since_last_message)
			{
				// timed out, disconnect
				return PING_TIMEOUT;
			}
			else
			{
				expecting_ping_ack = false;
			}
		}
		else
		{
			if (15000 < millis_since_last_message)
			{
				expecting_ping_ack = true;
				return ping();
			}
		}
		return NO_ERROR;
	}

	bool is_expecting_ping_ack() const { return expecting_ping_ack; }
};


}}
