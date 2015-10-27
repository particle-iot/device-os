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
		THEN("No ping is required at 0")
		{
			Pinger pinger;
			REQUIRE(pinger.process(0, []{return TRANSPORT_FAILURE;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 5000")
		{
			Pinger pinger;
			REQUIRE(pinger.process(5000, []{return TRANSPORT_FAILURE;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 9999")
		{
			Pinger pinger;
			REQUIRE(pinger.process(9999, []{return TRANSPORT_FAILURE;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("No ping is required at 15000")
		{
			Pinger pinger;
			REQUIRE(pinger.process(15000, []{return TRANSPORT_FAILURE;})==NO_ERROR);
			REQUIRE(!pinger.is_expecting_ping_ack());
		}

		THEN("A ping is required at 15001")
		{
			Pinger pinger;
			bool callback_called = false;
			REQUIRE(pinger.process(15001, [&]()->int{callback_called = true; return NO_ERROR;})==NO_ERROR);
			REQUIRE(pinger.is_expecting_ping_ack());
			REQUIRE(callback_called);
		}

	}


	GIVEN("A ping has been sent")
	{
		Pinger pinger;
		bool callback_called = false;
		REQUIRE(pinger.process(15001, [&]()->int{callback_called = true; return NO_ERROR;})==NO_ERROR);
		REQUIRE(pinger.is_expecting_ping_ack());
		REQUIRE(callback_called);

		WHEN("Calling back before 10s")
		{
			THEN("The ping is considered anknowledged.")
			{
				REQUIRE(pinger.process(10000, []{return TRANSPORT_FAILURE;})==NO_ERROR);
				REQUIRE(!pinger.is_expecting_ping_ack());
			}
		}

		WHEN("Calling back after 10s")
		{
			THEN("The ping is considered failed.")
			{
				REQUIRE(pinger.process(10001, []{return TRANSPORT_FAILURE;})==PING_TIMEOUT);
				REQUIRE(pinger.is_expecting_ping_ack());
			}
		}


	}

}
