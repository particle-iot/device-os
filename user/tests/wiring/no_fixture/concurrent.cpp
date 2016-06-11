/**
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#if PLATFORM_ID == 6

// Regression test for the WICED deadlock in sys_sem_new
// See https://github.com/spark/firmware/pull/984
Thread allocatorThread;
test(concurrent_semaphore_deadlock)
{
    volatile bool run = true;

    // Call malloc in one thread
    allocatorThread = Thread("allocator", [&]
    {
        while(run)
        {
            void *ptr = malloc(100);
            free(ptr);
        }
        allocatorThread.dispose();
    });

    // Create WICED connections in one thread
    unsigned long start = millis();
    while(millis() - start < 2000)
    {
        WiFi.resolve("www.particle.io");
        Particle.process();
    }

    run = false;

    // No assertion needed. If the test finishes there was no deadlock
}

#endif
