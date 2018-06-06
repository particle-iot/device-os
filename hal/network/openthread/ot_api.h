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

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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

#ifdef __cplusplus
}

namespace particle { namespace net { namespace ot {

/**
 * Convenience struct meeting BasicLockable requirements for
 * use with std::lock_guard/unique_lock/scoped_lock
 */
struct ThreadLock {
    static void lock() {
        ot_lock(nullptr);
    }

    static void unlock() {
        ot_unlock(nullptr);
    }
};

} } } /* particle::net::ot */

/**
 * @}
 *
 */

#endif /* __cplusplus */

#endif /* NETWORK_OPENTHREAD_OT_API_H */
