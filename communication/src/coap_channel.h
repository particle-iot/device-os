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
#include "timer_hal.h"
#include "stdlib.h"
#include "service_debug.h"

namespace particle
{
namespace protocol
{

/**
 * Decorates a MessageChannel with message ID management as required by CoAP.
 * When a message is sent that doesn't have an assigned ID, it is assigned the next available ID.
 */
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

	/**
	 * Retrieves the next message ID this channel will send.
	 */
	message_id_t& next_id_ref()
	{
		return message_id;
	}

	ProtocolError send(Message& msg) override
	{
		if (msg.length()>=4)
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
			msg.decode_id();
		}
		return base::send(msg);
	}
};

/**
 * A CoAP message that is available for (re-)transmission.
 */
class __attribute__((packed)) CoAPMessage
{
public:
	enum Delivery
	{
		DELIVERED,
		DELIVERED_NACK,
		NOT_DELIVERED
	};

	using delivery_fn = std::function<void(Delivery)>;

private:
	/**
	 * Messages are stored as a singly-linked list.
	 * This pointer is the next message in the list, or nullptr if this is the last message in the list.
	 */
	CoAPMessage* next;

	/**
	 * The time when the system will resend this message or give up sending
	 * when the maximum number of transmits has been reached.
	 */
	system_tick_t timeout;

	/**
	 * The unique 16-bit ID for this message.
	 */
	message_id_t id;

	/**
	 * The number of times this message has been transmitted.
	 * 0 means the message has not been sent yet.
	 */
	uint8_t transmit_count;

	// padding
	// uint8_t reserved;
	std::function<void(Delivery)>* delivered;


	/**
	 * How many data bytes follow.
	 */
	uint16_t data_len;

	/**
	 * The CoAPMessage is dynamically allocated as a single chunk combining both the fields above and the message data.
	 */
	uint8_t data[0];

	static uint16_t message_count;

	/**
	 * Notification that the message has been delivered to the server.
	 */
	inline void notify_delivered(Delivery success) const {
		if (delivered) {
			(*delivered)(success);
		}
	}

public:

	static const uint16_t ACK_TIMEOUT = 4000;
	static const uint16_t ACK_RANDOM_FACTOR = 1500;
	static const uint16_t ACK_RANDOM_DIVISOR = 1000;
	static const uint8_t MAX_RETRANSMIT = 3;
	static const uint16_t MAX_TRANSMIT_SPAN = 45*1000;


	/**
	 * The number of outstanding messages allowed.
	 */
	static const uint8_t NSTART = 1;


	CoAPMessage(message_id_t id_) : next(nullptr), timeout(0), id(id_), transmit_count(0), delivered(nullptr), data_len(0) {
		message_count++;
	}

	/**
	 * Create a new CoAPMessage from the given Message instance. The returned CoAPMessage is dynamically allocated
	 * and has an independent lifetime from the Message
	 * instance. When no longer required, `delete` the CoAPMessage..
	 */
	static CoAPMessage* create(Message& msg, size_t data_len = 0)
	{
		size_t len = data_len && data_len<msg.length() ? data_len : msg.length();
		uint8_t* memory = new uint8_t[sizeof(CoAPMessage)+len];
		if (memory) {
			CoAPMessage* coapmsg = new (memory)CoAPMessage(msg.get_id());		// in-place new
			coapmsg->set_data(msg.buf(), len);
			return coapmsg;
		}
		return nullptr;
	}

	~CoAPMessage()
	{
		message_count--;
	}

	static uint16_t messages() { return message_count; }

	inline CoAPMessage* get_next() const { return next; }
	inline void set_next(CoAPMessage* next) { this->next = next; }
	inline bool matches(message_id_t id) const { return this->id==id; }
	inline message_id_t get_id() const { return id; }
	inline void removed() { next = nullptr; }
	inline system_tick_t get_timeout() const { return timeout; }

