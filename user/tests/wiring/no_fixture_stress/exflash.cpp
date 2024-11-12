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

#define PARTICLE_USE_UNSTABLE_API
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#include "storage_hal.h"

#if HAL_PLATFORM_FILESYSTEM && (HAL_PLATFORM_NRF52840 || HAL_PLATFORM_RTL872X) && !HAL_PLATFORM_PROHIBIT_XIP

void performXipRead(std::atomic_bool& exit) {
    for (uint32_t* addr = (uint32_t*)EXTERNAL_FLASH_XIP_BASE; !exit && addr < (uint32_t*)(EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_SIZE); addr++) {
        // We need to be doing something useful here, so that XIP accesses are not optimized out
        uint32_t result = HAL_Core_Compute_CRC32((const uint8_t*)addr, sizeof(*addr));
        (void)HAL_Core_Compute_CRC32((const uint8_t*)&result, sizeof(result));
    }
}

__attribute__((section(".xip.text"), noinline)) void performXipReadFromXipCode(std::atomic_bool& exit) {
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
            performXipRead(exit);
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
            performXipReadFromXipCode(exit);
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

void performNonXipRead(std::atomic_bool& exit) {
    for (uint32_t* addr = (uint32_t*)EXTERNAL_FLASH_XIP_BASE; !exit && addr < (uint32_t*)(EXTERNAL_FLASH_XIP_BASE + EXTERNAL_FLASH_SIZE); addr++) {
        // We need to be doing something useful here, so that XIP accesses are not optimized out
        uint32_t dummy = 0;
        hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, (uintptr_t)addr, (uint8_t*)&dummy, sizeof(dummy));
        uint32_t result = HAL_Core_Compute_CRC32((const uint8_t*)&dummy, sizeof(dummy));
        (void)HAL_Core_Compute_CRC32((const uint8_t*)&result, sizeof(result));
    }
}

test(EXFLASH_02_ConcurrentNonXipReadAndWriteErasureUsageStress) {
    std::atomic_bool exit;
    exit = false;

    Thread* t = new Thread("test", [](void* param) -> os_thread_return_t {
        std::atomic_bool& exit = *static_cast<std::atomic_bool*>(param);
        while (!exit) {
            performNonXipRead(exit);
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

#if HAL_PLATFORM_RTL872X

#include "hal_platform_rtl.h"
extern "C" {
#include "rtl8721d.h"
}

test(EXFLASH_02_rtl872x_validate_mode) {
    RRAM_TypeDef* RRAM = ((RRAM_TypeDef *) RRAM_BASE);
#if PLATFORM_ID == PLATFORM_P2
    // P2/Photon 2 in Dual IO mode
    assertEqual((uint32_t)RRAM->FLASH_ReadMode, (uint32_t)ReadDualIOMode);
#else
    // M SoM in Quad IO mode
    assertEqual((uint32_t)RRAM->FLASH_ReadMode, (uint32_t)ReadQuadIOMode);
#endif // PLATFORM_ID == PLATFORM_P2
    // Highest clock speed (clkdiv=1)
    assertEqual((uint32_t)SPIC->fbaudr, (uint32_t)1);
    assertEqual((uint32_t)SPIC->baudr, (uint32_t)1);
}
#endif // HAL_PLATFORM_RTL872X
