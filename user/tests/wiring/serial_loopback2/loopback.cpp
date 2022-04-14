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

#ifndef USE_BUFFER_SIZE
#warning "Using default 64 byte buffer"
#define USE_BUFFER_SIZE (SERIAL_BUFFER_SIZE)
#else
hal_usart_buffer_config_t acquireSerial1Buffer()
{
#if !HAL_PLATFORM_USART_9BIT_SUPPORTED
    const size_t bufferSize = USE_BUFFER_SIZE;
#else
    const size_t bufferSize = USE_BUFFER_SIZE * sizeof(uint16_t);
#endif // HAL_PLATFORM_USART_9BIT_SUPPORTED

    hal_usart_buffer_config_t config = {
        .size = sizeof(hal_usart_buffer_config_t),
        .rx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .rx_buffer_size = bufferSize,
        .tx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .tx_buffer_size = bufferSize
    };

    return config;
}
#endif // USE_BUFFER_SIZE

void runLoopback(size_t buffer_size_min, size_t buffer_size_max, bool sleep, bool ninebit = false) {
    particle::Random rand;

    size_t bufferSize = random(buffer_size_min, buffer_size_max);
    // Generate random data
    char txBuf[bufferSize + 1] = {};
    rand.genBase32(txBuf, bufferSize);

    for (int i = 0; i < bufferSize; i++) {
        uint16_t c = txBuf[i];
        if (ninebit) {
            c |= 0b100000000;
        }
        Serial1.write(c);
    }
    Serial1.flush();

    if (sleep) {
        int ret = hal_usart_sleep(HAL_USART_SERIAL1, true, nullptr);
        assertEqual(ret, (int)SYSTEM_ERROR_NONE);
        assertFalse(Serial1.isEnabled());

        ret = hal_usart_sleep(HAL_USART_SERIAL1, false, nullptr);
        assertEqual(ret, (int)SYSTEM_ERROR_NONE);
        assertTrue(Serial1.isEnabled());
    }

    size_t pos = 0;
    char rxBuf[bufferSize] = {};
    do {
        size_t available = bufferSize - pos;
        assertEqual(available, Serial1.available());
        if (available) {
            uint16_t c = Serial1.read();
            if (ninebit) {
                assertTrue(c & 0b100000000);
            } else {
                assertFalse(c & 0b100000000);
            }
            rxBuf[pos] = (char)c;
        }
    } while (++pos <= bufferSize);

    assertTrue(!strncmp(txBuf, rxBuf, bufferSize));
}

test(SERIAL_00_LoopbackNoDataLossAndAvailableIsCorrect) {
    const size_t TEST_BUFFER_SIZE_MIN = 8;
    const size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    const unsigned ITERATIONS = 10000;
    const unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE);

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, false);
    }
}

test(SERIAL_01_LoopbackSleepWakeupShouldSucceed) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    constexpr unsigned ITERATIONS = 10000;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE);

    int ret = hal_usart_sleep(HAL_USART_SERIAL1, true, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertFalse(Serial1.isEnabled());

    ret = hal_usart_sleep(HAL_USART_SERIAL1, false, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertTrue(Serial1.isEnabled());

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, false);
    }
}

test(SERIAL_02_LoopbackReceivedDataShouldRetainAfterSleepWakeup) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    constexpr unsigned ITERATIONS = 10000;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE);

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, true);
    }
}

#if HAL_PLATFORM_USART_9BIT_SUPPORTED
test(SERIAL_03_Loopback9BitNoDataLossAndAvailableIsCorrect) {
    const size_t TEST_BUFFER_SIZE_MIN = 8;
    const size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    const unsigned ITERATIONS = 10000;
    const unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE, SERIAL_9N1);

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, false);
    }
}

test(SERIAL_04_Loopback9BitSleepWakeupShouldSucceed) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    constexpr unsigned ITERATIONS = 10000;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE, SERIAL_9N1);

    int ret = hal_usart_sleep(HAL_USART_SERIAL1, true, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertFalse(Serial1.isEnabled());

    ret = hal_usart_sleep(HAL_USART_SERIAL1, false, nullptr);
    assertEqual(ret, (int)SYSTEM_ERROR_NONE);
    assertTrue(Serial1.isEnabled());

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, false);
    }
}

test(SERIAL_05_Loopback9BitReceivedDataShouldRetainAfterSleepWakeup) {
    constexpr size_t TEST_BUFFER_SIZE_MIN = 8;
    constexpr size_t TEST_BUFFER_SIZE_MAX = USE_BUFFER_SIZE / 2;
    constexpr unsigned ITERATIONS = 10000;
    constexpr unsigned BAUD_RATE = 115200;

    Serial1.end();
    Serial1.begin(BAUD_RATE, SERIAL_9N1);

    for (unsigned i = 0; i < ITERATIONS; ++i) {
        runLoopback(TEST_BUFFER_SIZE_MIN, TEST_BUFFER_SIZE_MAX, true);
    }
}

#endif // HAL_PLATFORM_USART_9BIT_SUPPORTED