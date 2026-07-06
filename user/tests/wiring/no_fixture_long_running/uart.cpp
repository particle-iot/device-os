/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License, either version 3 of the License, or (at your option) any
 * later version.
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
#include "spark_wiring_thread.h"

test(UART_01_TxRingConsistentUnderHighPriorityThreadPreemption)
{
    const system_tick_t UART_STRESS_DURATION_MS = 60 * 1000;
    const uint8_t UART_STRESS_FILL = 0x55;

    Serial1.begin(115200, SERIAL_8N1);
    assertTrue(Serial1.isEnabled());

    volatile bool running = true;
    Thread hpThread("uart_hp", [&]() {
        while (running) {
            delay(1);
        }
    }, OS_THREAD_PRIORITY_NETWORK, OS_THREAD_STACK_SIZE_DEFAULT);

    const size_t txBufferSize = Serial1.availableForWrite();
    assertMoreOrEqual((int)txBufferSize, 64);

    const uint8_t fill[16] = { UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL,
                               UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL,
                               UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL,
                               UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL, UART_STRESS_FILL };
    const system_tick_t start = millis();
    unsigned iterations = 0;
    bool ok = true;

    while (millis() - start < UART_STRESS_DURATION_MS) {
        Serial1.write(fill, sizeof(fill));
        Serial1.flush();
        const int available = Serial1.availableForWrite();
        if ((size_t)available != txBufferSize) {
            ok = false;
            break;
        }
        ++iterations;
    }

    running = false;
    hpThread.dispose();

    Serial1.end();

    assertMoreOrEqual((int)iterations, 1);
    assertTrue(ok);
}
