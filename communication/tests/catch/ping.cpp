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

#include "ping.h"

#include "catch.hpp"

using namespace particle::protocol;

SCENARIO("ping requests and responses are managed")
{
	GIVEN("A ping has not been sent")
	{
		Pinger pinger;
		pinger.init(15000, 10000);

		THEN("No ping is required at 0")
		{
			REQUIRE(pinger.process(0, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 5000")
		{
			REQUIRE(pinger.process(5000, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 9999")
		{
			REQUIRE(pinger.process(9999, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 15000")
		{
			REQUIRE(pinger.process(15000, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("A ping is required at 15001")
		{
			bool callback_called = false;
			REQUIRE(pinger.process(15001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}

		THEN("The ping interval can be updated by the SYSTEM")
		{
			pinger.set_interval(30000, KeepAliveSource::SYSTEM);
			REQUIRE(pinger.process(15001, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());

			bool callback_called = false;
			REQUIRE(pinger.process(30001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}

		THEN("The ping interval can be updated by the USER")
		{
			pinger.set_interval(30000, KeepAliveSource::USER);
			REQUIRE(pinger.process(15001, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());

			bool callback_called = false;
			REQUIRE(pinger.process(30001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}

		THEN("The ping interval can be updated by the SYSTEM and then the USER")
		{
			pinger.set_interval(30000, KeepAliveSource::SYSTEM);
			REQUIRE(pinger.process(15001, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());

			pinger.set_interval(60000, KeepAliveSource::USER);
			REQUIRE(pinger.process(30001, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());

			bool callback_called = false;
			REQUIRE(pinger.process(60001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}

		THEN("The ping interval can be updated by the USER but then not the SYSTEM")
		{
			pinger.set_interval(30000, KeepAliveSource::USER);
			REQUIRE(pinger.process(15001, []{return IO_ERROR;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());

			pinger.set_interval(60000, KeepAliveSource::SYSTEM);

			bool callback_called = false;
			REQUIRE(pinger.process(30001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}
	}


	GIVEN("A ping has been sent")
	{
		Pinger pinger;
		pinger.init(15000,10000);
		bool callback_called = false;
		REQUIRE(!pinger.is_expecting_ping_ack());
		REQUIRE(pinger.process(15001, [&]()->ProtocolError{callback_called = true; return NO_ERROR;})==NO_ERROR);
		REQUIRE(pinger.is_expecting_ping_ack());
		REQUIRE(callback_called);

		WHEN("Calling back before 10s")
		{
			THEN("The ping is not considered acknowledge")
			{
				// ping callback is not used
				REQUIRE(pinger.process(10000, []{return IO_ERROR;})==NO_ERROR);
				REQUIRE(pinger.is_expecting_ping_ack());
			}

			AND_WHEN("A message is received at the timeout interval")
			{
				REQUIRE(pinger.process(10000, []{return IO_ERROR;})==NO_ERROR);
				pinger.message_received();
				THEN("the ping request is acknowledged")
				{
					REQUIRE(!pinger.is_expecting_ping_ack());
				}
			}
		}

		WHEN("Calling back after 10s")
		{
			THEN("The ping is considered failed.")
			{
				REQUIRE(pinger.process(10001, []{return IO_ERROR;})==PING_TIMEOUT);
				REQUIRE(pinger.is_expecting_ping_ack());
			}
		}

	}

}
