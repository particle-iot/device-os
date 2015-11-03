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

#include "service_debug.h"
#include "message_channel.h"
#include "coap.h"

namespace particle
{
namespace protocol
{


template <typename T>
class CoAPChannel : public T
{
	message_id_t message_id;
	using base = T;

protected:

	message_id_t next_message_id()
	{
		return ++message_id;
	}

public:


	ProtocolError send(Message& msg)
	{
		message_id_t id;
		if (msg.has_id())
		{
			id = msg.get_id();
		}
		else
		{
			id = next_message_id();
		}

		uint8_t* buf = msg.buf();
		buf[2] = id >> 8;
		buf[3] = id & 0xFF;

		return T::send(msg);
	}
};

}}
