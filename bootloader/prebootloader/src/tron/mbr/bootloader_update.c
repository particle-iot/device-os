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
#include "flash_common.h"

extern FLASH_InitTypeDef flash_init_para;

extern uintptr_t link_system_part1_flash_start;
extern uintptr_t link_user_part_flash_end;
extern uintptr_t link_km0_mbr_flash_end;
extern uintptr_t link_platform_flash_end;
extern uintptr_t link_part1_module_info_flash_start;

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
    const uint32_t COPY_BLOCK_SIZE = 256; 
    uint8_t buf[COPY_BLOCK_SIZE];
    const uintptr_t src_end_addr = src_addr + size;

    uint32_t sectorNum = (size / 4096) + (((size % 4096) > 0) ? 1 : 0);
    if (hal_flash_erase_sector(dest_addr, sectorNum) != 0) {
        return false;
    }

    while (src_addr < src_end_addr) {
        size_t n = src_end_addr - src_addr;
        if (n > sizeof(buf)) {
            n = sizeof(buf);
        }
        if (hal_flash_read(src_addr, buf, n) != 0) {
            return false;
        }
        if (hal_flash_write(dest_addr, buf, n) != 0) {
            return false;
        }
        if (memcmp((uint8_t*)src_addr, (uint8_t*)dest_addr, n)) {
            return false;
        }
        src_addr += n;
        dest_addr += n;
    }
    return true;
}

bool bootloaderUpdateIfPending(void) {
    flash_init();

    flash_update_info_t info = {};

    uint32_t infoAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM0_BOOTLOADER_UPDATE_INFO_OFFSET;
    memcpy(&info, (void*)infoAddr, sizeof(info));

    // Validate flash_update_info_t integrity
    if (info.magic_num != KM0_BOOTLOADER_UPDATE_MAGIC_NUMBER
            || Compute_CRC32((const uint8_t*)&info, sizeof(flash_update_info_t) - sizeof(info.crc32), NULL) != info.crc32) {
        goto invalidate;
    }

    // flash_update_info_t looks valid
    // Validate source location, source should be within a potential dynamic OTA location region
    const uintptr_t ota_region_start = (uintptr_t)&link_system_part1_flash_start;
    const uintptr_t ota_region_end = (uintptr_t)&link_user_part_flash_end;
    if (!(info.src_addr >= ota_region_start && info.src_addr < ota_region_end
            && info.size > 0 && info.src_addr + info.size <= ota_region_end)) {
        goto invalidate;
    }

    // Check destination location, it should be within flash excluding MBR
    if (!(info.dest_addr >= (uintptr_t)&link_km0_mbr_flash_end && info.dest_addr < (uintptr_t)&link_platform_flash_end
            && info.dest_addr + info.size < (uintptr_t)&link_platform_flash_end)) {
        goto invalidate;
    }

    // Destination-specific checks. 
    // When updating KM0 part1, if the image is not encrypted and we have encryption enabled, then invalidate the update. 
    // IE dont allow unencrypted updates once we enable encryption.
    uint8_t userEfuse0 = 0xFF;
    EFUSE_PMAP_READ8(0, USER_KEY_0_EFUSE_ADDRESS, &userEfuse0, L25EOUTVOLTAGE);
    bool part1_encryption_enabled = !(userEfuse0 & PART1_ENCRYPTED_BIT);
    bool pending_image_encrypted = (info.flags & MODULE_ENCRYPTED);
    if (info.dest_addr == (uintptr_t)&link_part1_module_info_flash_start && (!pending_image_encrypted && part1_encryption_enabled)) {
        goto invalidate;
    }

    bool enableRsip = false;
    if ((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) != 0) {
        // Temporarily disable RSIP for memory copying, should leave the image in the OTA region as-is
        uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control & (~BIT_LSYS_PLFM_FLASH_SCE)));
        enableRsip = true;
    }
    
    bool ret = flash_copy(info.src_addr, info.dest_addr, info.size);
    if (enableRsip) {
        uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control | BIT_LSYS_PLFM_FLASH_SCE));
    }

    if (!ret) {
        // Flash copy failed. Try rebooting
        return false;
    }

    // Flash copy succeeded. If this is an encrypted part1 image, and encryption isnt enabled, enable it in pysical efuse
    if (info.dest_addr == (uintptr_t)&link_part1_module_info_flash_start && pending_image_encrypted && !part1_encryption_enabled) {
        userEfuse0 &= ~(PART1_ENCRYPTED_BIT);
        bool success = (EFUSE_PMAP_WRITE8(0, USER_KEY_0_EFUSE_ADDRESS, userEfuse0, L25EOUTVOLTAGE) != 0);
        if (!success) {
            return false;
        }
    }

invalidate:
    memset(&info, 0x00, sizeof(info));
    if (hal_flash_write(infoAddr, (const uint8_t*)&info, sizeof(info)) != 0) {
        hal_flash_erase_sector(BOOT_INFO_FLASH_XIP_START_ADDR, 1);
    }

    return true;
}
