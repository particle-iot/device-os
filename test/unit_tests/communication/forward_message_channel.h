/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

namespace particle { namespace protocol {

/**
 * A message handler that forwards to another handler.
 */
class ForwardMessageChannel : public MessageChannel
{
	MessageChannel* channel;

public:

	ForwardMessageChannel() : channel(nullptr) {}
	ForwardMessageChannel(MessageChannel& channel_) : channel(&channel_) {}

	bool is_unreliable() override { return true; }

	void setForward(MessageChannel* channel)
	{
		this->channel = channel;
	}

	ProtocolError send(Message& msg) override
	{
		return channel->send(msg);
	}

	ProtocolError receive(Message& msg) override
	{
		return channel->receive(msg);
	}

	ProtocolError create(Message& msg, size_t size) override
	{
		return channel->create(msg, size);
	}

	virtual ProtocolError establish(uint32_t& flags, uint32_t app_state_crc) override
	{
		return channel->establish(flags, app_state_crc);
	}

	virtual ProtocolError response(Message& original, Message& response, size_t required) override
	{
		return channel->response(original, response, required);
	}

	virtual ProtocolError command(Command cmd, void* arg=nullptr) override {
		return IO_ERROR_FORWARD_MESSAGE_CHANNEL;
	}

	virtual ProtocolError notify_established() override { return NO_ERROR; }
};


}}
