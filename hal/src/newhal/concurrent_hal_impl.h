#pragma once

#include <bits/gthr-default.h>
#include <time.h>

// redefine these for the underlying concurrency primitives available in the RTOS

typedef int os_result_t;
typedef int os_thread_prio_t;
typedef void* os_thread_t;
typedef void*os_timer_t;
typedef void*os_queue_t;
typedef void*os_mutex_t;
typedef void* condition_variable_t;
typedef void* os_semaphore_t;
typedef void* os_mutex_recursive_t;
typedef struct timespec __gthread_time_t;
typedef void* os_thread_notify_t;
typedef uintptr_t os_unique_id_t;

#define OS_THREAD_PRIORITY_DEFAULT (0)
#define OS_THREAD_STACK_SIZE_DEFAULT (0)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int __gthread_mutex_timedlock (__gthread_mutex_t* mutex, const __gthread_time_t* timeout);

int __gthread_recursive_mutex_timedlock (__gthread_recursive_mutex_t* mutex, const __gthread_time_t* timeout);

#ifdef __cplusplus
}
#endif // __cplusplus