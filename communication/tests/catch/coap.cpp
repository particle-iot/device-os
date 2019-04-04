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

#include "catch.hpp"
#include "coap.h"

using namespace particle::protocol;

SCENARIO("CoAP::code")
{
    CoAP coap;

    unsigned char msg[2];
    
    msg[1] = 0x00;
    REQUIRE(coap.code(msg)==CoAPCode::EMPTY);

    msg[1] = 0x1;
    REQUIRE(coap.code(msg)==CoAPCode::GET);

    msg[1] = 0x2;
    REQUIRE(coap.code(msg)==CoAPCode::POST);

    msg[1] = 0x3;
    REQUIRE(coap.code(msg)==CoAPCode::PUT);

    msg[1] = 0x45;
    REQUIRE(coap.code(msg)==CoAPCode::CONTENT);

    msg[1] = 10;	// todo - this is not a valid CoAP code in the spec
    REQUIRE(coap.code(msg)==CoAPCode::ERROR);

}

SCENARIO("CoAP::header")
{
	GIVEN("A CoAP instance and a 4-byte buffer")
	{
		CoAP coap;
		uint8_t buf[4];

		WHEN("header is called with arguments")
		{
			int size = coap.header(buf, CoAPType::CON, CoAPCode::CONTINUE, 0, nullptr, message_id_t(0x1234));
			THEN("The buffer is filled out correctly")
			{
				REQUIRE(buf[0]==0x40);	// version << 6 (0x40) + Type:CON=0 << 4 + tokenlen 0
				REQUIRE(buf[1]==40);
				REQUIRE(buf[2]==0x12);
				REQUIRE(buf[3]==0x34);
			}
		}
	}

}

SCENARIO("CoAP::path is retrieved from a CoAP message")
{
	GIVEN("a simple message")
	{
		CoAP coap;
		unsigned char msg[] = { 0x40, 40, 0x12, 0x34, 0x91, 'Z', 0 };
		WHEN("requesting the path")
		{
			const unsigned char* path = coap.path(msg);
			THEN("the result is the path")
			{
				REQUIRE(path==msg+5);
			}
		}
	}
}

