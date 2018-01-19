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

#if PLATFORM_THREADING

#if PLATFORM_ID == 6

// Regression test for the WICED deadlock in sys_sem_new
// See https://github.com/spark/firmware/pull/984
test(CONCURRENT_01_semaphore_deadlock)
{
    volatile bool run = true;

    // Call malloc in one thread
    Thread allocatorThread("allocator", [&]
    {
        while(run)
        {
            void *ptr = malloc(100);
            free(ptr);
        }
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
    allocatorThread.dispose();
}

#endif // PLATFORM_ID == 6

test(CONCURRENT_02_crc32_is_thread_safe) {
    const unsigned TEST_DURATION = 1000; // 1 second
    const size_t DATA_SIZE = 1024;
    // Allocate a buffer and fill it with random data
    std::unique_ptr<uint8_t[]> data(new uint8_t[DATA_SIZE]);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        data[i] = random(256);
    }
    const uint32_t validCrc = HAL_Core_Compute_CRC32(data.get(), DATA_SIZE);
    // Spawn a couple of threads doing parallel CRC computations
    bool ok = false;
    system_tick_t timeStart = 0;
    const auto threadFn = [&]() {
        do {
            const uint32_t crc = HAL_Core_Compute_CRC32(data.get(), DATA_SIZE);
            if (crc != validCrc) {
                ok = false;
                break;
            }
        } while (millis() - timeStart < TEST_DURATION);
    };
    ok = true;
    timeStart = millis();
    Thread thread1("thread1", threadFn);
    Thread thread2("thread2", threadFn);
    // Wait until the threads finish
    thread1.join();
    thread2.join();
    assertTrue(ok);
}

#endif // PLATFORM_THREADING