	inline void set_delivered_handler(std::function<void(Delivery)>* handler) { this->delivered = handler; }

	inline void notify_timeout() const {
		notify_delivered(NOT_DELIVERED);
	}

	inline void notify_delivered_ok() const {
		notify_delivered(DELIVERED);
	}

	inline void notify_delivered_nak() const {
		notify_delivered(DELIVERED_NACK);
	}

	/**
	 * Prepares to retransmit this message after a timeout.
	 * @return false if the message cannot be retransmitted.
	 */
	bool prepare_retransmit(system_tick_t now)
	{
		CoAPType::Enum coapType = CoAP::type(get_data());
		if (coapType==CoAPType::CON) {
			timeout = now + transmit_timeout(transmit_count);
			transmit_count++;
			return transmit_count <= MAX_RETRANSMIT+1;
		}
		// other message types are not resent on timeout
		return false;
	}

	/**
	 * Determines the transmit timeout for the given transmission count.
	 */
	static inline system_tick_t transmit_timeout(uint8_t transmit_count)
	{
		system_tick_t timeout = (ACK_TIMEOUT << transmit_count);
		timeout += ((timeout * (rand()%256))>>9);
		return timeout;
	}

	inline CoAPType::Enum get_type() const
	{
		return data_len>0 ? CoAP::type(data) : CoAPType::ERROR;
	}

	ProtocolError set_data(const uint8_t* data, size_t data_len)
	{
		if (data_len>1500)
			return IO_ERROR_SET_DATA_MAX_EXCEEDED;
		memcpy(this->data, data, data_len);
		this->data_len = data_len;
		return NO_ERROR;
	}

	const uint8_t* get_data() const { return data; }
	uint16_t get_data_length() const { return data_len; }

	/**
	 * Sets the time when this message expires.
	 */
	void set_expiration(system_tick_t time)
	{
		timeout = time;
		transmit_count = MAX_RETRANSMIT+2;	// do not send this message.
	}

    bool is_request() const
    {
    		switch (get_type()) {
    		case CoAPType::NON:
    		case CoAPType::CON:
    			return true;
    		default:
    			return false;
    		}
    }

};

inline bool time_has_passed(system_tick_t now, system_tick_t tick)
{
	static_assert(sizeof(system_tick_t)==4, "system_tick_t should be 4 bytes");
	if (now>=tick)
	{
		return now-tick <= 0x7FFFFFFF;
	}
	else
	{
		return tick-now >= 0x80000000;
	}
}



/**
 * A mix-in class that provides message resending for reliable delivery of messages.
 */
class CoAPMessageStore
{
	LOG_CATEGORY("comm.coap");

	/**
	 * The head of the list of messages.
	 */
	CoAPMessage* head;

	/**
	 * Retrieves the message with the given ID and the previous message.
	 * If no message exists with the given id, nullptr is returned.
	 */
	CoAPMessage* for_id(message_id_t id, CoAPMessage*& prev) const
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

	/**
	 * Removes a message given the message to remove and the previous entry in the list.
	 */
	void remove(CoAPMessage* message, CoAPMessage* previous)
	{
		if (previous)
			previous->set_next(message->get_next());
		else
			head = message->get_next();
		message->removed();
	}

	void message_timeout(CoAPMessage& msg, Channel& channel);

public:

	CoAPMessageStore() : head(nullptr) {}

	~CoAPMessageStore() {
		clear();
	}

	bool has_messages() const
	{
		return head!=nullptr;
	}

	bool has_unacknowledged_requests() const;

	/**
	 * Retrieves the current confirmable message that is still
	 * waiting acknowledgement.
	 * Returns nullptr if there is no such unconfirmed message.
	 */
	CoAPMessage* from_id(message_id_t id) const
	{
		CoAPMessage* prev;
		CoAPMessage* next = for_id(id, prev);
		return next;
	}

	ProtocolError add(CoAPMessage* message)
	{
		return add(*message);
	}

