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
#include "flash_mal.h"
#include "boot_info.h"
#include "hw_config.h"


static volatile platform_flash_modules_t* flashModule = NULL;
static volatile uint16_t bootloaderUpdateReqId = KM0_KM4_IPC_INVALID_REQ_ID;
static int32_t __attribute__((aligned(32))) updateResult[8];

extern FLASH_InitTypeDef flash_init_para;
extern CPU_PWR_SEQ HSPWR_OFF_SEQ[];

static void onFlashModuleReceived(km0_km4_ipc_msg_t* msg, void* context) {
    flashModule = (platform_flash_modules_t*)msg->data;
    if (msg->data_len != sizeof(platform_flash_modules_t)) {
        flashModule = NULL;
    }
    bootloaderUpdateReqId = msg->req_id;
}

static void onResetRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    BOOT_ROM_CM4PON((u32)HSPWR_OFF_SEQ);
    NVIC_SystemReset();
}

static bool flash_write_update_info(void) {
    if (hal_flash_erase_sector(BOOT_INFO_FLASH_XIP_START_ADDR, 1) != 0) {
        return false;
    }

    flash_update_info_t info = {};
    uint32_t writeAddr = BOOT_INFO_FLASH_XIP_START_ADDR + KM0_BOOTLOADER_UPDATE_INFO_OFFSET;

    info.src_addr = flashModule->sourceAddress;
    info.dest_addr = flashModule->destinationAddress;
    info.size = flashModule->length;
    info.magic_num = KM0_BOOTLOADER_UPDATE_MAGIC_NUMBER;
    info.flags = flashModule->flags;
    info.crc32 = Compute_CRC32((const uint8_t*)&info, sizeof(flash_update_info_t) - sizeof(info.crc32), NULL);

    if (hal_flash_write(writeAddr, (const uint8_t*)&info, sizeof(info)) != 0) {
        goto err;
    }
    if (memcmp((uint8_t*)&info, (uint8_t*)writeAddr, sizeof(info))) {
        goto err;
    }
    goto done;

err:
    hal_flash_erase_sector(BOOT_INFO_FLASH_XIP_START_ADDR, 1);
    return false;

done:
    return true;
}

void bootloaderUpdateInit(void) {
    flashModule = NULL;
    bootloaderUpdateReqId = KM0_KM4_IPC_INVALID_REQ_ID;
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE, onFlashModuleReceived, NULL);
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_RESET, onResetRequestReceived, NULL);
}

void bootloaderUpdateProcess(void) {
    // Handle bootloader update
    updateResult[0] = SYSTEM_ERROR_NONE;
    if (bootloaderUpdateReqId != KM0_KM4_IPC_INVALID_REQ_ID) {
        if (!flashModule) {
            updateResult[0] = SYSTEM_ERROR_BAD_DATA;
        } else {
            DCache_Invalidate((uint32_t)flashModule, sizeof(platform_flash_modules_t));
            if (!flash_write_update_info()) {
                updateResult[0] = SYSTEM_ERROR_INTERNAL;
            }
        }
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, bootloaderUpdateReqId, updateResult, sizeof(updateResult));
        bootloaderUpdateReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        flashModule = NULL;
    }
}
