/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "random.h"

void runLoopback(size_t buffer_size_min, size_t buffer_size_max) {
    particle::Random rand;

    size_t bufferSize = random(buffer_size_min, buffer_size_max);
    // Generate random data
    char txBuf[bufferSize + 1] = {};
    rand.genBase32(txBuf, bufferSize);

    Serial1.write(txBuf);
    Serial1.flush();

    size_t pos = 0;
    char rxBuf[bufferSize] = {};
    do {
        size_t available = bufferSize - pos;
        assertEqual(available, Serial1.available());
        if (available) {
            rxBuf[pos] = (char)Serial1.read();
        }
    } while (++pos <= bufferSize);

    assertTrue(!strncmp(txBuf, rxBuf, bufferSize));
}

test(SERIAL_00_LoopbackNoDataLossAndAvailableIsCorrect) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = SERIAL_BUFFER_SIZE / 2;
    constexpr unsigned ITERATIONS = 10000;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE);

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX);
    }
}

test(SERIAL_01_LoopbackSleepWakeupShouldSucceed) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = SERIAL_BUFFER_SIZE / 2;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE);

    int ret = hal_usart_sleep(HAL_USART_SERIAL1, true, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertFalse(Serial1.isEnabled());

    ret = hal_usart_sleep(HAL_USART_SERIAL1, false, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertTrue(Serial1.isEnabled());

    runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX);
}
