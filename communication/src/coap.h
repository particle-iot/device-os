/**
  ******************************************************************************
  * @file    coap.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   COAP
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

#include <string.h>
#include <stdint.h>
#include <stddef.h>

namespace particle { namespace protocol {

typedef uint8_t token_t;
typedef uint16_t message_id_t;

namespace CoAPMessageType {
  enum Enum {
    HELLO,                  // 0
    DESCRIBE,
    FUNCTION_CALL,
    VARIABLE_REQUEST,
    SAVE_BEGIN,
    UPDATE_BEGIN,           // 5
    UPDATE_DONE,
    CHUNK,
    EVENT,
    KEY_CHANGE,
    SIGNAL_START,           // 10
    SIGNAL_STOP,
    TIME,
    EMPTY_ACK,
    PING,
    ERROR,
    NONE,
  };
}

namespace CoAPCode {
  enum Enum {
    GET,
    POST,
    PUT,
    EMPTY,
    CONTENT,
    ERROR,

	// responses
	NONE = 0,
	CONTINUE = 40,
	OK = 80,
	CREATED = 81,
	NOT_MODIFIED = 124,
	BAD_REQUEST = 160,
	NOT_FOUND = 164,
	METHOD_NOT_ALLOWED = 165,
	UNSUPPORTED_MEDIA_TYPE = 175,
	INTERNAL_SERVER_ERROR = 200,
	BAD_GATEWAY = 202,
	SERVICE_UNAVAILABLE = 203,
	GATEWAY_TIMEOUT = 204,
	TOKEN_OPTION_REQUIRED = 240,
	URI_AUTHORITY_OPTION_REQUIRED = 241,
	CRITICAL_OPTION_NOT_SUPPORTED = 242

  };
}

namespace CoAPType {
  enum Enum {
    CON,
    NON,
    ACK,
    RESET,
	ERROR
  };
}

class CoAP
{
  public:

	static const uint8_t VERSION = 1;

	static inline message_id_t message_id(uint8_t* buf)
	{
		return buf[2]<<8 | buf[3];
	}

	size_t header(uint8_t* buf, CoAPType::Enum type, uint8_t tokenLen, CoAPCode::Enum code, uint16_t msgid)
	{
		buf[0] = VERSION<<6 | type << 4 | (tokenLen & 0xF);
		buf[1] = code;
		buf[2] = msgid >> 8;
		buf[3] = msgid & 0xFF;
		return 4;
	}


	/**
	 * Fetches the CoAP path from a CoAP message.
	 */
	static const unsigned char* path(const unsigned char* message)
	{
		// this assumes the Uri-Path is the first option
		return message + 5 + (message[0] & 0x0F);
	}

    static CoAPCode::Enum code(const unsigned char *message);
    static CoAPType::Enum type(const unsigned char *message);
    static size_t option_decode(unsigned char **option);
};

// this uses version 0 to maintain compatiblity with the original comms lib codes
#define COAP_MSG_HEADER(type, tokenlen) \
	((CoAP::VERSION)<<6 | (type)<<4 | ((tokenlen) & 0xF))
}}
