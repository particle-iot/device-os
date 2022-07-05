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

#ifndef CONCURRENT_HAL_IMPL_H
#define CONCURRENT_HAL_IMPL_H

#define OS_TIMER_INVALID_HANDLE NULL

#ifdef  __cplusplus
extern "C" {
#endif

// This code is used by HAL-clients which don't have access to the FreeRTOS sources
// so we cannot directly define __gthread_t as TaskHandle_t, however, we define it
// here and statically assert that it is the same size.

typedef void* os_thread_t;
typedef os_thread_t __gthread_t;
typedef int32_t os_result_t;
typedef uint8_t os_thread_prio_t;
/* Default priority is the same as the application thread */
#define OS_THREAD_PRIORITY_DEFAULT       (2)
#define OS_THREAD_PRIORITY_CRITICAL      (9)
#define OS_THREAD_PRIORITY_NETWORK       (7)
#define OS_THREAD_PRIORITY_NETWORK_HIGH  (8)
#define OS_THREAD_STACK_SIZE_DEFAULT (3*1024)
#define OS_THREAD_STACK_SIZE_DEFAULT_HIGH (4*1024)
#define OS_THREAD_STACK_SIZE_DEFAULT_NETWORK (6*1024)

typedef uint32_t os_thread_notify_t;

typedef void* os_mutex_t;
typedef void* os_mutex_recursive_t;
typedef void* condition_variable_t;
typedef void* os_timer_t;
typedef uint32_t os_unique_id_t;

typedef os_mutex_t __gthread_mutex_t;
typedef os_mutex_recursive_t __gthread_recursive_mutex_t;


/**
 * Alias for a queue handle in FreeRTOS - all handles are pointers.
 */
typedef void* os_queue_t;
typedef void* os_semaphore_t;

typedef struct timespec __gthread_time_t;

bool __gthread_equal(__gthread_t t1, __gthread_t t2);
__gthread_t __gthread_self();

typedef condition_variable_t __gthread_cond_t;

int __gthread_cond_timedwait (__gthread_cond_t *cond,
                                   __gthread_mutex_t *mutex,
                                   const __gthread_time_t *abs_timeout);


int __gthread_mutex_timedlock (__gthread_mutex_t* mutex, const __gthread_time_t* timeout);

int __gthread_recursive_mutex_timedlock (__gthread_recursive_mutex_t* mutex, const __gthread_time_t* timeout);

// MOD_FUNC_MONO_FIRMWARE = 3
// MOD_FUNC_SYSTEM_PART = 4
#if (MODULE_FUNCTION == 3) || (MODULE_FUNCTION == 4)
// There is a symbol conflict with the SDK, so in system we use non-conflicting symbol names
// but when importing into the app, we'll use the regular one.
#define os_queue_peek           os_queue_peek_workaround
#define os_mutex_create         os_mutex_create_workaround
#define os_timer_create         os_timer_create_workaround
#endif // MODULE_FUNCTION == MOD_FUNC_SYSTEM_PART

#ifdef  __cplusplus
}
#endif

#endif  /* CONCURRENT_HAL_IMPL_H */
