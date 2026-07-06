/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "concurrent_hal.h"
#include "service_debug.h"
#include "iperf_config.h"

typedef struct pthread_cb {
    os_thread_t handle;
    void* (*start_routine)(void*);
    void* arg;
    void* retval;
    os_semaphore_t done;
} pthread_cb_t;

static os_thread_return_t pthread_entry(void* param) {
    pthread_cb_t* cb = (pthread_cb_t*)param;
    cb->retval = cb->start_routine(cb->arg);
    os_semaphore_give(cb->done, false);
    os_thread_exit(NULL);
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
        void* (*start_routine)(void*), void* arg) {
    (void)attr;
    pthread_cb_t* cb = (pthread_cb_t*)calloc(1, sizeof(pthread_cb_t));
    if (!cb) {
        return EAGAIN;
    }
    cb->start_routine = start_routine;
    cb->arg = arg;
    if (os_semaphore_create(&cb->done, 1, 0) != 0) {
        free(cb);
        return EAGAIN;
    }
    if (os_thread_create(&cb->handle, "pthread", OS_THREAD_PRIORITY_DEFAULT + 1, pthread_entry,
            cb, IPERF_WORKER_THREAD_STACK_SIZE) != 0) {
        os_semaphore_destroy(cb->done);
        free(cb);
        return EAGAIN;
    }
    *thread = (pthread_t)(uintptr_t)cb;
    return 0;
}

int pthread_join(pthread_t thread, void** retval) {
    pthread_cb_t* cb = (pthread_cb_t*)(uintptr_t)thread;
    if (!cb) {
        return ESRCH;
    }
    os_semaphore_take(cb->done, 60000, false);
    os_thread_join(cb->handle);
    os_semaphore_destroy(cb->done);
    if (retval) {
        *retval = cb->retval;
    }
    free(cb);
    return 0;
}

void pthread_exit(void* retval) {
    (void)retval;
    os_thread_exit(NULL);
    // os_thread_exit() does not return
    for (;;) {
    }
}

pthread_t pthread_self(void) {
    return (pthread_t)(uintptr_t)os_thread_current(NULL);
}

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

int pthread_cancel(pthread_t thread) {
    (void)thread;
    return 0;
}

int pthread_setcancelstate(int state, int* oldstate) {
    (void)state;
    if (oldstate) {
        *oldstate = PTHREAD_CANCEL_ENABLE;
    }
    return 0;
}

int pthread_setcanceltype(int type, int* oldtype) {
    (void)type;
    if (oldtype) {
        *oldtype = PTHREAD_CANCEL_DEFERRED;
    }
    return 0;
}

int pthread_attr_init(pthread_attr_t* attr) {
    (void)attr;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t* attr) {
    (void)attr;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    (void)attr;
    os_mutex_t m = NULL;
    if (os_mutex_create(&m) != 0) {
        return EAGAIN;
    }
    *mutex = (pthread_mutex_t)(uintptr_t)m;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    os_mutex_t m = (os_mutex_t)(uintptr_t)*mutex;
    if (!m) {
        return EINVAL;
    }
    os_mutex_destroy(m);
    *mutex = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    return os_mutex_lock((os_mutex_t)(uintptr_t)*mutex) == 0 ? 0 : EINVAL;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    return os_mutex_trylock((os_mutex_t)(uintptr_t)*mutex) == 0 ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    return os_mutex_unlock((os_mutex_t)(uintptr_t)*mutex) == 0 ? 0 : EINVAL;
}

int pthread_mutexattr_init(pthread_mutexattr_t* attr) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* attr) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type) {
    (void)attr;
    (void)type;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type) {
    (void)attr;
    if (type) {
        *type = PTHREAD_MUTEX_NORMAL;
    }
    return 0;
}

int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) {
    (void)how;
    (void)set;
    (void)oldset;
    return 0;
}
