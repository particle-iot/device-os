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
#include "protocol_defs.h"

namespace particle
{
namespace protocol
{

inline uint32_t decode_uint32(unsigned char* buf) {
    return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

inline uint16_t decode_uint16(unsigned char* buf) {
    return buf[0] << 8 | buf[1];
}

inline uint8_t decode_uint8(unsigned char* buf) {
    return buf[0];
}

#define RESPONSE_CODE(x,y)  (x<<5 | y)



class Messages
{
public:
	static CoAPMessageType::Enum decodeType(const uint8_t* buf, size_t length)
	{
        if (length<4)
            return CoAPMessageType::ERROR;

        char path = 0;
		size_t path_idx = 5 + (buf[0] & 0x0F);
        if (path_idx<length)
			 path = buf[path_idx];

		switch (CoAP::code(buf))
		{
		case CoAPCode::GET:
			switch (path)
			{
			case 'v':
				return CoAPMessageType::VARIABLE_REQUEST;
			case 'd':
				return CoAPMessageType::DESCRIBE;
			default:
				break;
			}
			break;
		case CoAPCode::POST:
			switch (path)
			{
			case 'E':
			case 'e':
				return CoAPMessageType::EVENT;
			case 'h':
				return CoAPMessageType::HELLO;
			case 'f':
				return CoAPMessageType::FUNCTION_CALL;
			case 's':
				return CoAPMessageType::SAVE_BEGIN;
			case 'u':
				return CoAPMessageType::UPDATE_BEGIN;
			case 'c':
				return CoAPMessageType::CHUNK;
			default:
				break;
			}
			break;
		case CoAPCode::PUT:
			switch (path)
			{
			case 'k':
				return CoAPMessageType::KEY_CHANGE;
			case 'u':
				return CoAPMessageType::UPDATE_DONE;
			case 's':
				// todo - use a single message SIGNAL and decode the rest of the message to determine desired state
				if (buf[8])
					return CoAPMessageType::SIGNAL_START;
				else
					return CoAPMessageType::SIGNAL_STOP;
			default:
				break;
			}
			break;
		case CoAPCode::EMPTY:
			switch (CoAP::type(buf))
			{
			case CoAPType::CON:
				return CoAPMessageType::PING;
			default:
				return CoAPMessageType::EMPTY_ACK;
			}
			break;
		case CoAPCode::CONTENT:
			return CoAPMessageType::TIME;
		default:
			break;
		}
		return CoAPMessageType::ERROR;
	}

	static size_t hello(uint8_t* buf, message_id_t message_id, uint8_t flags,
			uint16_t platform_id, uint16_t product_id,
			uint16_t product_firmware_version, bool confirmable=false)
	{
		buf[0] = COAP_MSG_HEADER(confirmable ? CoAPType::CON : CoAPType::NON, 0);
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

	static size_t update_done(uint8_t* buf, message_id_t message_id)
	{
		buf[0] = 0x50; // non-confirmable, no token
		buf[1] = 0x02; // POST
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		buf[4] = 0xb1; // Uri-Path option of length 1
		buf[5] = 'u';
		return 6;
	}

	static const size_t function_return_size = 10;

	static size_t function_return(unsigned char *buf, message_id_t message_id, token_t token, int return_value)
	{
		buf[0] = 0x51; // non-confirmable, one-byte token
		buf[1] = 0x44; // response code 2.04 CHANGED
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		buf[4] = token;
		buf[5] = 0xff; // payload marker
		buf[6] = return_value >> 24;
		buf[7] = return_value >> 16 & 0xff;
		buf[8] = return_value >> 8 & 0xff;
		buf[9] = return_value & 0xff;
		return function_return_size;
	}

	static size_t variable_value(unsigned char *buf, message_id_t message_id, token_t token, bool return_value)
	{
		size_t size = content(buf, message_id, token);
		buf[size++] = return_value ? 1 : 0;
		return size;
	}

	static size_t variable_value(unsigned char *buf, message_id_t message_id,
			token_t token, int return_value)
	{
		size_t size = content(buf, message_id, token);
		buf[size++] = return_value >> 24;
		buf[size++] = return_value >> 16 & 0xff;
		buf[size++] = return_value >> 8 & 0xff;
		buf[size++] = return_value & 0xff;
		return size;
	}

	static size_t variable_value(unsigned char *buf, message_id_t message_id,
			token_t token, double return_value)
	{
		size_t size = content(buf, message_id, token);
		memcpy(buf + size, &return_value, 8);
		return size+sizeof(double);
	}

	// Returns the length of the buffer to send
	static size_t variable_value(unsigned char *buf, message_id_t message_id,
			token_t token, const void *return_value, int length)
	{
		size_t size = content(buf, message_id, token);
		memcpy(buf + size, return_value, length);
		return size + length;
	}

	static size_t time_request(uint8_t* buf, uint16_t message_id, uint8_t token)
	{
		unsigned char *p = buf;

		*p++ = 0x41; // Confirmable, one-byte token
		*p++ = 0x01; // GET request

		*p++ = message_id >> 8;
		*p++ = message_id & 0xff;

		*p++ = token;
		*p++ = 0xb1; // One-byte, Uri-Path option
		*p++ = 't';

		return p - buf;
	}

	static size_t chunk_missed(uint8_t* buf, uint16_t message_id, chunk_index_t chunk_index)
	{
		buf[0] = 0x40; // confirmable, no token
		buf[1] = 0x01; // code 0.01 GET
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		buf[4] = 0xb1; // one-byte Uri-Path option
		buf[5] = 'c';
		buf[6] = 0xff; // payload marker
		buf[7] = chunk_index >> 8;
		buf[8] = chunk_index & 0xff;
		return 9;
	}

	static size_t content(uint8_t* buf, uint16_t message_id, uint8_t token)
	{
		buf[0] = 0x61; // acknowledgment, one-byte token
		buf[1] = 0x45; // response code 2.05 CONTENT
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		buf[4] = token;
		buf[5] = 0xff; // payload marker
		return 6;
	}

	static size_t ping(uint8_t* buf, uint16_t message_id)
	{
		buf[0] = 0x40; // Confirmable, no token
		buf[1] = 0x00; // code signifying empty message
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		return 4;
	}

	static size_t presence_announcement(unsigned char *buf, const char *id)
	{
		buf[0] = 0x50; // Confirmable, no token
		buf[1] = 0x02; // Code POST
		buf[2] = 0x00; // message id ignorable in this context
		buf[3] = 0x00;
		buf[4] = 0xb1; // Uri-Path option of length 1
		buf[5] = 'h';
		buf[6] = 0xff; // payload marker
		memcpy(buf + 7, id, 12);
		return 19;
	}

	static size_t separate_response_with_payload(unsigned char *buf, uint16_t message_id,
			unsigned char token, unsigned char code, unsigned char* payload,
			unsigned payload_len)
	{
		buf[0] = 0x51; // non-confirmable, one-byte token
		buf[1] = code;
		buf[2] = message_id >> 8;
		buf[3] = message_id & 0xff;
		buf[4] = token;

		size_t len = 5;
		// for now, assume the payload is less than 9
		if (payload && payload_len)
		{
			buf[5] = 0xFF;
			memcpy(buf + 6, payload, payload_len);
			len += 1 + payload_len;
		}
		return len;
	}

    static inline size_t empty_ack(unsigned char *buf,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb) {
        return coded_ack(buf, 0, message_id_msb, message_id_lsb);
    };

    static inline size_t coded_ack(unsigned char *buf,
                                         unsigned char code,
                                         unsigned char message_id_msb,
                                         unsigned char message_id_lsb
                                         )
    {
      buf[0] = 0x60; // acknowledgment, no token
      buf[1] = code;
      buf[2] = message_id_msb;
      buf[3] = message_id_lsb;
      return 4;
    }

    static inline size_t coded_ack(unsigned char *buf,
                                         unsigned char token,
                                         unsigned char code,
                                         unsigned char message_id_msb,
                                         unsigned char message_id_lsb)
    {
      buf[0] = 0x61; // acknowledgment, one-byte token
      buf[1] = code;
      buf[2] = message_id_msb;
      buf[3] = message_id_lsb;
      buf[4] = token;
      return 5;
    }


    static inline size_t update_ready(unsigned char *buf, message_id_t message_id, token_t token)
    {
        return separate_response_with_payload(buf, message_id, token, 0x44, NULL, 0);
    }

    static inline size_t update_ready(unsigned char *buf, message_id_t message_id, token_t token, uint8_t flags)
    {
        return separate_response_with_payload(buf, message_id, token, 0x44, &flags, 1);
    }

    static inline size_t chunk_received(unsigned char *buf, message_id_t message_id, token_t token, ChunkReceivedCode::Enum code)
    {
       return separate_response(buf, message_id, token, code);
    }

    static inline size_t separate_response(unsigned char *buf, message_id_t message_id,
                                          unsigned char token, unsigned char code)
    {
        return separate_response_with_payload(buf, message_id, token, code, NULL, 0);
    }


    static inline size_t description(unsigned char *buf, token_t token,
                                   message_id_t message_id)
    {
    		return content(buf, message_id, token);
    }

};


}}
