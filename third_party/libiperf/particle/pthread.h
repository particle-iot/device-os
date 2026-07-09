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

#pragma once

#include <sys/types.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

// PTHREAD_CREATE_JOINABLE/DETACHED may already come from newlib <sys/types.h>
#ifndef PTHREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_JOINABLE 0
#endif
#ifndef PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED 1
#endif

#ifndef PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_ENABLE 0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
#define PTHREAD_CANCELED ((void*)-1)
#endif

#ifndef PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE 2
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
#endif

int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
        void* (*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void** retval);
void pthread_exit(void* retval) __attribute__((noreturn));
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);

int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int* oldstate);
int pthread_setcanceltype(int type, int* oldtype);

int pthread_attr_init(pthread_attr_t* attr);
int pthread_attr_destroy(pthread_attr_t* attr);

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

int pthread_mutexattr_init(pthread_mutexattr_t* attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t* attr);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type);

#if !defined(__POSIX_VISIBLE) || __POSIX_VISIBLE < 199506
int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset);
#endif

#ifdef __cplusplus
}
#endif
