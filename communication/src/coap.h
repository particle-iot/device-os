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
    ERROR,					// 15
    NONE,
  };
}

#define COAP_RESPONSE(x,y) ((x<<5)|y)

/**
 * Values for the code field of a CoAP message.
 */
namespace CoAPCode {
  enum Enum {
	EMPTY,
    GET,
    POST,
    PUT,
    DELETE,

	// responses
	NONE = 0,
	CONTINUE = 40,
	OK = COAP_RESPONSE(2,00),
	CREATED = COAP_RESPONSE(2,01),
	DELETED = COAP_RESPONSE(2,02),
	NOT_MODIFIED = COAP_RESPONSE(2,03),
	CHANGED = COAP_RESPONSE(2,04),
	CONTENT = COAP_RESPONSE(2,05),
	BAD_REQUEST = COAP_RESPONSE(4,00),
	UNAUTHORIZED = COAP_RESPONSE(4,01),
	BAD_OPTION = COAP_RESPONSE(4,02),
	FORBIDDEN = COAP_RESPONSE(4,03),
	NOT_FOUND = COAP_RESPONSE(4,04),
	METHOD_NOT_ALLOWED = COAP_RESPONSE(4,05),
	NOT_ACCEPTABLE = COAP_RESPONSE(4,06),
	REQUEST_ENTITY_INCOMPLETE = 136,
	PRECONDITION_FAILED = COAP_RESPONSE(4,12),
	REQUEST_ENTITY_TOO_LARGE = COAP_RESPONSE(4,13),
	UNSUPPORTED_CONTENT_FORMAT = COAP_RESPONSE(4,15),
	INTERNAL_SERVER_ERROR = COAP_RESPONSE(5,00),
	NOT_IMPLEMENTED = COAP_RESPONSE(5,01),
	BAD_GATEWAY = COAP_RESPONSE(5,02),
	SERVICE_UNAVAILABLE = COAP_RESPONSE(5,03),
	GATEWAY_TIMEOUT = COAP_RESPONSE(5,04),
//	TOKEN_OPTION_REQUIRED = 240,
//	URI_AUTHORITY_OPTION_REQUIRED = 241,
//	CRITICAL_OPTION_NOT_SUPPORTED = 242
    ERROR=BAD_REQUEST,				// this is a pseudo-code value. It used to be 5 (to represent an error type) but

  };

  inline bool is_success(Enum value) {
	  return value < COAP_RESPONSE(4,00);
  }
}

namespace CoAPOption {
	enum Enum {
		NONE = 0,
		LOCATION_PATH = 8,
		URI_PATH = 11,
		URI_QUERY = 15
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

  inline bool is_reply(CoAPType::Enum value) {
	  return value==ACK || value==RESET;
  }
}

class CoAP
{
public:

	static const uint8_t VERSION = 1;

	static inline message_id_t message_id(uint8_t* buf)
	{
		return buf[2]<<8 | buf[3];
	}

	static size_t header(uint8_t* buf, CoAPType::Enum type, CoAPCode::Enum code, const uint8_t tokenLen=0, const uint8_t* token=nullptr, message_id_t msgid=0)
	{
		buf[0] = VERSION<<6 | type << 4 | (tokenLen & 0xF);
		buf[1] = code;
		buf[2] = msgid >> 8;
		buf[3] = msgid & 0xFF;
		buf += 4;
		uint8_t t = tokenLen;
		while (t --> 0) {
			*buf++ = *token++;
		}
		return 4+tokenLen;
	}

	static size_t uri_path(uint8_t* buf, CoAPOption::Enum previous, const char* path) {
		return add_option(buf, previous, CoAPOption::URI_PATH, path, strlen(path));
	}

	static size_t uri_query(uint8_t* buf, CoAPOption::Enum previous, const char* query) {
		return add_option(buf, previous, CoAPOption::URI_QUERY, query, strlen(query));
	}

	static size_t add_option(uint8_t* const buf, CoAPOption::Enum previous, CoAPOption::Enum current, const void* data, uint16_t length) {
		uint8_t* p = buf;
		uint16_t delta = current-previous;
		uint8_t delta_nibble = option_value_nibble(delta);
		uint8_t length_nibble = option_value_nibble(length);
		*p++ = (delta_nibble << 4) | length_nibble;
		p += extended_option_value(p, delta_nibble, delta);
		p += extended_option_value(p, length_nibble, length);
		if (length) {
			memcpy(p, data, length);
			p += length;
		}
		return p-buf;
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

    /**
     * Computes the length indicator for a value encoded in CoAP.
     * Values less than 13 are encoded directly. Values between 13 and 268 (inclusive) are encoded as 13 (and later as a single byte extended option)
     * and values beyond that are encoded as 14, with a 2-byte extended option.
     */
    static uint8_t option_value_nibble(size_t value)
    {
		if (value <= 12)
		{
			return value;
		}
		else if (value <= 0xFF + 13)
		{
			return 13;
		}
		else
		{
			return 14;
		}
    }

    /**
     * Adds an extented option numeric value to the buffer.
     */
    static size_t extended_option_value(uint8_t* buf, uint8_t nibble, size_t value)
    {
    	if (nibble==13)
    	{
    		*buf++ = value - 13;
    		return 1;
    	}
    	else if (nibble==14)
    	{
    		value -= 269;
    		*buf++ = uint8_t(value >> 8);
    		*buf++ = uint8_t(value & 0xFF);
    		return 2;
    	}
    	return 0;
    }

    static size_t payload(uint8_t* buf, void* payload, size_t payload_length)
    {
    	size_t result = 0;
    	if (payload && payload_length) {
    		*buf++ = 0xFF;
    		memcpy(buf, payload, payload_length);
    		result = payload_length+1;
    	}
    	return result;
    }
};

// this uses version 0 to maintain compatiblity with the original comms lib codes
#define COAP_MSG_HEADER(type, tokenlen) \
	((CoAP::VERSION)<<6 | (type)<<4 | ((tokenlen) & 0xF))
}}
