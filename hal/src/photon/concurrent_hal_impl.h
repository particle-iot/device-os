
#ifndef CONCURRENT_HAL_IMPL_H
#define	CONCURRENT_HAL_IMPL_H

#ifdef	__cplusplus
extern "C" {
#endif


// This code is used by HAL-clients which don't have access to the FreeRTOS sources
// so we cannot directly define __gthread_t as TaskHandle_t, however, we define it
// here and statically assert that it is the same size.

typedef void* __gthread_t;

typedef void* os_thread_t;
typedef int32_t os_result_t;
typedef uint8_t os_thread_prio_t;
const os_thread_prio_t OS_THREAD_PRIORITY_DEFAULT = 0;
const size_t OS_THREAD_STACK_SIZE_DEFAULT = 512;


typedef struct timespec __gthread_time_t;

#define _GLIBCXX_HAS_GTHREADS
#include <bits/gthr.h>

bool __gthread_equal(__gthread_t t1, __gthread_t t2);
__gthread_t __gthread_self();

typedef struct { uint8_t tmp[4]; } __gthread_cond_t;
typedef __gthread_cond_t condition_variable_t;

int __gthread_cond_timedwait (__gthread_cond_t *cond,
                                   __gthread_mutex_t *mutex,
                                   const __gthread_time_t *abs_timeout);


#ifdef	__cplusplus
}
#endif

#endif	/* CONCURRENT_HAL_IMPL_H */

