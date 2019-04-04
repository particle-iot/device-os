#pragma once

#include "protocol_defs.h"

namespace particle { namespace protocol {

class Pinger
{
	bool expecting_ping_ack;
	system_tick_t ping_interval;
	system_tick_t ping_timeout;
	keepalive_source_t keepalive_source;

public:
	Pinger() : expecting_ping_ack(false), ping_interval(0), ping_timeout(10000), keepalive_source(KeepAliveSource::SYSTEM) {}

	/**
	 * Sets the ping interval that the client will send pings to the server, and the expected maximum response time.
	 */
	void init(system_tick_t interval, system_tick_t timeout)
	{
		this->ping_interval = interval;
		this->ping_timeout = timeout;
		this->keepalive_source = KeepAliveSource::SYSTEM;
	}

	void set_interval(system_tick_t interval, keepalive_source_t source)
	{
		/**
		 * LAST  CURRENT  UPDATE?
		 * ======================
		 * SYS   SYS      YES
		 * SYS   USER     YES
		 * USER  SYS      NO
		 * USER  USER     YES
		 */
		if ( !(this->keepalive_source == KeepAliveSource::USER && source == KeepAliveSource::SYSTEM) )
		{
			this->ping_interval = interval;
			this->keepalive_source = source;
		}
	}

	void reset()
	{
		expecting_ping_ack = false;
	}

	/**
	 * Handle ping messages. If a message is not received
	 * within the timeout, the connection is considered unreliable.
	 * @param millis_since_last_message Elapsed number of milliseconds since the last message was received.
	 * @param callback a no-arg callable that is used to perform a ping to the cloud.
	 */
	template <typename Callback> ProtocolError process(system_tick_t millis_since_last_message, Callback ping)
	{
		if (expecting_ping_ack)
		{
			if (ping_timeout < millis_since_last_message)
			{
				// timed out, disconnect
				return PING_TIMEOUT;
			}
		}
		else
		{
			// ping interval set, so check if we need to send a ping
			// The ping is sent based on the elapsed time since the last message
			if (ping_interval && ping_interval < millis_since_last_message)
			{
				expecting_ping_ack = true;
				return ping();
			}
		}
		return NO_ERROR;
	}

	bool is_expecting_ping_ack() const { return expecting_ping_ack; }

	/**
	 * Notifies the Pinger that a message has been received
	 * and that there is presently no need to resend a ping
	 * until the ping interval has elapsed.
	 */
	void message_received() { expecting_ping_ack = false; }
};


}}
