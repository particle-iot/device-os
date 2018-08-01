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

#include <cstdint>
#include <cstddef>
#include "protocol_defs.h"
#include "coap.h"

namespace particle
{
namespace protocol
{


class Message
{
	static const unsigned MINIMUM_COAP_MESSAGE_LENGTH = 4;
	template<size_t max, size_t prefix, size_t suffix>
	friend class BufferMessageChannel;

	uint8_t* buffer;
	size_t buffer_length;
	size_t message_length;
    int id;                     // if < 0 then not-defined.
    bool confirm_received;

	size_t trim_capacity()
	{
		size_t trimmed = buffer_length-message_length;
		buffer_length = message_length;
		return trimmed;
	}

	size_t buffer_available() const { return buffer_length-message_length; }

	bool splinter(Message& target, size_t size_required, size_t offset)
	{
		size_t available = buffer_available();
		if (available<(size_required+offset))
			return false;

		int excess = trim_capacity();
		target.set_buffer(buf()+length()+offset, excess);
		return true;
	}

public:
	Message() : Message(nullptr, 0, 0) {}

	Message(uint8_t* buf, size_t buflen, size_t msglen=0) : buffer(buf), buffer_length(buflen), message_length(msglen), id(-1), confirm_received(false) {}

	void clear() { id = -1; }

	size_t capacity() const { return buffer_length; }
	uint8_t* buf() const { return buffer; }
	size_t length() const { return message_length; }

	void set_length(size_t length) { if (length<=buffer_length) message_length = length; }
	void set_buffer(uint8_t* buffer, size_t length) { this->buffer = buffer; buffer_length = length; message_length = 0; }

    void set_id(message_id_t id) { this->id = id; }
    bool has_id() { return id>=0; }
    message_id_t get_id() { return message_id_t(id); }

    /**
     * This is used for non-CoAP messages, such as zero and 1 byte messages used to keep the
     * channel open.
     */
    bool send_direct() { return length()<MINIMUM_COAP_MESSAGE_LENGTH; }

    CoAPType::Enum get_type() const
    {
    		return length()>=MINIMUM_COAP_MESSAGE_LENGTH ? CoAP::type(buf()): CoAPType::ERROR;
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

    bool decode_id()
    {
    		bool decode = (length()>=MINIMUM_COAP_MESSAGE_LENGTH);
    		if (decode)
    			set_id(CoAP::message_id(buf()));
    		return decode;
    }

    void set_confirm_received(bool confirm)
    {
    		this->confirm_received = confirm;
    }

    bool get_confirm_received() const { return confirm_received; }

    /**
     * Set the contents of this message.
     */
	size_t copy(const uint8_t* buf, size_t len)
	{
		if (len>capacity())
			len = capacity();
		memcpy(this->buffer, buf, len);
		set_length(len);
		return len;
	}

	bool content_equals(Message& msg)
	{
		return msg.length()==this->length() &&
				memcmp(msg.buf(), this->buf(),this->length())==0;
	}

	Message& operator=(const Message& msg)
	{
		this->buffer = msg.buffer;
		this->buffer_length = msg.buffer_length;
		this->message_length = msg.message_length;
		this->id = msg.id;
		this->confirm_received = msg.confirm_received;
		return *this;
	}

};

struct Channel
{
	enum Command
	{
		CLOSE = 0,

		/**
		 * Discard the current session. This performs a new
		 * DTLS handshake with the server.
		 */
		DISCARD_SESSION = 1,

		/**
		 * Sends a special DTLS packet to the server that indicates
		 * the session has moved.
		 */
		MOVE_SESSION = 2,

		/**
		 * Load session - load the session from persistent store.
		 */
		LOAD_SESSION = 3,

		/**
		 * Save session - saves the session to persistent store.
		 */
		SAVE_SESSION = 4,
	};


	/**
	 * Fetch the next message from the channel.
	 * If no message is ready, a message of size 0 is returned.
	 *
	 * @return an error value !=0 on error.
	 */
	virtual ProtocolError receive(Message& message)=0;

	/**
	 * Send the given message to the endpoint
	 * @return an error value !=0 on error.
	 */
	virtual ProtocolError send(Message& msg)=0;

	/**
	 * Close this channel, preventing further messages from being sent.
	 */
	virtual ProtocolError command(Command cmd, void* arg=nullptr)=0;
};

/**
 * A message channel represents a way to send and receive messages with an endpoint.
 *
 * Note that the implementation may use a shared message buffer for all
 * message operations. The only operation that does not invalidate an existing
 * message is MessageChannel::response() since this allocates the new message at the end of the existing one.
 *
 */
struct MessageChannel : public Channel
{

	virtual ~MessageChannel() {}

	/**
	 * Determines if this channel requires CoAP acknowledgements for helping with reliable delivery or not.
	 */
	virtual bool is_unreliable()=0;

	/**
	 * Establish this channel for communication.
	 * @param flags on return, SKIP_SESSION_RESUME_HELLO is set if the hello/vars/funcs/sucriptions regitration is not needed.
	 * @param app_state_crc	The crc of the current application state.
	 */
	virtual ProtocolError establish(uint32_t& flags, uint32_t app_state_crc)=0;

	/**
	 * Retrieves a new message object containing the message buffer.
	 */
	virtual ProtocolError create(Message& message, size_t minimum_size=0)=0;

	/**
	 * Fill out a message struct to contain storage for a response.
	 */
	virtual ProtocolError response(Message& original, Message& response, size_t required)=0;

	/**
	 * Notification from the upper layers that the application has established
	 * the communications channel. Any post establish actions that are needed
	 * can be performed.
	 */
	virtual ProtocolError notify_established()=0;
};

class AbstractMessageChannel : public MessageChannel
{

};


}}
