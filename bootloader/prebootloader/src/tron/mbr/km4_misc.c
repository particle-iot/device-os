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
#include "rtl_support.h"
#include "module_info.h"

#define KM4_MISC_CONTROL_IPC_CHANNEL       1

static bool isPart1Valid = false;
static bool msgReceived = false; // Use queue instead

__attribute__((used)) void* dynalib_table_location = 0; // part1 dynalib location

extern uintptr_t link_part1_flash_start;
extern uintptr_t link_part1_flash_end;
extern uintptr_t link_part1_module_info_flash_start;
extern uintptr_t link_part1_dynalib_table_flash_start;
extern uintptr_t link_part1_dynalib_table_ram_start;

int bootloader_part1_preinit(void);
int bootloader_part1_init(void);
int bootloader_part1_postinit(void);

static uint32_t computeCrc32(const uint8_t *address, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    while (length > 0) {
        crc ^= *address++;
        for (uint8_t i = 0; i < 8; i++) {
            uint32_t mask = ~((crc & 1) - 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        length--;
    }
    return ~crc;
}

static bool isPart1ImageValid() {
    module_info_t info = {};
    _memcpy(&info, &link_part1_module_info_flash_start, sizeof(module_info_t));
    if (((uint32_t)info.module_start_address == (uint32_t)&link_part1_flash_start)
            && ((uint32_t)info.module_end_address <= (uint32_t)&link_part1_flash_end)
            && (info.platform_id == PLATFORM_ID)) {
        uint32_t length = (uint32_t)info.module_end_address - (uint32_t)info.module_start_address;
        uint32_t crc = computeCrc32((const uint8_t*)info.module_start_address, length);
        uint32_t expectedCRC = __REV((*(__IO uint32_t*)((uint32_t)info.module_start_address + length)));
        return (crc == expectedCRC);
    }
    return false;
}

static void km4MiscIpcInt(void *data, uint32_t irqStatus, uint32_t channel) {
    uint32_t msgAddr = rtlIpcGetMessage(channel);
    DCache_Invalidate(msgAddr, sizeof(uint32_t));
    DiagPrintf("KM0 received misc message address: 0x%08X\n", msgAddr);
    msgReceived = true;
}

void km4MiscInit(void) {
    rtlIpcChannelInit(KM4_MISC_CONTROL_IPC_CHANNEL, km4MiscIpcInt);

    isPart1Valid = isPart1ImageValid();
    if (isPart1Valid) {
        // dynalib table point to flash
        dynalib_table_location = &link_part1_dynalib_table_flash_start;
        // bootloader_part1_preinit() is executed in XIP
        if (bootloader_part1_preinit() < 0) {
            return;
        }
        // dynalib table point to SRAM
        dynalib_table_location = &link_part1_dynalib_table_ram_start;
        if (bootloader_part1_init() < 0) {
            return;
        }
        if (bootloader_part1_postinit() < 0) {
            return;
        }
        DiagPrintf("KM0 part1 is initialized\n");
    } else {
        DiagPrintf("KM0 part1 is invalid!\n");
    }
}

void km4MiscProcess(void) {
    if (msgReceived) {
        msgReceived = false;
        DiagPrintf("KM0 calls into km4MiscProcess()\n");
        if (!isPart1Valid) {
            // TODO: Send IPC message to indicate failure
            return;
        }
        // Echo response
        DelayMs(100); // Just to not mess up log message
        DiagPrintf("KM0 sends misc ACK\n");
        DelayMs(100); // Just to not mess up log message
        rtlIpcSendMessage(KM4_MISC_CONTROL_IPC_CHANNEL, 0xAABBCCDD);
        // TODO: process the received message
    }
}
