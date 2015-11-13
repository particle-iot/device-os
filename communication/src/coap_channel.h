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
	CoAPChannel(message_id_t msg_seed=0) : message_id(msg_seed)
	{
	}

	ProtocolError send(Message& msg) override
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

/**
 * A CoAP message that is available for transmission.
 */
class CoAPMessage
{
	CoAPMessage* next;
	message_id_t id;
	system_tick_t timeout;

	/**
	 * The number of retransmissions of this message allowed.
	 * When 0, the message is removed from the list.
	 */
	uint8_t retransmits;

	// this must come last so that when dynamicaly allocated, the data buffer can be extended
	uint16_t data_len;
	uint8_t data[5];
public:

	CoAPMessage(message_id_t id_) : next(nullptr), id(id_), timeout(0), retransmits(0), data_len(0) {}

	inline CoAPMessage* get_next() const { return next; }
	inline void set_next(CoAPMessage* next) { this->next = next; }
	inline bool matches(message_id_t id) const { return this->id==id; }
	inline message_id_t get_id() const { return id; }
	inline void removed() { next = nullptr; }

	inline CoAPType::Enum type() const
	{
		return data_len>0 ? CoAP::type(data) : CoAPType::ERROR;
	}

	ProtocolError set_data(const uint8_t* data, size_t data_len)
	{
		if (data_len>1500)
			return IO_ERROR;
		memcpy(this->data, data, data_len);
		this->data_len = data_len;
		return NO_ERROR;
	}



};

template <typename T>
class CoAPReliability : public T
{
	CoAPMessage* head;

	CoAPMessage* for_id(message_id_t id, CoAPMessage*& prev)
	{
		prev = nullptr;
		CoAPMessage* next = head;
		while (next)
		{
			if (next->matches(id))
				return next;
			prev = next;
			next = next->get_next();
		}
		return nullptr;
	}

	void remove(CoAPMessage* message, CoAPMessage* previous)
	{
		if (previous)
			previous->set_next(message->get_next());
		else
			head = message->get_next();
		message->removed();
	}

public:

	CoAPReliability() : head(nullptr) {}

	CoAPMessage* from_id(message_id_t id)
	{
		CoAPMessage* prev;
		CoAPMessage* next = for_id(id, prev);
		return next;
	}

	ProtocolError add(CoAPMessage& message)
	{
		remove(message);
		if (message.get_next())
			return INVALID_STATE;
		message.set_next(head);
		head = &message;
		return NO_ERROR;
	}

	bool remove(CoAPMessage& msg)
	{
		return remove(msg.get_id())==&msg;
	}

	CoAPMessage* remove(message_id_t msg_id)
	{
		CoAPMessage* prev;
		CoAPMessage* msg = for_id(msg_id, prev);
		if (msg) {
			remove(msg, prev);
		}
		return msg;
	}

	ProtocolError send(Message& msg) override
	{
		return IO_ERROR;
	}

};

}}
