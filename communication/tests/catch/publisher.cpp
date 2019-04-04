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

#include "publisher.h"

#include "catch.hpp"

using namespace particle::protocol;

SCENARIO("publisher")
{
	GIVEN("a publisher")
	{
		Protocol* protocol = nullptr;
		Publisher publisher(protocol);

		WHEN("4 application events are sent within 1 second")
		{
			REQUIRE(publisher.is_rate_limited(false, 1000)==false);
			REQUIRE(publisher.is_rate_limited(false, 1200)==false);
			REQUIRE(publisher.is_rate_limited(false, 1400)==false);
			REQUIRE(publisher.is_rate_limited(false, 1600)==false);

			const system_tick_t next_app_event = 5000;  // 1000ms + 4s
			THEN("application events until 4 seconds have elapsed are rate limited")
			{
				for (system_tick_t i=1600; i<next_app_event; i+=100) {
					REQUIRE(publisher.is_rate_limited(false, i)==true);
				}
			}

			THEN("an application event after 4 seconds have elapsed is not rate limited")
			{
				REQUIRE(publisher.is_rate_limited(false, next_app_event)==false);
			}
		}

		WHEN("255 system events are sent in less than a minute")
		{
			for (int i=0; i<255; i++) {
				INFO("The counter is " << i);
				REQUIRE(publisher.is_rate_limited(true, i)==false);
			}

			THEN("all system events until the next minute begins are rate limited")
			{
				for (int i=1000; i<60*1000; i+=1000) {
					INFO("The counter is " << i);
					REQUIRE(publisher.is_rate_limited(true, i)==true);
				}

				// it's only approximately 1 minute, the cutoff is 64k milliseconds
				for (int i=60000; i<65536; i+=8) {
					INFO("The counter is " << i);
					REQUIRE(publisher.is_rate_limited(true, i)==true);
				}

				AND_THEN("system events in the next minute are not rate limited")
				{
					for (int i=65536; i<65536+255; i++) {
						INFO("The counter is " << i);
						REQUIRE(publisher.is_rate_limited(true, i)==false);
					}
				}
			}

			THEN("application events are still rate limited after a burst of 4")
			{
				REQUIRE(publisher.is_rate_limited(false, 1000)==false);
				REQUIRE(publisher.is_rate_limited(false, 1000)==false);
				REQUIRE(publisher.is_rate_limited(false, 1000)==false);
				REQUIRE(publisher.is_rate_limited(false, 1000)==false);
				REQUIRE(publisher.is_rate_limited(false, 1000)==true);
			}
		}
	}
}
