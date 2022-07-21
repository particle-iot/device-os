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
extern "C" {
#include "rtl8721d.h"
}
#include "hw_config.h"
#include "rtl_sdk_support.h"
#include "check.h"
#include "static_recursive_mutex.h"
#include "km0_km4_ipc.h"
#include "align_util.h"

using namespace particle;

/* FIXME: MBR and KM0 part1 have MODULE_INDEX defined in makefile, but not for Particle bootloader */
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER && (defined(MODULE_INDEX) && MODULE_INDEX > 0)
#define WAIT_TIMED(timeout_ms, what) ({ \
    bool res = true;                                                            \
    res;                                                                        \
})
#else
#include "timer_hal.h"
#define WAIT_TIMED(timeout_ms, what) ({ \
    system_tick_t _micros = HAL_Timer_Get_Micro_Seconds();                      \
    bool res = true;                                                            \
    while ((what)) {                                                            \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - _micros);           \
        bool nok = (((timeout_ms * 1000) < dt) && (what));                      \
        if (nok) {                                                              \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    res;                                                                        \
})
#endif


namespace {

typedef struct km0_km4_ipc_msg_v1_t {
    uint16_t size;
    uint16_t version;
    km0_km4_ipc_msg_type_t type;
    uint16_t req_id;
    void* data;                         // WARNING: The data must not be allocated from stack
    uint32_t data_len;
    uint32_t data_crc32;                // of data payload
    uint32_t crc32;
} km0_km4_ipc_msg_v1_t;

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

class Km0Km4IpcLock {
};

#else

class Km0Km4IpcLock {
public:
    Km0Km4IpcLock() :
            locked_(false) {
        lock();
    }

    ~Km0Km4IpcLock() {
        if (locked_) {
            unlock();
        }
    }

    Km0Km4IpcLock(Km0Km4IpcLock&& lock) :
            locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        mutex_.lock();
        locked_ = true;
    }

    void unlock() {
        mutex_.unlock();
        locked_ = false;
    }

    Km0Km4IpcLock(const Km0Km4IpcLock&) = delete;
    Km0Km4IpcLock& operator=(const Km0Km4IpcLock&) = delete;

private:
    bool locked_;
    static StaticRecursiveMutex mutex_;
};

StaticRecursiveMutex Km0Km4IpcLock::mutex_;

#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

void km0Km4IpcIntHandler(void *data, uint32_t irqStatus, uint32_t channel) {
    km0_km4_ipc_msg_t* message = (km0_km4_ipc_msg_t*)ipc_get_message_alt(channel);
    DCache_Invalidate((uint32_t)message, message->size);

    if (message) {
        Km0Km4IpcClass::getInstance(channel)->processReceivedMessage(message);
    }
}

} // anonymous namespace


Km0Km4IpcClass* Km0Km4IpcClass::getInstance(uint8_t channel) {
    static __attribute__((section(".boot.ipc_data"))) Km0Km4IpcClass ipcs[] = {
        {KM0_KM4_IPC_CHANNEL_GENERIC},
        // Add more channels if needed
    };
    static_assert(is_member_aligned_to<32>(ipcs[0].km0Km4IpcMessage_), "alignof doesn't match");

    for (auto& ipc : ipcs) {
        if (ipc.channel_ == channel) {
            return &ipc;
        }
    }
    return nullptr;
}

int Km0Km4IpcClass::init() {
    return SYSTEM_ERROR_NONE;
}

