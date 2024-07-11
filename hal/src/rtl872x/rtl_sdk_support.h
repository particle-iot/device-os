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

#ifndef RTL_SDK_SUPPORT_H
#define RTL_SDK_SUPPORT_H

#include "stdint.h"

typedef void (*rtl_ipc_callback_t)(void *data, uint32_t irq_status, uint32_t channel);

typedef enum {
    RTW_RADIO_NONE = 0x00,
    RTW_RADIO_WIFI = 0x01,
    RTW_RADIO_BLE = 0x02,
    RTW_RADIO_ALL = 0x03
} RtwRadio;

typedef struct __attribute__((packed)) rtl_wifi_fw_ram_alloc {
    uint16_t size;
    uint16_t reserved;
    uint32_t start;
    uint32_t end;
} rtl_wifi_fw_ram_alloc;

typedef struct __attribute((packed)) rtl_wifi_fw_resp {
    uint16_t size;
    uint16_t reserved;
    int result;
    uint32_t start;
    uint32_t end;
    uint32_t reserved_heap;
} rtl_wifi_fw_resp;

#ifdef __cplusplus
extern "C" {
#endif

int ipc_channel_init(uint8_t channel, rtl_ipc_callback_t callback);
void ipc_send_message_alt(uint8_t channel, uint32_t message);
uint32_t ipc_get_message_alt(uint8_t channel);

// SDK stub calls
void ipc_table_init(void);
void ipc_send_message(uint8_t channel, uint32_t message);
uint32_t ipc_get_message(uint8_t channel);

void rtwCoexRunDisable(int idx);
void rtwCoexPreventCleanup(int idx);
void rtwCoexRunEnable(int idx);
void rtwCoexCleanup(int idx);
void rtwCoexCleanupMutex(int idx);

void rtwRadioReset();
void rtwRadioAcquire(RtwRadio r);
void rtwRadioRelease(RtwRadio r);
void rtwCoexStop();
void rtwCoexSetWifiConnectedState(bool state);
bool rtwCoexWifiConnectedState();
int rtwCoexSet(uint32_t coex[4], uint8_t tmda[5], bool apply);

#ifdef __cplusplus
}
#endif

#endif // RTL_SDK_SUPPORT_H
