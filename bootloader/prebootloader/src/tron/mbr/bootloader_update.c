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

static volatile platform_flash_modules_t* flashModule = NULL;
static volatile uint16_t bootloaderUpdateReqId = INVALID_IPC_REQ_ID;

static void onRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    if (msg->data_len != sizeof(platform_flash_modules_t)) {
        DiagPrintf("KM0: invalid IPC message length.\n");
        return;
    }
    flashModule = (platform_flash_modules_t*)msg->data;
    bootloaderUpdateReqId = msg->req_id;
    DiagPrintf("KM0 received KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE: 0x%08X\n", (uint32_t)flashModule);
}

void bootloaderUpdateInit(void) {
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE, onRequestReceived, NULL);
}

void bootloaderUpdateProcess(void) {
    // Handle bootloader update
    if (bootloaderUpdateReqId != INVALID_IPC_REQ_ID && flashModule) {
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, bootloaderUpdateReqId, NULL, 0);
        bootloaderUpdateReqId = INVALID_IPC_REQ_ID;

        DCache_Invalidate((uint32_t)flashModule, sizeof(platform_flash_modules_t));

        // TODO: perform bootloader update
        
        flashModule = NULL;
    }
}
