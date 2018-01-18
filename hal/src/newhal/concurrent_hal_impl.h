#pragma once

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
typedef uintptr_t os_unique_id_t;
