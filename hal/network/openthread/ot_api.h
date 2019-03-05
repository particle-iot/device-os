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

/**
 * @file
 * @brief
 *  This file defines the POSIX-compatible inet_hal and associated types.
 */

#ifndef NETWORK_OPENTHREAD_OT_API_H
#define NETWORK_OPENTHREAD_OT_API_H

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include "hal_platform.h"

#include "check.h"

#if HAL_OPENTHREAD_USE_LWIP_LOCK
#include "lwiplock.h"
#endif // HAL_OPENTHREAD_USE_LWIP_LOCK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHECK_THREAD(_expr) \
        do { \
            const auto ret = _expr; \
            if (ret != OT_ERROR_NONE) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", (int)ret); \
                return ot_system_error(ret); \
            } \
        } while (false)

// Does not convert otErrors to system_error_t
#define CHECK_THREAD_OTERR(_expr) \
        do { \
            const auto ret = _expr; \
            if (ret != OT_ERROR_NONE) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", (int)ret); \
                return ret; \
            } \
        } while (false)

/**
 * @addtogroup ot_api
 *
 * @brief
 *   This module provides utility functions for managing OpenThread.
 *
 * @{
 *
 */

/**
 * Initializes OpenThread and starts its event loop.
 *
 * @param      onInit    Function to be called immediately after creating the OpenThread
 *                       instance and starting its even loop for platform-specific
 *                       configuration. Can be NULL.
 * @param      reserved  The reserved
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_init(int (*onInit)(otInstance*), void* reserved);

/**
 * Returns the OpenThread instance object (otInstance)
 *
 * @returns    OpenThread instance object, or NULL in case of an error
 */
otInstance* ot_get_instance();

/**
 * Acquires the lock for exclusive access to the OpenThread API.
 *
 * @param      reserved  The reserved
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_lock(void* reserved);

/**
 * Releases the lock for exclusive access to the OpenThread API.
 *
 * @param      reserved  The reserved
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_unlock(void* reserved);

/**
 * Converts OpenThread otError to system_error_t
 *
 * @param[in]  error  otError to be converted
 *
 * @returns    system_error_t corresponding to the otError
 */
int ot_system_error(otError error);

/**
 * Retrieves a particular Border Router prefix from the persistent storage.
 *
 * @param      ot      OpenThread instance object
 * @param[in]  idx     The index of the Border Router prefix
 * @param[out] prefix  A pointer to otIp6Prefix, which will contain the retreived prefix
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_get_border_router_prefix(otInstance* ot, unsigned int idx, otIp6Prefix* prefix);

/**
 * Stores a particular Border Router prefix in persistent storage.
 *
 * @param      ot      OpenThread instance object
 * @param[in]  idx     The index of the Border Router prefix
 * @param[in]  prefix  The prefix to store
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_set_border_router_prefix(otInstance* ot, unsigned int idx, const otIp6Prefix* prefix);

/**
 * Retrieves Particle-specific Network Id from the persistent storage.
 *
 * @param           ot      OpenThread instance object
 * @param[out]      buf     Buffer to store the retrieved Network Id
 * @param[inout]    buflen  Size of the buffer, on return will contain the actual length
 *                          of the retrieved Network Id
 *
 * @returns         0 on success, system_error_t on error.
 */
int ot_get_network_id(otInstance* ot, char* buf, size_t* buflen);

/**
 * Stores Particle-specific Network Id in persistent storage.
 *
 * @param      ot       OpenThread instance object
 * @param[in]  buf      Buffer containing the Network Id
 * @param[in]  buflen   Size of the Network Id
 *
 * @returns    0 on success, system_error_t on error.
 */
int ot_set_network_id(otInstance* ot, const char* buf, size_t buflen);

#ifdef __cplusplus
}

namespace particle { namespace net { namespace ot {

#if !HAL_OPENTHREAD_USE_LWIP_LOCK
class ThreadLock {
public:
    ThreadLock() :
            locked_(false) {
        lock();
    }

    ~ThreadLock() {
        if (locked_) {
            unlock();
        }
    }

    ThreadLock(ThreadLock&& lock) :
            locked_(lock.locked_) {
        lock.locked_ = false;
    }

    void lock() {
        ot_lock(nullptr);
        locked_ = true;
    }

    void unlock() {
        ot_unlock(nullptr);
        locked_ = false;
    }

    ThreadLock(const ThreadLock&) = delete;
    ThreadLock& operator=(const ThreadLock&) = delete;

private:
    bool locked_;
};
#else

using ThreadLock = particle::net::LwipTcpIpCoreLock;

#endif // HAL_OPENTHREAD_USE_LWIP_LOCK

} } } /* particle::net::ot */

/**
 * @}
 *
 */

#endif /* __cplusplus */

#endif /* NETWORK_OPENTHREAD_OT_API_H */
