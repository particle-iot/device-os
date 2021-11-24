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

#include "stdbool.h"
#include "rtl8721d.h"
#include "check.h"
#include "flash_hal.h"
#include "boot_info.h"
#include "flash_mal.h"

#define COPY_BLOCK_SIZE                 256

#define OTA_REGION_LOWEST_ADDR          0x08060000
#define OTA_REGION_HIGHEST_ADDR         0x08600000


extern FLASH_InitTypeDef flash_init_para;

static void flash_init(void) {
    RCC_PeriphClockCmd(APBPeriph_FLASH, APBPeriph_FLASH_CLOCK_XTAL, ENABLE);

    uint32_t Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0);
    Temp &= ~(BIT_MASK_FLASH_CLK_SEL << BIT_SHIFT_FLASH_CLK_SEL);
    Temp |= BIT_SHIFT_FLASH_CLK_XTAL;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_CLK_CTRL0, Temp);

    FLASH_StructInit(&flash_init_para);
    FLASH_Init(SpicOneBitMode);

    uint8_t flashId[3];
    FLASH_RxCmd(flash_init_para.FLASH_cmd_rd_id, 3, flashId);
    if (flashId[0] == 0x20) {
        flash_init_para.FLASH_cmd_chip_e = 0xC7;
    }
}

static bool flash_copy(uintptr_t src_addr, uintptr_t dest_addr, size_t size) {
    uint8_t buf[COPY_BLOCK_SIZE];
    const uintptr_t src_end_addr = src_addr + size;

    DiagPrintf("[MBR] copy image from 0x%08X to 0x%08X, length: 0x%08X ....", src_addr, dest_addr, size);

    uint32_t sectorNum = (size / 4096) + (((size % 4096) > 0) ? 1 : 0);
    if (hal_flash_erase_sector(dest_addr, sectorNum) != 0) {
        DiagPrintf("[MBR] erasing flash failed\n");
        return false;
    }

    while (src_addr < src_end_addr) {
        size_t n = src_end_addr - src_addr;
        if (n > sizeof(buf)) {
            n = sizeof(buf);
        }
        if (hal_flash_read(src_addr, buf, n) != 0) {
            DiagPrintf("[MBR] hal_flash_read() failed\n");
            return false;
        }
        if (hal_flash_write(dest_addr, buf, n) != 0) {
            DiagPrintf("[MBR] hal_flash_write() failed\n");
            return false;
        }
        if (memcmp((uint8_t*)src_addr, (uint8_t*)dest_addr, n)) {
            DiagPrintf("[MBR] copy operation failed!\n");
            return false;
        }
        src_addr += n;
        dest_addr += n;
    }
    DiagPrintf("Done\n");
    return true;
}

bool bootloaderUpdateIfPending(void) {
    flash_init();

    flash_update_info_t info;
    for (uint8_t i = 0; i < 2; i++) {
        uint32_t infoAddr, targetAddr;
        if (i == 0) {
            infoAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM4_BOOTLOADER_UPDATE_INFO_OFFSET;
            targetAddr = KM4_BOOTLOADER_START_ADDRESS;
        } else {
            infoAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM0_PART1_UPDATE_INFO_OFFSET;
            targetAddr = KM0_PART1_START_ADDRESS;
        }
        memcpy(&info, (void*)infoAddr, sizeof(info));
        if (info.magic_num == KM0_UPDATE_MAGIC_NUMBER
                && info.src_addr > OTA_REGION_LOWEST_ADDR
                && info.src_addr < OTA_REGION_HIGHEST_ADDR
                && info.dest_addr == targetAddr
                && info.size > 0) {
            if (!flash_copy(info.src_addr, info.dest_addr, info.size)) {
                return false;
            }
            memset(&info, 0x00, sizeof(info));
            if (hal_flash_write(infoAddr, (const uint8_t*)&info, sizeof(info)) != 0) {
                DiagPrintf("[MBR] hal_flash_write() failed\n");
                return false;
            }
        }
    }
    return true;
}