// The message must not be allocated from stack
int Km0Km4IpcClass::sendRequest(km0_km4_ipc_msg_type_t type, void* data, uint32_t len, km0_km4_ipc_msg_callback_t respCallback, void* context) {
    if (type == KM0_KM4_IPC_MSG_RESP || type >= KM0_KM4_IPC_MSG_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    Km0Km4IpcLock lock();
    km0Km4IpcMessage_ = {};
    km0Km4IpcMessage_.size = sizeof(km0_km4_ipc_msg_t);
    km0Km4IpcMessage_.version = KM0_KM4_IPC_MSG_VERSION;
    km0Km4IpcMessage_.type = type;
    km0Km4IpcMessage_.req_id = reqId_;
#if defined (ARM_CPU_CORTEX_M33)
    SPARK_ASSERT(((uint32_t)data & 0x0000001F) == 0); // Make sure the buffer is 32-byte aligned
    // Length of data should be 32 bytes aligned as well, but no need to send padding data.
#endif
    km0Km4IpcMessage_.data = data;
    km0Km4IpcMessage_.data_len = len;
    km0Km4IpcMessage_.data_crc32 = Compute_CRC32((const uint8_t*)data, len, nullptr);
    if (peerMsgVersion_ == KM0_KM4_IPC_MSG_VERSION_1) {
        auto msg = reinterpret_cast<km0_km4_ipc_msg_v1_t*>(&km0Km4IpcMessage_);
        msg->crc32 = Compute_CRC32((const uint8_t*)msg, sizeof(km0_km4_ipc_msg_v1_t) - sizeof(km0_km4_ipc_msg_v1_t::crc32), nullptr);
    } else {
        km0Km4IpcMessage_.crc32 = Compute_CRC32((const uint8_t*)&km0Km4IpcMessage_, sizeof(km0_km4_ipc_msg_t) - sizeof(km0_km4_ipc_msg_t::crc32), nullptr);
    }
    respCallback_ = respCallback;
    respCallbackContext_ = context;

    expectedRespReqId_ = reqId_;
    DCache_CleanInvalidate((uint32_t)&km0Km4IpcMessage_, sizeof(km0Km4IpcMessage_));
    ipc_send_message_alt(channel_, (uint32_t)(&km0Km4IpcMessage_));

    int ret = SYSTEM_ERROR_NONE;
    if (!WAIT_TIMED(KM0_KM4_IPC_TIMEOUT_MS, expectedRespReqId_ == reqId_)) {
        ret = SYSTEM_ERROR_TIMEOUT;
    }

    expectedRespReqId_ = KM0_KM4_IPC_INVALID_REQ_ID;
    respCallback_ = nullptr;
    respCallbackContext_ = nullptr;
    reqId_++;
    reqId_ = (reqId_ == KM0_KM4_IPC_INVALID_REQ_ID) ? 0 : reqId_;
    return ret;
}

int Km0Km4IpcClass::sendResponse(uint16_t reqId, void* data, uint32_t len) {
    Km0Km4IpcLock lock();
    km0Km4IpcMessage_ = {};
    km0Km4IpcMessage_.size = sizeof(km0_km4_ipc_msg_t);
    km0Km4IpcMessage_.version = KM0_KM4_IPC_MSG_VERSION;
    km0Km4IpcMessage_.type = KM0_KM4_IPC_MSG_RESP;
    km0Km4IpcMessage_.req_id = reqId;
    km0Km4IpcMessage_.data = data;
    km0Km4IpcMessage_.data_len = len;
    km0Km4IpcMessage_.data_crc32 = Compute_CRC32((const uint8_t*)data, len, nullptr);
    if (peerMsgVersion_ == KM0_KM4_IPC_MSG_VERSION_1) {
        auto msg = reinterpret_cast<km0_km4_ipc_msg_v1_t*>(&km0Km4IpcMessage_);
        msg->crc32 = Compute_CRC32((const uint8_t*)msg, sizeof(km0_km4_ipc_msg_v1_t) - sizeof(km0_km4_ipc_msg_v1_t::crc32), nullptr);
    } else {
        km0Km4IpcMessage_.crc32 = Compute_CRC32((const uint8_t*)&km0Km4IpcMessage_, sizeof(km0_km4_ipc_msg_t) - sizeof(km0_km4_ipc_msg_t::crc32), nullptr);
    }
    DCache_CleanInvalidate((uint32_t)&km0Km4IpcMessage_, sizeof(km0Km4IpcMessage_));
    ipc_send_message_alt(channel_, (uint32_t)(&km0Km4IpcMessage_));
    return SYSTEM_ERROR_NONE;
}

int Km0Km4IpcClass::onRequestReceived(km0_km4_ipc_msg_type_t type, km0_km4_ipc_msg_callback_t callback, void* context) {
    Km0Km4IpcLock lock();
    for (auto& handler : ipcRequestHandlers_) {
        if (handler.type == type)  {
            handler.callback = callback;
            handler.context = context;
            return SYSTEM_ERROR_NONE;
        }
    }
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    for (auto& handler : ipcRequestHandlers_) {
        if (handler.type >= KM0_KM4_IPC_MSG_MAX)  {
            handler.type = type;
            handler.callback = callback;
            handler.context = context;
            return SYSTEM_ERROR_NONE;
        }
    }
    return SYSTEM_ERROR_NO_MEMORY;
#else
    km0_km4_ipc_request_handler_t handler = {};
    handler.callback = callback;
    handler.context = context;
    handler.type = type;
    CHECK_TRUE(ipcRequestHandlers_.append(handler), SYSTEM_ERROR_NO_MEMORY);
#endif
    return SYSTEM_ERROR_NONE;
}

void Km0Km4IpcClass::processReceivedMessage(km0_km4_ipc_msg_t* msg) {
    // necessary
    DCache_Invalidate((uint32_t)msg->data, msg->data_len);
    uint32_t crc32 = *((uint32_t*)((uint32_t)msg + msg->size - sizeof(km0_km4_ipc_msg_t::crc32)));
    if (Compute_CRC32((const uint8_t*)msg, msg->size - sizeof(km0_km4_ipc_msg_t::crc32), nullptr) != crc32
            || (msg->data_len > 0 && (msg->data == nullptr || Compute_CRC32((const uint8_t*)msg->data, msg->data_len, nullptr) != msg->data_crc32))) {
        msg->data = nullptr;
        msg->data_len = 0;
    }
    peerMsgVersion_ = msg->version;
    // Handler response
    if (msg->type == KM0_KM4_IPC_MSG_RESP && expectedRespReqId_ == msg->req_id) {
        if (respCallback_) {
            respCallback_(msg, respCallbackContext_);
        }
        expectedRespReqId_ = KM0_KM4_IPC_INVALID_REQ_ID;
        return;
    }
    // Handle request
    for (auto& handler : ipcRequestHandlers_) {
        if (handler.type == msg->type && handler.callback) {
            handler.callback(msg, handler.context);
            return;
        }
    }
}

Km0Km4IpcClass::Km0Km4IpcClass(uint8_t ipcChannel)
        : channel_(ipcChannel),
          peerMsgVersion_(KM0_KM4_IPC_MSG_VERSION) {
    reqId_ = 0;
    expectedRespReqId_ = KM0_KM4_IPC_INVALID_REQ_ID;
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
    for (auto& handler : ipcRequestHandlers_) {
        handler.type = KM0_KM4_IPC_MSG_MAX;
    }
#endif
    ipc_channel_init(channel_, km0Km4IpcIntHandler);
}


int km0_km4_ipc_init(uint8_t channel) {
    Km0Km4IpcClass* instance = Km0Km4IpcClass::getInstance(channel);
    CHECK_TRUE(instance, SYSTEM_ERROR_NOT_FOUND);
    instance->init();
    return SYSTEM_ERROR_NONE;
}

int km0_km4_ipc_on_request_received(uint8_t channel, km0_km4_ipc_msg_type_t type, km0_km4_ipc_msg_callback_t callback, void* context) {
    Km0Km4IpcClass* instance = Km0Km4IpcClass::getInstance(channel);
    CHECK_TRUE(instance, SYSTEM_ERROR_NOT_FOUND);
    CHECK(instance->onRequestReceived(type, callback, context));
    return SYSTEM_ERROR_NONE;
}

int km0_km4_ipc_send_request(uint8_t channel, km0_km4_ipc_msg_type_t type, void* data, uint32_t len, km0_km4_ipc_msg_callback_t resp_callback, void* context) {
    Km0Km4IpcClass* instance = Km0Km4IpcClass::getInstance(channel);
    CHECK_TRUE(instance, SYSTEM_ERROR_NOT_FOUND);
    CHECK(instance->sendRequest(type, data, len, resp_callback, context));
    return SYSTEM_ERROR_NONE;
}

int km0_km4_ipc_send_response(uint8_t channel, uint16_t req_id, void* data, uint32_t len) {
    Km0Km4IpcClass* instance = Km0Km4IpcClass::getInstance(channel);
    CHECK_TRUE(instance, SYSTEM_ERROR_NOT_FOUND);
    CHECK(instance->sendResponse(req_id, data, len));
    return SYSTEM_ERROR_NONE;
}
