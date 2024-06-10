/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "wififw_handler.h"
#include "km0_km4_ipc.h"
#include "system_error.h"
#include "flash_hal.h"
#include "check.h"
#include "FreeRTOS.h"
#include "task.h"
#include "heap_portable.h"
extern "C" {
#include "rtl8721d.h"
}
#include "rtl_sdk_support.h"
#include <algorithm>

namespace {

volatile uint16_t sReqId = KM0_KM4_IPC_INVALID_REQ_ID;
rtl_wifi_fw_resp __attribute__((aligned(32))) sResult; // 32 bytes for Dcache requirement

rtl_wifi_fw_ram_alloc sWifiFwRamAlloc = {};

}

extern uintptr_t link_wifi_fw_text_flash_start;
extern uintptr_t link_wifi_fw_text_start;
extern uintptr_t link_wifi_fw_text_end;
#define link_wifi_fw_text_size ((uintptr_t)&link_wifi_fw_text_end - (uintptr_t)&link_wifi_fw_text_start)

extern uintptr_t link_wifi_fw_bss_start;
extern uintptr_t link_wifi_fw_bss_end;
#define link_wifi_fw_bss_size ((uintptr_t)&link_wifi_fw_bss_end - (uintptr_t)&link_wifi_fw_bss_start)

extern uintptr_t link_wifi_fw_data_flash_start;
extern uintptr_t link_wifi_fw_data_start;
extern uintptr_t link_wifi_fw_data_end;
#define link_wifi_fw_data_size ((uintptr_t)&link_wifi_fw_data_end - (uintptr_t)&link_wifi_fw_data_start)

extern uintptr_t link_wifi_fw_used_end;

#define RESERVED_MINIMAL_HEAP_SIZE (20 * 1024)

extern "C" void wifi_FW_init_ram(void);

static void onWifiFwInitReceived(km0_km4_ipc_msg_t* msg, void* context) {
    sReqId = msg->req_id;
    if (msg->data) {
        memcpy(&sWifiFwRamAlloc, msg->data, std::min<size_t>(sizeof(sWifiFwRamAlloc), msg->size));
    } else {
        memset(&sWifiFwRamAlloc, 0, sizeof(sWifiFwRamAlloc));
    }
}

void wifiFwInit() {
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_WIFI_FW_INIT, onWifiFwInitReceived, nullptr);
}

void __malloc_lock(struct _reent *ptr) {
    vPortEnterCritical();
}

void __malloc_unlock(struct _reent *ptr) {
    vPortExitCritical();
}

extern const int  __attribute__((used)) uxTopUsedPriority = configMAX_PRIORITIES - 1;

extern "C" void driver_fw_flow_ipc_int(void* data, uint32_t irqStatus, uint32_t chanNum);
extern "C" int bootloader_part1_loop(void);

extern "C" void vApplicationIdleHook(void) {
    // FIXME: use semaphore
    bootloader_part1_loop();
}

static void wifiFwStart() {
    sResult.size = sizeof(sResult);
    sResult.start = (uint32_t)&link_wifi_fw_bss_start;
    sResult.end = (uint32_t)&link_wifi_fw_used_end;
    sResult.reserved_heap = RESERVED_MINIMAL_HEAP_SIZE;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        sResult.result = SYSTEM_ERROR_ALREADY_EXISTS;
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sReqId, &sResult, sizeof(sResult));
        sReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        return;
    }
    if (sWifiFwRamAlloc.start != (uint32_t)&link_wifi_fw_bss_start ||
            sWifiFwRamAlloc.end < ((uint32_t)&link_wifi_fw_used_end + RESERVED_MINIMAL_HEAP_SIZE)) {
        sResult.result = SYSTEM_ERROR_NO_MEMORY;
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sReqId, &sResult, sizeof(sResult));
        sReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        return;
    }
    _memset(&link_wifi_fw_bss_start, 0, link_wifi_fw_bss_size);

    hal_flash_read((uintptr_t)&link_wifi_fw_text_flash_start, (uint8_t*)&link_wifi_fw_text_start, link_wifi_fw_text_size);
    hal_flash_read((uintptr_t)&link_wifi_fw_data_flash_start, (uint8_t*)&link_wifi_fw_data_start, link_wifi_fw_data_size);

    malloc_enable(true);

    __NVIC_SetVector(SVCall_IRQn, (u32)vPortSVCHandler);
	__NVIC_SetVector(PendSV_IRQn, (u32)xPortPendSVHandler);
	__NVIC_SetVector(SysTick_IRQn, (u32)xPortSysTickHandler); 

    wifi_FW_init_ram();

    IPC_INTUserHandler(1, (void*)driver_fw_flow_ipc_int, (void*)IPCM4_DEV);

    sResult.result = 0;
    km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sReqId, &sResult, sizeof(sResult));
    sReqId = KM0_KM4_IPC_INVALID_REQ_ID;

    vTaskStartScheduler(); // Does not exit
}

void wifiFwProcess() {
    if (sReqId != KM0_KM4_IPC_INVALID_REQ_ID) {
        memset(&sResult, 0, sizeof(sResult));
        wifiFwStart(); // Should normally not exit
    }
}
