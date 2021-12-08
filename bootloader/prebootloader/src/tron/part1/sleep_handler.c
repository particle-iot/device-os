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
static volatile uint16_t sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
static int sleepResult = 0;

static void onSleepRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    sleepConfig = (hal_sleep_config_t*)msg->data;
    if (msg->data_len != sizeof(hal_sleep_config_t)) {
        sleepConfig = NULL;
    }
    sleepReqId = msg->req_id;
}

void sleepInit(void) {
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, onSleepRequestReceived, NULL);
}

void sleepProcess(void) {
    // Handle sleep
    sleepResult = SYSTEM_ERROR_NONE;
    if (sleepReqId != KM0_KM4_IPC_INVALID_REQ_ID) {
        if (!sleepConfig) {
            sleepResult = SYSTEM_ERROR_BAD_DATA;
        } else {
            DCache_Invalidate((uint32_t)sleepConfig, sizeof(hal_sleep_config_t));
            // TODO: perform sleep
            sleepResult = SYSTEM_ERROR_NOT_SUPPORTED;
        }
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sleepReqId, &sleepResult, sizeof(sleepResult));
        sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        sleepConfig = NULL;
    }
}
