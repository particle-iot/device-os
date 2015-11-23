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

#include "coap_channel.h"

namespace particle { namespace protocol {

uint16_t CoAPMessage::message_count = 0;



/**
 * Returns false if the message should be removed from the queue.
 */
bool CoAPMessageStore::retransmit(CoAPMessage* msg, MessageChannel& channel, system_tick_t now)
{
	bool retransmit = (msg->prepare_retransmit(now));
	if (retransmit)
	{
		Message m((uint8_t*)msg->get_data(), msg->get_data_length(), msg->get_data_length());
		m.decode_id();
		channel.send(m);
	}
	return retransmit;
}

/**
 * Process existing messages, resending any unacknowledged requests to the given channel.
 */
void CoAPMessageStore::process(system_tick_t time, MessageChannel& channel)
{
	CoAPMessage* msg = head;
	CoAPMessage* prev = nullptr;
	while (msg!=nullptr)
	{
		if (time_has_passed(time, msg->get_timeout()) && !retransmit(msg, channel, time))
		{
			remove(msg, prev);
			delete msg;
			msg = (prev==nullptr) ? head : prev->get_next();
		}
		else
		{
			prev = msg;
			msg = msg->get_next();
		}
	}
}


}}