	/**
	 * Adds a message to this message store.
	 */
	ProtocolError add(CoAPMessage& message)
	{
		// trying to add exactly the same message
		if (from_id(message.get_id())==&message)
			return NO_ERROR;

		clear_message(message.get_id());
		if (message.get_next())
			return INVALID_STATE;
		message.set_next(head);
		head = &message;
		return NO_ERROR;
	}

	/**
	 * Removes a message from the store with the given id.
	 * Returns nullptr if the message does not exist. Returns
	 * the removed message otherwise.
	 */
	CoAPMessage* remove(message_id_t msg_id)
	{
		CoAPMessage* prev;
		CoAPMessage* msg = for_id(msg_id, prev);
		if (msg) {
			remove(msg, prev);
		}
		return msg;
	}

	bool is_confirmable(const uint8_t* buf) const
	{
		return CoAP::type(buf)==CoAPType::CON;
	}

	/**
	 * Returns false if the message should be removed from the queue.
	 */
	bool retransmit(CoAPMessage* msg, Channel& channel, system_tick_t now);

	/**
	 * Process existing messages, resending any unacknowledged requests to the given channel.
	 */
	void process(system_tick_t time, Channel& channel);

	/**
	 * Sends the given CoAPMessage to the channel.
	 */
	ProtocolError send_message(CoAPMessage* msg, Channel& channel);

	bool is_ack_or_reset(const uint8_t* buf, size_t len)
	{
		if (len<1)
			return false;
		CoAPType::Enum type = CoAP::type(buf);
		return type==CoAPType::ACK || type==CoAPType::RESET;
	}

	/**
	 * Send a message synchronously, waiting for the acknowledgement.
	 */
	template<typename Time>
	ProtocolError send_synchronous(Message& msg, Channel& channel, Time& time)
	{
		message_id_t id = msg.get_id();
		DEBUG("sending message id=%x synchronously", id);
		CoAPType::Enum coapType = CoAP::type(msg.buf());
		ProtocolError error = send(msg, time());
		if (!error)
			error = channel.send(msg);
		if (!error && coapType==CoAPType::CON)
		{
			CoAPMessage::delivery_fn flag_delivered = [&error](CoAPMessage::Delivery delivered) {
				if (delivered==CoAPMessage::NOT_DELIVERED)
					error = MESSAGE_TIMEOUT;
				else if (delivered==CoAPMessage::DELIVERED_NACK)
					error = MESSAGE_RESET;
			};
			CoAPMessage* coapmsg = from_id(id);
			if (coapmsg)
				coapmsg->set_delivered_handler(&flag_delivered);
			else
				ERROR("no coapmessage for msg id=%x", id);
			while (from_id(id)!=nullptr && !error)
			{
				msg.clear();
				msg.set_length(0);
				error = channel.receive(msg);
				if (!error && msg.decode_id() && is_ack_or_reset(msg.buf(), msg.length()))
				{
					// handle acknowledgements, waiting for the one that
					// acknowledges the original confirmation.
					ProtocolError receive_error = receive(msg, channel, time());
					if (!error)
						error = receive_error;
				}
				// drop CON messages on the floor since we cannot handle them now
				process(time(), channel);
			}
		}
		clear_message(id);
		// todo - if msg contains a delivery callback then call that with the outcome of this
		return error;
	}

	/**
	 * Registers that this message has been sent from the application.
	 * Confirmable messages, and ack/reset responses are cached.
	 */
	ProtocolError send(Message& msg, system_tick_t time);

	/**
	 * Notifies the message store that a message has been received.
	 */
	ProtocolError receive(Message& msg, Channel& channel, system_tick_t time);

	bool clear_message(message_id_t id)
	{
		CoAPMessage* msg = remove(id);
		delete msg;
		return msg!=nullptr;
	}

	/**
	 * Removes all knowledge of any messages.
	 */
	void clear()
	{
		while (head!=nullptr)
		{
			delete remove(head->get_id());
		}
	}

};


