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

#ifndef KM0_KM4_IPC_H
#define KM0_KM4_IPC_H

#include <stdint.h>
#include "module_info.h"

#define KM0_KM4_IPC_MSG_VERSION         2

#define KM0_KM4_IPC_MSG_VERSION_1       1
#define KM0_KM4_IPC_MSG_VERSION_2       KM0_KM4_IPC_MSG_VERSION

// Define more IPC channels if needed
#define KM0_KM4_IPC_CHANNEL_GENERIC     0

#define KM0_KM4_IPC_TIMEOUT_MS          5000

#define KM0_KM4_IPC_INVALID_REQ_ID      0xFFFF

typedef enum km0_km4_ipc_msg_type_t {
    KM0_KM4_IPC_MSG_RESP                = 0,
    KM0_KM4_IPC_MSG_BOOTLOADER_UPDATE   = 1,
    KM0_KM4_IPC_MSG_SLEEP               = 2,
    KM0_KM4_IPC_MSG_RESET               = 3,
    KM0_KM4_IPC_MSG_MAX                 = 0x7FFF
} km0_km4_ipc_msg_type_t;

static_assert(sizeof(km0_km4_ipc_msg_type_t) == 2, "size of km0_km4_ipc_msg_type_t is incorrect.");

typedef struct km0_km4_ipc_msg_t {
    uint16_t size;
    uint16_t version;
    km0_km4_ipc_msg_type_t type;
    uint16_t req_id;
    void* data;                         // WARNING: The data must not be allocated from stack
    uint32_t data_len;
    uint32_t data_crc32;                // of data payload
    uint32_t reserved[2];
    uint32_t crc32;                     // of this struct
} km0_km4_ipc_msg_t;

static_assert(sizeof(km0_km4_ipc_msg_t) % 32 == 0, "sizeof(km0_km4_ipc_msg_t) should be multiple of 32 bytes");

typedef void (*km0_km4_ipc_msg_callback_t)(km0_km4_ipc_msg_t* msg, void* context);


#ifdef __cplusplus

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#include "spark_wiring_vector.h"
#endif

namespace particle {

class Km0Km4IpcClass {
public:
    int init();

    // WARNING: The data must not be allocated from stack
    int sendRequest(km0_km4_ipc_msg_type_t type, void* data, uint32_t len, km0_km4_ipc_msg_callback_t respCallback, void* context);

    // WARNING: The data must not be allocated from stack
    int sendResponse(uint16_t reqId, void* data, uint32_t len);

    int onRequestReceived(km0_km4_ipc_msg_type_t type, km0_km4_ipc_msg_callback_t callback, void* context);

    void processReceivedMessage(km0_km4_ipc_msg_t* msg);

    static Km0Km4IpcClass* getInstance(uint8_t channel);

private:
    typedef struct {
        km0_km4_ipc_msg_type_t type;
        km0_km4_ipc_msg_callback_t callback;
        void* context;
    } km0_km4_ipc_request_handler_t;

    Km0Km4IpcClass(uint8_t ipcChannel);
    Km0Km4IpcClass() = delete;
    ~Km0Km4IpcClass() = default;

    uint8_t channel_;
    uint16_t reqId_;
    volatile uint16_t expectedRespReqId_;
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    km0_km4_ipc_request_handler_t ipcRequestHandlers_[10];
#else
    spark::Vector<km0_km4_ipc_request_handler_t> ipcRequestHandlers_;
#endif
    // WARNING: it is allocated in static RAM and will be used for all IPC message exchanges.
    // We introduce Km0Km4IpcLock and a "blocked" parameter when sending IPC message to prevent this memory
    // from being overwritten before sending the next IPC message.
    km0_km4_ipc_msg_t __attribute__((aligned(32))) km0Km4IpcMessage_;
    km0_km4_ipc_msg_callback_t respCallback_;
    void* respCallbackContext_;
    uint16_t peerMsgVersion_;
};

} // namespace::particle

extern "C" {
#endif

int km0_km4_ipc_init(uint8_t channel);
int km0_km4_ipc_on_request_received(uint8_t channel, km0_km4_ipc_msg_type_t type, km0_km4_ipc_msg_callback_t callback, void* context);
int km0_km4_ipc_send_request(uint8_t channel, km0_km4_ipc_msg_type_t type, void* data, uint32_t len, km0_km4_ipc_msg_callback_t resp_callback, void* context);
int km0_km4_ipc_send_response(uint8_t channel, uint16_t req_id, void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // KM0_KM4_IPC_H
