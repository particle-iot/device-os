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
#include "service_debug.h"
#include "messages.h"
#include "communication_diagnostic.h"

namespace particle { namespace protocol {

uint16_t CoAPMessage::message_count = 0;

ProtocolError CoAPMessageStore::send_message(CoAPMessage* msg, Channel& channel)
{
	Message m((uint8_t*)msg->get_data(), msg->get_data_length(), msg->get_data_length());
	m.decode_id();
	return channel.send(m);
}

/**
 * Returns false if the message should be removed from the queue.
 */
bool CoAPMessageStore::retransmit(CoAPMessage* msg, Channel& channel, system_tick_t now)
{
	bool retransmit = (msg->prepare_retransmit(now));
	if (retransmit)
	{
		send_message(msg, channel);
	}
	return retransmit;
}

void CoAPMessageStore::message_timeout(CoAPMessage& msg, Channel& channel)
{
	g_unacknowledgedMessageCounter++;
	msg.notify_timeout();
	if (msg.is_request())
		channel.command(MessageChannel::CLOSE);
}

/**
 * Process existing messages, resending any unacknowledged requests to the given channel.
 */
void CoAPMessageStore::process(system_tick_t time, Channel& channel)
{
	CoAPMessage* msg = head;
	CoAPMessage* prev = nullptr;
	while (msg!=nullptr)
	{
		if (time_has_passed(time, msg->get_timeout()) && !retransmit(msg, channel, time))
		{
			remove(msg, prev);
			message_timeout(*msg, channel);
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


/**
 * Registers that this message has been sent from the application.
 * Confirmable messages, and ack/reset responses are cached.
 */
ProtocolError CoAPMessageStore::send(Message& msg, system_tick_t time)
{
	if (!msg.has_id())
		return MISSING_MESSAGE_ID;

	DEBUG("sending message id=%x", msg.get_id());
	CoAPType::Enum coapType = CoAP::type(msg.buf());
	if (coapType==CoAPType::CON || coapType==CoAPType::ACK || coapType==CoAPType::RESET)
	{
		// confirmable message, create a CoAPMessage for this
		CoAPMessage* coapmsg = CoAPMessage::create(msg);
		if (coapmsg==nullptr)
			return INSUFFICIENT_STORAGE;
		if (coapType==CoAPType::CON)
			coapmsg->prepare_retransmit(time);
		else
			coapmsg->set_expiration(time+CoAPMessage::MAX_TRANSMIT_SPAN);
		add(*coapmsg);
	}
	return NO_ERROR;
}

/**
 * Notifies the message store that a message has been received.
 */
ProtocolError CoAPMessageStore::receive(Message& msg, Channel& channel, system_tick_t time)
{
	CoAPType::Enum msgtype = msg.get_type();
	msg.decode_id();
	if (msgtype==CoAPType::ACK || msgtype==CoAPType::RESET)
	{
		message_id_t id = msg.get_id();
		if (msgtype==CoAPType::RESET) {
			CoAPMessage* msg = from_id(id);
			if (msg) {
				msg->notify_delivered_nak();
			}
			// a RESET indicates that the session is invalid.
			// Currently the device never sends a RESET, but if it were to do that
			// then we should track which direction we are sending
			channel.command(Channel::DISCARD_SESSION, nullptr);
		}
		DEBUG("recieved ACK for message id=%x", id);
		if (!clear_message(id)) {		// message didn't exist, means it's already been acknoweldged or is unknown.
			msg.set_length(0);
		}
	}
	else if (msgtype==CoAPType::CON)
	{
		CoAPMessage* response = from_id(msg.get_id());
		if (response!=nullptr)
		{
			// consume this message by setting the length to 0
			msg.set_length(0);
			// the message ID already exists as a response message, so
			// send that
			if (is_ack_or_reset(response->get_data(), response->get_data_length()))
				return send_message(response, channel);
		}
		else
		{
			// first time we're seeing this confirmable message, store it in the message store to prevent it from being resent.
			CoAPMessage* coapmsg = CoAPMessage::create(msg, 5);
			if (coapmsg==nullptr)
				return INSUFFICIENT_STORAGE;
			// the timeout here is ideally purely academic since the application will respond immediately with an ACK/RESET
			// which will be stored in place of this message, with it's own timeout.
			coapmsg->set_expiration(time+CoAPMessage::MAX_TRANSMIT_SPAN);
			add(*coapmsg);
		}
	}
	// else it's a NON message - pass through
	return NO_ERROR;
}

bool CoAPMessageStore::has_unacknowledged_requests() const
{
	for (const CoAPMessage* msg = head; msg != nullptr; msg = msg->get_next()) {
		if (is_confirmable((uint8_t*)msg->get_data()))
			return true;
	}

	return false;
}

}}
