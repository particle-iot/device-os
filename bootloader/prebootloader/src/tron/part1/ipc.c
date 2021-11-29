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

#include "ameba_soc.h"
#include "rtl_sdk_support.h"

int ipc_channel_init(uint8_t channel, rtl_ipc_callback_t callback) {
    if (channel > 15) {
        return -1;
    }
    IPC_INTUserHandler(channel, (void*)callback, NULL);
    return 0;
}

void ipc_send_message_alt(uint8_t channel, uint32_t message) {
    IPCM0_DEV->IPCx_USR[channel] = message;
    IPC_INTRequest(IPCM0_DEV, channel);
}

uint32_t ipc_get_message_alt(uint8_t channel) {
    uint32_t msgAddr = IPCM4_DEV->IPCx_USR[channel];
    return msgAddr;
}
