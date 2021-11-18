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
#include "km0_km4_ipc.h"
#include "platform_flash_modules.h"
#include "check.h"
#include "flash_hal.h"
#include "boot_info.h"


static volatile platform_flash_modules_t* flashModule = NULL;
static volatile uint16_t bootloaderUpdateReqId = INVALID_IPC_REQ_ID;
static int updateResult = 0;
static bool bootInfoSectorErased = false;
static volatile bool km4BldUpdate;

extern FLASH_InitTypeDef flash_init_para;
extern CPU_PWR_SEQ HSPWR_OFF_SEQ[];

static void onFlashModuleReceived(km0_km4_ipc_msg_t* msg, void* context) {
    if ((uint32_t)context == KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE) {
        km4BldUpdate = true;
    } else {
        km4BldUpdate = false; // KM0 part1 image update
    }
    if (msg->data_len != sizeof(platform_flash_modules_t)) {
        DiagPrintf("KM0: invalid IPC message length.\n");
        return;
    }
    flashModule = (platform_flash_modules_t*)msg->data;
    bootloaderUpdateReqId = msg->req_id;
    DiagPrintf("KM0 received KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE: 0x%08X\n", (uint32_t)flashModule);
}

static void onResetRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM4_CTRL) & BIT_LSYS_HPLAT_CKE){
        BOOT_ROM_CM4PON((u32)HSPWR_OFF_SEQ);
    }
    NVIC_SystemReset();
}

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

static bool flash_write_update_info(void) {
    DiagPrintf("Call into flash_write_update_info(), 0x%08X -> 0x%08X, length: 0x%08X\n",
        flashModule->sourceAddress, flashModule->destinationAddress, flashModule->length);

    if (!bootInfoSectorErased) {
        flash_init();
        DiagPrintf("Erasing the boot info sector...\n");
        if (hal_flash_erase_sector(BOOT_INFO_FLASH_XIP_START_ADDR, 1) != 0) {
            DiagPrintf("Erasing flash failed\n");
            return false;
        }
        bootInfoSectorErased = true;
    }

    flash_update_info_t info;
    uint32_t writeAddr;

    memset(&info, 0x00, sizeof(info));

    info.src_addr = flashModule->sourceAddress;
    info.dest_addr = flashModule->destinationAddress;
    info.size = flashModule->length;

    if (km4BldUpdate) {
        info.magic_num = KM0_UPDATE_MAGIC_NUMBER;
        writeAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM4_BOOTLOADER_UPDATE_INFO_OFFSET;
    } else {
        info.magic_num = KM0_UPDATE_MAGIC_NUMBER;
        writeAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM0_PART1_UPDATE_INFO_OFFSET;
    }

    if (hal_flash_write(writeAddr, (const uint8_t*)&info, sizeof(info)) != 0) {
        DiagPrintf("hal_flash_write() failed\n");
        return false;
    }
    if (memcmp((uint8_t*)&info, (uint8_t*)writeAddr, sizeof(info))) {
        DiagPrintf("Write boot info failed!\n");
        return false;
    }
    return true;
}

void bootloaderUpdateInit(void) {
    flashModule = NULL;
    bootloaderUpdateReqId = INVALID_IPC_REQ_ID;
    bootInfoSectorErased = false;
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE, onFlashModuleReceived, (void*)KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE);
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_KM0_PART1_UPDATE, onFlashModuleReceived, (void*)KM0_KM4_IPC_MSG_KM0_PART1_UPDATE);
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_RESET, onResetRequestReceived, NULL);
}

void bootloaderUpdateProcess(void) {
    // Handle bootloader update
    updateResult = 0;
    if (bootloaderUpdateReqId != INVALID_IPC_REQ_ID && flashModule) {
        DCache_Invalidate((uint32_t)flashModule, sizeof(platform_flash_modules_t));

        if (!flash_write_update_info()) {
            updateResult = -1;
        }

        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, bootloaderUpdateReqId, &updateResult, sizeof(updateResult));

        bootloaderUpdateReqId = INVALID_IPC_REQ_ID;
        flashModule = NULL;
    }
}
