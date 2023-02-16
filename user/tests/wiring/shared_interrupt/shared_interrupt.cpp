/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#if HAL_PLATFORM_SHARED_INTERRUPT

constexpr uint8_t handlers = 3;
uint32_t record[handlers] = { 0 };
uint8_t idx = 0;

void handler(void* data) {
    record[idx++] = (uint32_t)data;
    idx %= handlers;
}

void handler1(void* data) {
    handler(data);
}

test(01_Shared_Interrupt_Execute_Handlers_As_Expected) {
    constexpr uint8_t trigPin = TX;
    constexpr uint8_t intPin = WKP;
    const InterruptMode mode = FALLING;

    Serial.println("Please connect TX with WKP and then press any key to start the tests.");
    while (Serial.available() <= 0);
    while (Serial.available() > 0) {
        (void)Serial.read();
    }

    hal_gpio_mode(trigPin, OUTPUT);
    hal_gpio_write(trigPin, HIGH);

    hal_gpio_mode(intPin, INPUT_PULLUP);
    hal_interrupt_extra_configuration_t config = {};
    config.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION;
    config.appendHandler = 1;

    config.chainPriority = 1;
    hal_interrupt_attach(intPin, handler, (void*)0x12345678, mode, &config);

    config.chainPriority = 3;
    hal_interrupt_attach(intPin, handler, (void*)0xDEADBEEF, mode, &config);

    config.chainPriority = 2;
    hal_interrupt_attach(intPin, handler1, (void*)0x55AA55AA, mode, &config);

    // Generate falling edge
    hal_gpio_write(trigPin, LOW);
    HAL_Delay_Milliseconds(1);

    assertEqual(record[0], 0x12345678);
    assertEqual(record[1], 0x55AA55AA);
    assertEqual(record[2], 0xDEADBEEF);

    hal_gpio_write(trigPin, HIGH);
    HAL_Delay_Milliseconds(100);
    memset(record, 0x0, sizeof(record));

    hal_interrupt_detach_ext(intPin, false, (void*)handler1);

    // Generate falling edge
    hal_gpio_write(trigPin, LOW);
    HAL_Delay_Milliseconds(1);
    
    assertEqual(record[0], 0x12345678);
    assertEqual(record[1], 0xDEADBEEF);
    assertEqual(record[2], 0x0);

    hal_gpio_write(trigPin, HIGH);
    HAL_Delay_Milliseconds(100);
    memset(record, 0x0, sizeof(record));

    hal_interrupt_detach_ext(intPin, false, (void*)handler);

    // Generate falling edge
    hal_gpio_write(trigPin, LOW);
    HAL_Delay_Milliseconds(1);
    
    assertEqual(record[0], 0x0);
    assertEqual(record[1], 0x0);
    assertEqual(record[2], 0x0);
}

#endif // HAL_PLATFORM_SHARED_INTERRUPT
