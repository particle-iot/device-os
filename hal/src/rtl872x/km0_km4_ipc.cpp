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
#include "rtl_sdk_support.h"
#include "check.h"
#include "static_recursive_mutex.h"
#include "km0_km4_ipc.h"

/* FIXME: MBR and KM0 part1 have MODULE_INDEX defined in makefile, but not for Particle bootloader */
#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER && defined(MODULE_INDEX)
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
#endif // 

void km0Km4IpcIntHandler(void *data, uint32_t irqStatus, uint32_t channel) {
    km0_km4_ipc_msg_t* message = (km0_km4_ipc_msg_t*)ipc_get_message(channel);
    DCache_Invalidate((uint32_t)message, sizeof(km0_km4_ipc_msg_t));

    Km0Km4IpcClass::getInstance(channel)->processReceivedMessage(message);
}

} // anonymous namespace


Km0Km4IpcClass* Km0Km4IpcClass::getInstance(uint8_t channel) {
    static __attribute__((section(".boot.ipc_data"))) Km0Km4IpcClass ipcs[] = {
        {KM0_KM4_IPC_CHANNEL_GENERIC},
        // Add more channels if needed
    };
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
    if (type == KM0_KM4_IPC_MSG_RESP || type == KM0_KM4_IPC_MSG_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    Km0Km4IpcLock lock();
    km0Km4IpcMessage_ = {};
    km0Km4IpcMessage_.size = sizeof(km0Km4IpcMessage_);
    km0Km4IpcMessage_.version = KM0_KM4_IPC_MSG_VERSION;
    km0Km4IpcMessage_.type = type;
    km0Km4IpcMessage_.req_id = reqId_;
    km0Km4IpcMessage_.data = data;
    km0Km4IpcMessage_.data_len = len;
    respCallback_ = respCallback;
    respCallbackContext_ = context;
    if (!respCallback) {
        expectedRespReqId_ = INVALID_IPC_REQ_ID;
    } else {
        expectedRespReqId_ = reqId_;
    }
    ipc_send_message(channel_, (uint32_t)(&km0Km4IpcMessage_));

    int ret = SYSTEM_ERROR_NONE;
    if (respCallback_) {
        if (!WAIT_TIMED(KM0_KM4_IPC_TIMEOUT_MS, expectedRespReqId_ == reqId_)) {
            ret = SYSTEM_ERROR_TIMEOUT;
            goto done;
        }
    }
done:
    expectedRespReqId_ = INVALID_IPC_REQ_ID;
    respCallback_ = nullptr;
    respCallbackContext_ = nullptr;
    reqId_++;
    reqId_ = (reqId_ == INVALID_IPC_REQ_ID) ? 0 : reqId_;
    return ret;
}

int Km0Km4IpcClass::sendResponse(uint16_t reqId, void* data, uint32_t len) {
    Km0Km4IpcLock lock();
    km0Km4IpcMessage_ = {};
    km0Km4IpcMessage_.size = sizeof(km0Km4IpcMessage_);
    km0Km4IpcMessage_.version = KM0_KM4_IPC_MSG_VERSION;
    km0Km4IpcMessage_.type = KM0_KM4_IPC_MSG_RESP;
    km0Km4IpcMessage_.req_id = reqId;
    km0Km4IpcMessage_.data = data;
    km0Km4IpcMessage_.data_len = len;
    ipc_send_message(channel_, (uint32_t)(&km0Km4IpcMessage_));
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
        if (handler.type == KM0_KM4_IPC_MSG_MAX)  {
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
    if (!msg) {
        return;
    }
    // Handler response
    if (msg->type == KM0_KM4_IPC_MSG_RESP && expectedRespReqId_ == msg->req_id) {
        if (respCallback_) {
            respCallback_(msg, respCallbackContext_);
        }
        expectedRespReqId_ = INVALID_IPC_REQ_ID;
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
        : channel_(ipcChannel) {
    reqId_ = 0;
    expectedRespReqId_ = INVALID_IPC_REQ_ID;
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
