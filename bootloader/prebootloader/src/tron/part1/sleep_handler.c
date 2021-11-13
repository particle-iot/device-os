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
#include "sleep_hal.h"
#include "check.h"

static volatile hal_sleep_config_t* sleepConfig = NULL;
static volatile uint16_t sleepReqId = INVALID_IPC_REQ_ID;

static void onSleepRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    if (msg->data_len != sizeof(hal_sleep_config_t)) {
        DiagPrintf("KM0: invalid IPC message length.\n");
        return;
    }
    sleepConfig = (hal_sleep_config_t*)msg->data;
    sleepReqId = msg->req_id;
    DiagPrintf("KM0 received KM0_KM4_IPC_MSG_SLEEP: 0x%08X\n", (uint32_t)sleepConfig);
}

void sleepInit(void) {
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, onSleepRequestReceived, NULL);
}

void sleepProcess(void) {
    // Handle sleep
    if (sleepReqId != INVALID_IPC_REQ_ID && sleepConfig) {
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sleepReqId, NULL, 0);
        sleepReqId = INVALID_IPC_REQ_ID;

        DCache_Invalidate((uint32_t)sleepConfig, sizeof(hal_sleep_config_t));

        // TODO: perform sleep
        
        sleepConfig = NULL;
    }
}
