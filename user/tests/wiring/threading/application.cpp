/**
 ******************************************************************************
 * @file    application.cpp
 * @authors mat
 * @date    21 January 2015
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "system_threading.h"
#include <functional>

UNIT_TEST_APP();
SYSTEM_THREAD(ENABLED);

volatile int test_val;
void increment(void)
{
	test_val++;
}

test(system_thread_can_pump_events)
{
	WiFi.listen(false);
    test_val = 0;

    ActiveObjectBase* system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system->invoke_async(std::function<void()>(increment));

    uint32_t start = millis();
    while (test_val != 1 && millis() - start < 4000); // Busy wait

    assertEqual((int)test_val, 1);
}

test(application_thread_can_pump_events)
{
	test_val = 0;

	ActiveObjectBase* app = (ActiveObjectBase*)system_internal(0, nullptr); // Returns application thread instance
	std::function<void(void)> fn = increment;
	app->invoke_async(fn);

	// test value not incremented
	assertEqual((int)test_val, 0);

    Particle.process();

    // validate the function was called.
    assertEqual((int)test_val, 1);
}
