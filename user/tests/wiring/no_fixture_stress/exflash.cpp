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

namespace {

#if HAL_PLATFORM_RTL872X
std::pair<hal_storage_id, uintptr_t> getOtaStorage() {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0 /* flags */, nullptr /* reserved */);
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr /* reserved */);
    });
    uintptr_t addr = 0;
    for (size_t i = 0; i < info.module_count; ++i) {
        const auto& module = info.modules[i];
        if (module.info.module_function == MODULE_FUNCTION_SYSTEM_PART) {
            uintptr_t end = (uintptr_t)module.info.module_end_address;
            end += 4; // CRC32
            end = ((end) & 0xFFFFF000) + 0x1000; // 4K aligned
            addr = end - EXTERNAL_FLASH_XIP_BASE;
            break;
        }
    }
    return {HAL_STORAGE_ID_EXTERNAL_FLASH, addr};
}
#elif HAL_PLATFORM_NRF52840
std::pair<hal_storage_id, uintptr_t> getOtaStorage() {
    return {HAL_STORAGE_ID_EXTERNAL_FLASH, EXTERNAL_FLASH_OTA_ADDRESS};
}
#else
#error "Unsupported platform"
#endif

}

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
        hal_storage_read(HAL_STORAGE_ID_EXTERNAL_FLASH, (uintptr_t)addr - EXTERNAL_FLASH_XIP_BASE, (uint8_t*)&dummy, sizeof(dummy));
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

test(EXFLASH_03_rtl872x_validate_mode_after_stop_sleep_and_read_write) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(10s));
    assertEqual(result.error(), SYSTEM_ERROR_NONE);

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

    auto duration = 10 * 1000;

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

test(EXFLASH_04_rtl872x_validate_mode_after_hibernate_sleep_and_read_write) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    SystemSleepResult result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::HIBERNATE).duration(10s));
    assertEqual(result.error(), SYSTEM_ERROR_NONE);
}

test(EXFLASH_05_rtl872x_validate_mode_after_hibernate_sleep_and_read_write) {
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);

    auto duration = 10 * 1000;

    for (system_tick_t now = millis(), begin = now; now < begin + duration; now = millis()) {
        uint32_t val = rand();
        uint32_t tmp;
        EEPROM.get(0, tmp);

        val = val ^ tmp;
        EEPROM.put(0, val);
        EEPROM.get(0, tmp);
        assertEqual(tmp, val);
    }

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

test(EXFLASH_03_aligned_and_unaligned_writes_from_internal_flash_work) {
    auto [storage, addr] = getOtaStorage();
    assertNotEqual(addr, (uintptr_t)0);
    assertEqual(addr % 4, 0);

    static const uint8_t dummy[8192 + 4] __attribute__((aligned(4))) = {0xff, 0xff, 0xff, 0xff};

    for (size_t iOffset = 0; iOffset < 4; iOffset++) {
        for (size_t oOffset = 0; oOffset  < 4; oOffset++) {
            assertEqual(sizeof(dummy) - iOffset, hal_storage_write(storage, addr + oOffset, dummy + iOffset, sizeof(dummy) - iOffset));
        }
    }
}
