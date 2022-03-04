/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include <cstdlib>
extern "C" {
#include "rtl8721d.h"
}
#include "module_info.h"
#include "hw_config.h"
#include "bootloader_update.h"
#include "rtl_it.h"
#include "flash_common.h"

__attribute__((used)) void* dynalib_table_location = 0; // part1 dynalib location

extern uintptr_t link_part1_flash_start;
extern uintptr_t link_part1_flash_end;
extern uintptr_t link_part1_flash_size;
extern uintptr_t link_part1_module_info_flash_start;
extern uintptr_t link_part1_dynalib_table_flash_start;
extern uintptr_t link_part1_dynalib_table_ram_start;

extern "C" {
extern u32 ConfigDebugClose;
int bootloader_part1_preinit(void);
int bootloader_part1_init(void);
int bootloader_part1_setup(void);
int bootloader_part1_loop(void);
}

static bool isPart1ImageValid() {
    module_info_t info = {};
    _memcpy(&info, &link_part1_module_info_flash_start, sizeof(module_info_t));
    if (((uint32_t)info.module_start_address == (uint32_t)&link_part1_flash_start)
            && ((uint32_t)info.module_end_address <= (uint32_t)&link_part1_flash_end)
            && (info.platform_id == PLATFORM_ID)) {
        uint32_t length = (uint32_t)info.module_end_address - (uint32_t)info.module_start_address;
        uint32_t crc = Compute_CRC32((const uint8_t*)info.module_start_address, length, nullptr);
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)((uint32_t)info.module_start_address + length)));
        return (crc == expectedCRC);
    }
    return false;
}

RSIP_MaskDef rsipMask[] = {
    /*MaskAddr,     MaskSize*/
    {0x08002000,    2},     //Entry 0: 4K system data & 4K backup
    /* customer can set here */
    {0x0805F000,    1},     //Entry 1: boot info sector
    {0xFFFFFFFF,    0xFF},  //Entry 2: Used to mask part1 image range when image is unencrypted
    {0xFFFFFFFF,    0xFF},  //Entry 3: Reserved by Realtek. If RDP is not used, this entry can be used by users.
};

extern "C" void rsipMaskConfig(void) {
    const uint8_t PART1_RSIP_MASK_INDEX = 2;

    // Check efuse bit for `part1 encrypted` indication
    uint8_t userEfuse0 = 0;
    bool success = (EFUSE_PMAP_READ8(0, USER_KEY_0_EFUSE_ADDRESS, &userEfuse0, L25EOUTVOLTAGE) != 0);

    // If bit still set -> part1 is NOT encrypted -> add part1 to RSIP mask
    if(success && (userEfuse0 & PART1_ENCRYPTED_BIT)) {
        rsipMask[PART1_RSIP_MASK_INDEX].MaskAddr = (uint32_t) &link_part1_flash_start;
        rsipMask[PART1_RSIP_MASK_INDEX].MaskSize = (uint8_t) CEIL_DIV(((uint32_t)&link_part1_flash_size), INTERNAL_FLASH_PAGE_SIZE);
    }
    else {
        // If part1 IS encrypted, then we want to make sure the RSIP mask is disabled for the slot
        rsipMask[PART1_RSIP_MASK_INDEX].MaskAddr = 0xFFFFFFFF;
        rsipMask[PART1_RSIP_MASK_INDEX].MaskSize = 0xFF;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (rsipMask[i].MaskAddr != 0xFFFFFFFF) {
            if ((rsipMask[i].MaskAddr >= SPI_FLASH_BASE) && (rsipMask[i].MaskAddr <= 0x0FFFFFFF)) {
                RSIP_OTF_Mask(i, rsipMask[i].MaskAddr, rsipMask[i].MaskSize, ENABLE);
            }
        } else {
            RSIP_OTF_Mask(i, 0, 0, DISABLE);
        }
    }
}

extern "C" int main() {
    // Override the fault handlers in case of getting stuck in the ROM
    __NVIC_SetVector(HardFault_IRQn_LP, (uint32_t)HardFault_Handler);
    __NVIC_SetVector(MemoryManagement_IRQn_LP, (uint32_t)MemManage_Handler);
    __NVIC_SetVector(BusFault_IRQn_LP, (uint32_t)BusFault_Handler);
    __NVIC_SetVector(UsageFault_IRQn_LP, (uint32_t)UsageFault_Handler);

    /*
     * FIXME: Do NOT allocate memory from heap in MBR, since the heap start address is incorrect!
     * As a workaround, we can use AtomicSimpleStaticPool instead.
     */
    ConfigDebugClose = 1;

    if ((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) != 0) {
        rsipMaskConfig();
    }

    if (!bootloaderUpdateIfPending()) {
        NVIC_SystemReset();
    }

    if ((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) != 0) {
        rsipMaskConfig();
    }

    if (!isPart1ImageValid()) {
        while (true) {
            __WFE();
        }
    }

    // dynalib table point to flash
    dynalib_table_location = &link_part1_dynalib_table_flash_start;
    bootloader_part1_preinit();

    // dynalib table point to SRAM
    dynalib_table_location = &link_part1_dynalib_table_ram_start;
    bootloader_part1_init();

    bootloader_part1_setup();

    while (true) {
        __SEV(); // signal event, also signal to KM4
        __WFE(); // clear event, immediately exits
        __WFE(); // sleep, waiting for event

        bootloader_part1_loop();
    }

    return 0;
}
