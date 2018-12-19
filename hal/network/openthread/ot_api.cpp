/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
LOG_SOURCE_CATEGORY("ot.api");

#include "ot_api.h"
#include <openthread-core-config.h>
#include <openthread/thread.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/instance.h>
#include <openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/platform/settings.h>
#include "concurrent_hal.h"
#include "system_error.h"
#include "static_recursive_mutex.h"
#include <mutex>
#include <limits>
#include "check.h"

using namespace particle::net::ot;

namespace {

// Particle-specific data indexes in OpenThread persisted settings
// Network Id
const uint16_t KEY_NETWORK_ID = 0x4000;
// Border router prefixes
const uint16_t KEY_BORDER_ROUTER_PREFIX_BASE = 0x4100;

otInstance* s_threadInstance = nullptr;
StaticRecursiveMutex s_threadMutex;
os_thread_t s_threadThread = nullptr;
os_semaphore_t s_threadSem = nullptr;

otInstance* allocInstance() {
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    size_t size = 0;
    otInstanceInit(nullptr, &size);
    void* const buf = calloc(1, size);
    if (!buf) {
        return nullptr;
    }
    return otInstanceInit(buf, &size);
#else
    return otInstanceInitSingle();
#endif
}

void ot_process(void* arg) {
    otInstance* thread = (otInstance*)arg;
    while (true) {
        os_semaphore_take(s_threadSem, CONCURRENT_WAIT_FOREVER, false);
        {
            ThreadLock lk;
            while (otTaskletsArePending(thread)) {
                otTaskletsProcess(thread);
            }
            otSysProcessDrivers(thread);
        }
    }
    os_thread_exit(nullptr);
}

} /* anonymous */

void otTaskletsSignalPending(otInstance* instance)
{
    os_semaphore_give(s_threadSem, false);
}

void otSysEventSignalPending(void)
{
    /* May be executed from an ISR. os_semaphore_give() should be able to handle this */
    os_semaphore_give(s_threadSem, false);
}

int ot_init(int (*onInit)(otInstance*), void* reserved) {
    ThreadLock lk;
    if (s_threadInstance != nullptr) {
        /* Already initialized */
        return SYSTEM_ERROR_ALREADY_EXISTS;
    }

    if (os_semaphore_create(&s_threadSem, 1, 0)) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    otSysInit(0, nullptr);

    otInstance* const thread = allocInstance();
    if (!thread) {
        LOG(ERROR, "Unable to initialize OpenThread");
        return SYSTEM_ERROR_UNKNOWN;
    }

    LOG(INFO, "OpenThread version: %s", otGetVersionString());

    if (os_thread_create(&s_threadThread, "ot", OS_THREAD_PRIORITY_NETWORK_HIGH,
                         ot_process, thread, OS_THREAD_STACK_SIZE_DEFAULT_NETWORK)) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    s_threadInstance = thread;

    otLinkModeConfig mode = {};
    mode.mRxOnWhenIdle = true;
    mode.mSecureDataRequests = true;
    mode.mDeviceType = true;
    mode.mNetworkData = true;
    // FIXME: CHECK_THREAD()
    CHECK_TRUE(otThreadSetLinkMode(thread, mode) == OT_ERROR_NONE, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(otPlatRadioSetTransmitPower(thread, (int8_t)HAL_PLATFORM_OPENTHREAD_MAX_TX_POWER) == OT_ERROR_NONE,
            SYSTEM_ERROR_UNKNOWN);

    int8_t transmitPower = 0;
    CHECK_TRUE(otPlatRadioGetTransmitPower(thread, &transmitPower) == OT_ERROR_NONE, SYSTEM_ERROR_UNKNOWN);
    LOG(INFO, "Max transmit power: %d", (int)transmitPower);

    if (otDatasetIsCommissioned(thread)) {
        LOG(INFO, "Network name: %s", otThreadGetNetworkName(thread));
        LOG(INFO, "802.15.4 channel: %d", (int)otLinkGetChannel(thread));
        LOG(INFO, "802.15.4 PAN ID: 0x%04x", (unsigned)otLinkGetPanId(thread));
    }

    if (onInit != nullptr) {
        onInit(thread);
    }

    /* Start event loop */
    otTaskletsSignalPending(thread);

    return 0;
}

otInstance* ot_get_instance() {
    ThreadLock lk;
    return s_threadInstance;
}

int ot_lock(void* reserved) {
    return !s_threadMutex.lock();
}

int ot_unlock(void* reserved) {
    return !s_threadMutex.unlock();
}

int ot_system_error(otError error) {
    switch (error) {
    case OT_ERROR_NONE:
        return SYSTEM_ERROR_NONE;
    case OT_ERROR_SECURITY:
        return SYSTEM_ERROR_NOT_ALLOWED;
    case OT_ERROR_NOT_FOUND:
        return SYSTEM_ERROR_NOT_FOUND;
    case OT_ERROR_RESPONSE_TIMEOUT:
        return SYSTEM_ERROR_TIMEOUT;
    case OT_ERROR_NO_BUFS:
        return SYSTEM_ERROR_NO_MEMORY;
    case OT_ERROR_BUSY:
        return SYSTEM_ERROR_BUSY;
    case OT_ERROR_ABORT:
        return SYSTEM_ERROR_ABORTED;
    case OT_ERROR_INVALID_STATE:
        return SYSTEM_ERROR_INVALID_STATE;
    default:
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int ot_get_border_router_prefix(otInstance* ot, unsigned int idx, otIp6Prefix* prefix) {
    ThreadLock lk;
    uint16_t sz = sizeof(otIp6Prefix);
    auto result = otPlatSettingsGet(ot, KEY_BORDER_ROUTER_PREFIX_BASE + idx, 0,
            (uint8_t*)prefix, &sz);
    return ot_system_error(result);
}

int ot_set_border_router_prefix(otInstance* ot, unsigned int idx, const otIp6Prefix* prefix) {
    ThreadLock lk;
    auto result = otPlatSettingsSet(ot, KEY_BORDER_ROUTER_PREFIX_BASE + idx,
            (const uint8_t*)prefix, sizeof(otIp6Prefix));
    return ot_system_error(result);
}

int ot_get_network_id(otInstance* ot, char* buf, size_t* buflen) {
    ThreadLock lk;
    uint16_t sz = *buflen;
    auto result = otPlatSettingsGet(ot, KEY_NETWORK_ID, 0, (uint8_t*)buf, &sz);
    *buflen = sz;
    return ot_system_error(result);
}

int ot_set_network_id(otInstance* ot, const char* buf, size_t buflen) {
    ThreadLock lk;
    auto result = otPlatSettingsSet(ot, KEY_NETWORK_ID, (const uint8_t*)buf, buflen);
    return ot_system_error(result);
}
