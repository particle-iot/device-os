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

#include "rtl8721d.h"
#include "rtl_support.h"
#include "platform_flash_modules.h"

#define BOOTLOADER_UPDATE_IPC_CHANNEL       0

static volatile platform_flash_modules_t* flashModule = NULL;

static void bootloaderUpdateIpcInt(void *data, uint32_t irqStatus, uint32_t channel) {
    flashModule = (platform_flash_modules_t*)rtlIpcGetMessage(channel);
    DCache_Invalidate((uint32_t)flashModule, sizeof(platform_flash_modules_t));
    DiagPrintf("KM0 received flash module address: 0x%08X\n", flashModule);
}

void bootloaderUpdateInit(void) {
    rtlIpcChannelInit(BOOTLOADER_UPDATE_IPC_CHANNEL, bootloaderUpdateIpcInt);
}

void bootloaderUpdateProcess(void) {
    if (flashModule) {
        // Echo response
        DelayMs(100); // Just to not mess up log message
        DiagPrintf("KM0 sends bootloader update ACK.\n");
        DelayMs(100); // Just to not mess up log message
        rtlIpcSendMessage(BOOTLOADER_UPDATE_IPC_CHANNEL, 0x9abcdef0);

        // TODO: perform bootloader update

        flashModule = NULL;
    }
}
