/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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
#include "scope_guard.h"

#if HAL_PLATFORM_FILESYSTEM && (HAL_PLATFORM_NRF52840 || HAL_PLATFORM_RTL872X) && !HAL_PLATFORM_PROHIBIT_XIP

void performXipRead() {
    for (uint32_t* addr = (uint32_t*)EXTERNAL_FLASH_XIP_BASE; !exit && addr < (uint32_t*)(EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_SIZE); addr++) {
        // We need to be doing something useful here, so that XIP accesses are not optimized out
        uint32_t result = HAL_Core_Compute_CRC32((const uint8_t*)addr, sizeof(*addr));
        (void)HAL_Core_Compute_CRC32((const uint8_t*)&result, sizeof(result));
    }
}

__attribute__((section(".xip.text"), noinline)) void performXipReadFromXipCode() {
    for (uint32_t* addr = (uint32_t*)EXTERNAL_FLASH_XIP_BASE; !exit && addr < (uint32_t*)(EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_SIZE); addr++) {
        // We need to be doing something useful here, so that XIP accesses are not optimized out
        uint32_t result = HAL_Core_Compute_CRC32((const uint8_t*)addr, sizeof(*addr));
        (void)HAL_Core_Compute_CRC32((const uint8_t*)&result, sizeof(result));
    }
}

test(EXFLASH_00_ConcurrentXipAndWriteErasureUsageStress) {
    std::atomic_bool exit;
    exit = false;

    Thread* t = new Thread("test", [](void* param) -> os_thread_return_t {
        std::atomic_bool& exit = *static_cast<std::atomic_bool*>(param);
        while (!exit) {
            performXipRead();
        }
    }, (void*)&exit);
    assertTrue(t);

    SCOPE_GUARD({
        exit = true;
        t->join();
        delete t;
    });

    // 30 seconds
    constexpr system_tick_t duration = 30 * 1000;

    for (system_tick_t now = millis(), begin = now; now < begin + duration; now = millis()) {
        uint32_t val = rand();
        uint32_t tmp;
        EEPROM.get(0, tmp);

        val = val ^ tmp;
        EEPROM.put(0, val);
        EEPROM.get(0, tmp);
        assertEqual(tmp, val);
    }
}

test(EXFLASH_01_ConcurrentXipCodeAndWriteErasureUsageStress) {
    std::atomic_bool exit;
    exit = false;

    Thread* t = new Thread("test", [](void* param) -> os_thread_return_t {
        std::atomic_bool& exit = *static_cast<std::atomic_bool*>(param);
        while (!exit) {
            performXipReadFromXipCode();
        }
    }, (void*)&exit);
    assertTrue(t);

    SCOPE_GUARD({
        exit = true;
        t->join();
        delete t;
    });

    // 30 seconds
    constexpr system_tick_t duration = 30 * 1000;

    for (system_tick_t now = millis(), begin = now; now < begin + duration; now = millis()) {
        uint32_t val = rand();
        uint32_t tmp;
        EEPROM.get(0, tmp);

        val = val ^ tmp;
        EEPROM.put(0, val);
        EEPROM.get(0, tmp);
        assertEqual(tmp, val);
    }
}

#endif // HAL_PLATFORM_FILESYSTEM && (HAL_PLATFORM_NRF52840 || HAL_PLATFORM_RTL872X) && !HAL_PLATFORM_PROHIBIT_XIP
