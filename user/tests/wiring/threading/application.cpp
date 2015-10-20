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

void* abc = NULL;

UNIT_TEST_APP();
SYSTEM_THREAD(ENABLED);

int test_val;
void increment(void)
{
	test_val++;
}

test(application_thread_can_pump_events)
{
	test_val = 0;

	ActiveObjectBase* app = (ActiveObjectBase*)system_internal(0, nullptr);
	std::function<void(void)> fn = increment;
	app->invoke_async(fn);

	// test value not incremented
	assertEqual(test_val, 0);

    Particle.process();

    // validate the function was called.
    assertEqual(test_val, 1);

}

uint32_t last_listen_event = 0;
void app_listening(system_event_t event, uint32_t time, void*)
{
	last_listen_event = time;
	if (time>2000)
		WiFi.listen(false); // exit listening mode
}

// This test ensures the RTOS time slices between threads when the system is in listening mode.
// It's not a complete test, since we never exit or re-enter loop so the loop-calling logic isn't exercised.
// This test indirectly ensures the system thread also runs, since if it didn't then the listen update events wouldn't be
// published.
test(application_thread_runs_during_listening_mode)
{
	System.on(wifi_listen_update, app_listening);

	uint32_t start = millis();
	WiFi.listen();
	delay(10);		// time for the system thread to enter listening mode

	while (millis()-start<1000);		// busy wait 1000 ms

	uint32_t end = millis();
	assertLess(end-start, 1200);		// small margin of error
	assertMore(last_listen_event, 0);	// system event should have ran the listen loop

	System.off(app_listening);
}

