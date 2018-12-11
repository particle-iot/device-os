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
#include <openthread/openthread.h>
#include <openthread/thread.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/instance.h>
#include <openthread/platform.h>
#include <openthread/tasklet.h>
#include "concurrent_hal.h"
#include "system_error.h"
#include "static_recursive_mutex.h"
#include <mutex>
#include <limits>
#include "check.h"

using namespace particle::net::ot;

namespace {

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
            otTaskletsProcess(thread);
            PlatformProcessDrivers(thread);
        }
    }
    os_thread_exit(nullptr);
}

} /* anonymous */

void otTaskletsSignalPending(otInstance* instance)
{
    os_semaphore_give(s_threadSem, false);
}

void PlatformEventSignalPending(void)
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

    PlatformInit(0, nullptr);

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
