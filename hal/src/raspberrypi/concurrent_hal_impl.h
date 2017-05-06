
#ifndef CONCURRENT_HAL_IMPL_H
#define	CONCURRENT_HAL_IMPL_H

#ifdef	__cplusplus
extern "C" {
#endif

// Alias thread primitive to the HAL definitions

typedef void* os_thread_t;
typedef int32_t os_result_t;
typedef uint8_t os_thread_prio_t;
/* No thread priorities */
const os_thread_prio_t OS_THREAD_PRIORITY_DEFAULT = 0;
const os_thread_prio_t OS_THREAD_PRIORITY_CRITICAL = 0;
/* No stack size override */
const size_t OS_THREAD_STACK_SIZE_DEFAULT = 0;

typedef void* os_mutex_t;
typedef void* os_mutex_recursive_t;
typedef void* condition_variable_t;
typedef void* os_timer_t;

/**
 * Alias for a queue handle - all handles are pointers.
 */
typedef void* os_queue_t;
typedef void* os_semaphore_t;

#ifdef	__cplusplus
}
#endif

#endif	/* CONCURRENT_HAL_IMPL_H */