/**
 * Implements reliable CoAP messaging by attempting to resent the message
 * multiple times when an acknowledgement isn't received.
 * @param T the baseclass
 * @param M: a callable type that provides the current system ticks
 */
template <class T, typename M>
class CoAPReliableChannel : public T
{
	using channel = T;
	M millis;

	/**
	 * Stores the unhandled confirmable messages received from the server, or the outstanding acknowledgment.
	 */
	CoAPMessageStore server;

	/**
	 * Stores the confirmable messages sent from the client requiring acknowledgement.
	 */
	CoAPMessageStore client;


	ProtocolError base_send(Message& msg)
	{
		return channel::send(msg);
	}

	ProtocolError base_receive(Message& msg)
	{
		return channel::receive(msg);
	}

	/**
	 * A channel that delegates to another one.
	 */
	class DelegateChannel : public Channel
	{
		CoAPReliableChannel<T,M>* channel;

	public:
		void init(CoAPReliableChannel<T, M>* channel)
		{
			this->channel = channel;
		}

		ProtocolError receive(Message& msg) override
		{
			return channel->base_receive(msg);
		}

		ProtocolError send(Message& msg) override
		{
			return channel->base_send(msg);
		}

		ProtocolError command(Command cmd, void* arg) override
		{
			return channel->command(cmd, arg);
		}
	};

	friend class DelegateChannel;

	DelegateChannel delegateChannel;

public:

	CoAPReliableChannel(M m=0) : millis(m) {
		delegateChannel.init(this);
	}

	void set_millis(M m) {
		this->millis = m;
	}

	const CoAPMessageStore& client_messages() const {
		return client;
	}

	const CoAPMessageStore& server_messages() const {
		return server;
	}

	/**
	 * Clear the message stores when the channel is initially established.
	 */
	ProtocolError establish(uint32_t& flags, uint32_t app_crc) override
	{
		server.clear();
		client.clear();
		return channel::establish(flags, app_crc);
	}

	/**
	 * Sends the message reliably. A non-confirmable message
	 * it is sent once. A confirmable message is sent and resent
	 * until an ack is received or the message times out.
	 */
	ProtocolError send(Message& msg) override
	{
		if (msg.send_direct())
			return delegateChannel.send(msg);

		if (msg.is_request() && msg.get_confirm_received())
			return client.send_synchronous(msg, delegateChannel, millis);

		// determine the type of message.
		CoAPMessageStore& store = msg.is_request() ? client : server;
		ProtocolError error = store.send(msg, millis());
		if (!error)
			error = channel::send(msg);
		return error;
	}

	/**
	 * Receives a message from the channel and passes it to the message store for processing before
	 * passing on to the application.
	 *
	 * Calls background processing for the message store.
	 */
	ProtocolError receive(Message& msg) override
	{
		return receive(msg, true);
	}

	/**
	 * Pulls messages from the message channel
	 */
	ProtocolError receive_confirmations()
	{
		Message msg;
		channel::create(msg);
		return receive(msg, false);
	}

	bool has_unacknowledged_requests() const
	{
		return client.has_messages() || server.has_unacknowledged_requests();
	}

	/**
	 * Pulls messages from the channel and stores it in a message store for
	 * reliable receipt and retransmission.
	 *
	 * @param msg			The message received
	 * @param requests		When true, both requests and responses are retrieved. When false, only responses are retrieved.
	 */
	ProtocolError receive(Message& msg, bool requests)
	{
		ProtocolError error = channel::receive(msg);
		if (!error && msg.length())
		{
			// is it a request from the server or a response from the server?
			// responses are paired with the original client request
			CoAPMessageStore& store = msg.is_request() ? server : client;
			if (!msg.is_request() || requests) {
				error = store.receive(msg, delegateChannel, millis());
			}
		}
		client.process(millis(), delegateChannel);
		server.process(millis(), delegateChannel);
		return error;
	}

};





}}
