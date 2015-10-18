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

#include "coap.h"

namespace spark
{
namespace protocol
{

class Messages
{
public:
	static CoAPMessageType::Enum decodeType(const uint8_t* buf)
	{
		char path = buf[ 5 + (buf[0] & 0x0F) ];
		// todo - why 5 ? the header is length 4

		switch (CoAP::code(buf))
		{
			case CoAPCode::GET:
				switch (path)
				{
					case 'v': return CoAPMessageType::VARIABLE_REQUEST;
					case 'd': return CoAPMessageType::DESCRIBE;
					default: break;
				}
				break;
			case CoAPCode::POST:
				switch (path)
				{
					case 'E':
					case 'e':
						return CoAPMessageType::EVENT;
					case 'h': return CoAPMessageType::HELLO;
					case 'f': return CoAPMessageType::FUNCTION_CALL;
					case 's': return CoAPMessageType::SAVE_BEGIN;
					case 'u': return CoAPMessageType::UPDATE_BEGIN;
					case 'c': return CoAPMessageType::CHUNK;
					default: break;
				}
				break;
			case CoAPCode::PUT:
				switch (path)
				{
					case 'k': return CoAPMessageType::KEY_CHANGE;
					case 'u': return CoAPMessageType::UPDATE_DONE;
					case 's':
						// todo - use a single message SIGNAL and decode the rest of the message to determine desired state
						if (buf[8])
							return CoAPMessageType::SIGNAL_START;
						else
							return CoAPMessageType::SIGNAL_STOP;
					default: break;
				}
				break;
			case CoAPCode::EMPTY:
				switch (CoAP::type(buf))
				{
					case CoAPType::CON: return CoAPMessageType::PING;
					default: return CoAPMessageType::EMPTY_ACK;
				}
				break;
			case CoAPCode::CONTENT:
				return CoAPMessageType::TIME;
			default:
				break;
		}
		return CoAPMessageType::ERROR;
	}

	static size_t hello(uint8_t* buf, uint16_t message_id, uint8_t flags, uint16_t platform_id, uint16_t product_id, uint16_t product_firmware_version)
	{
		  buf[0] = 0x50; // non-confirmable, no token
		  buf[1] = 0x02; // POST
		  buf[2] = message_id >> 8;
		  buf[3] = message_id & 0xff;
		  buf[4] = 0xb1; // Uri-Path option of length 1
		  buf[5] = 'h';
		  buf[6] = 0xff; // payload marker
		  buf[7] = product_id >> 8;
		  buf[8] = product_id & 0xff;
		  buf[9] = product_firmware_version >> 8;
		  buf[10] = product_firmware_version & 0xff;
		  buf[11] = 0; // reserved flags
		  buf[12] = flags;
		  buf[13] = platform_id >> 8;
		  buf[14] = platform_id & 0xFF;
		  return 15;
	}
};

}}
