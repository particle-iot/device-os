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

#include "messages.h"

#include "catch.hpp"

using namespace spark::protocol;

SCENARIO("determining message type from a CoAP GET message")
{
	WHEN("a message is formatted as DESCRIBE")
	{
		uint8_t buf[] = { 1, 1, 0, 0, 0, 0x91, 'd'};
		THEN("then message is recognized as a DESCRIBE message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::DESCRIBE);
		}
	}


	WHEN("a message is formatted as VARIABLE_REQUEST")
	{
		uint8_t buf[] = { 1, 1, 0, 0, 0, 0x91, 'v'};
		THEN("then message is recognized as a VARIABLE_REQUEST message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::VARIABLE_REQUEST);
		}
	}


	WHEN("a message is an unknown GET request")
	{
		uint8_t buf[] = { 1, 1, 0, 0, 0, 0x91, '!'};
		THEN("then message is recognized as a ERROR")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::ERROR);
		}
	}
}


SCENARIO("determining message type from a CoAP POST message")
{
	WHEN("a message is formatted as EVENT")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'E'};
		THEN("then message is recognized as a EVENT message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::EVENT);
		}
	}

	WHEN("a message is formatted as EVENT")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'e'};
		THEN("then message is recognized as a EVENT message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::EVENT);
		}
	}

	WHEN("a message is formatted as HELLO")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'h'};
		THEN("then message is recognized as a HELLO message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::HELLO);
		}
	}

	WHEN("a message is formatted as FUNCTION_CALL")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'f'};
		THEN("then message is recognized as a FUNCTION_CALL message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::FUNCTION_CALL);
		}
	}

	WHEN("a message is formatted as UPDATE_BEGIN")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'u'};
		THEN("then message is recognized as a UPDATE_BEGIN message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::UPDATE_BEGIN);
		}
	}

	WHEN("a message is formatted as SAVE_BEGIN")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 's'};
		THEN("then message is recognized as a SAVE_BEGIN message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::SAVE_BEGIN);
		}
	}

	WHEN("a message is formatted as CHUNK")
	{
		uint8_t buf[] = { 1, 2, 0, 0, 0, 0x91, 'c'};
		THEN("then message is recognized as a CHUNK message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::CHUNK);
		}
	}

	WHEN("a message is an unknown POST request")
	{
		uint8_t buf[] = { 1, 1, 0, 0, 0, 0x91, '!'};
		THEN("then message is recognized as a ERROR")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::ERROR);
		}
	}
}



SCENARIO("determining message type from a CoAP PUT message")
{
	WHEN("a message is formatted as KEY_CHANGE")
	{
		uint8_t buf[] = { 1, 3, 0, 0, 0, 0x91, 'k'};
		THEN("then message is recognized as a KEY_CHANGE message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::KEY_CHANGE);
		}
	}

	WHEN("a message is formatted as UPDATE_DONE")
	{
		uint8_t buf[] = { 1, 3, 0, 0, 0, 0x91, 'u'};
		THEN("then message is recognized as a UPDATE_DONE message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::UPDATE_DONE);
		}
	}

	WHEN("a message is formatted as SIGNAL_START")
	{
		uint8_t buf[] = { 1, 3, 0, 0, 0, 0x91, 's', 0xFF, 1 };
		THEN("then message is recognized as a SIGNAL_START message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::SIGNAL_START);
		}
	}

	WHEN("a message is formatted as SIGNAL_STOP")
	{
		uint8_t buf[] = { 1, 3, 0, 0, 0, 0x91, 's', 0xFF, 0 };
		THEN("then message is recognized as a SIGNAL_STOP message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::SIGNAL_STOP);
		}
	}

	WHEN("a message is an unknown PUT request")
	{
		uint8_t buf[] = { 1, 3, 0, 0, 0, 0x91, '!'};
		THEN("then message is recognized as a ERROR")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::ERROR);
		}
	}
}


SCENARIO("determining message type from a CoAP EMPTY message")
{
	WHEN("a Confirmable message with no method is received")
	{
		uint8_t buf[] = { 1, 0 };
		THEN("then message is recognized as a PING message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::PING);
		}
	}

	WHEN("a Non-confirmable message is sent with no method")
	{
		uint8_t buf[] = { 0x31, 0 };
		THEN("then message is recognized as a PING message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::EMPTY_ACK);
		}
	}
}


SCENARIO("determining message type from a CoAP Content message")
{
	WHEN("a Content message is received")
	{
		uint8_t buf[] = { 1, 0x45 };
		THEN("then message is recognized as a TIME message")
		{
			REQUIRE(Messages::decodeType(buf)==CoAPMessageType::TIME);
		}
	}
}

SCENARIO("production of request messages")
{
	GIVEN("")
	{

	}

}
