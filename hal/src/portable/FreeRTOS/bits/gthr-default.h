/**
 * This file allows us to inject the appropriate implementation of mutexes and other
 * concurrency primitives that aren't supported natively by GCC ARM.
 */
#ifndef GTHR_DEFAULT_H
#define GTHR_DEFAULT_H

#include "concurrent_hal.h"

#define PARTICLE_GTHREAD_INCLUDED 1

#ifdef __cplusplus

#include <atomic>

typedef int __gthread_key_t;

typedef enum __gthread_once_state_t {
    GTHREAD_ONCE_STATE_NOT_INITIALIZED = 0,
    GTHREAD_ONCE_STATE_RUNNING = 1,
    GTHREAD_ONCE_STATE_INITIALIZED = 2
} __gthread_once_state_t;

typedef std::atomic<__gthread_once_state_t> __gthread_once_t;

#define _GLIBCXX_UNUSED __attribute__((unused))

static inline int
__gthread_active_p (void)
{
    return 0;
}

extern "C" int __gthread_once (__gthread_once_t* once, void (*func) (void));

static inline int _GLIBCXX_UNUSED
__gthread_key_create (__gthread_key_t *__key _GLIBCXX_UNUSED, void (*__func) (void *) _GLIBCXX_UNUSED)
{
    return 0;
}

static int _GLIBCXX_UNUSED
__gthread_key_delete (__gthread_key_t __key _GLIBCXX_UNUSED)
{
    return 0;
}

static inline void *
__gthread_getspecific (__gthread_key_t __key _GLIBCXX_UNUSED)
{
    return 0;
}

static inline int
__gthread_setspecific (__gthread_key_t __key _GLIBCXX_UNUSED, const void *__v _GLIBCXX_UNUSED)
{
    return 0;
}

static inline int
__gthread_mutex_destroy (__gthread_mutex_t *__mutex)
{
    return os_mutex_destroy(*__mutex);
}

static inline int
__gthread_mutex_lock (__gthread_mutex_t *__mutex)
{
    return os_mutex_lock(*__mutex);
}

static inline int
__gthread_mutex_trylock (__gthread_mutex_t *__mutex)
{
    return os_mutex_trylock(*__mutex);
}

static inline int
__gthread_mutex_unlock (__gthread_mutex_t *__mutex)
{
    return os_mutex_unlock(*__mutex);
}

static inline int
__gthread_recursive_mutex_lock (__gthread_recursive_mutex_t *__mutex)
{
    return os_mutex_recursive_lock(*__mutex);
}

static inline int
__gthread_recursive_mutex_trylock (__gthread_recursive_mutex_t *__mutex)
{
    return os_mutex_recursive_trylock(*__mutex);
}

static inline int
__gthread_recursive_mutex_unlock (__gthread_recursive_mutex_t *__mutex)
{
    return os_mutex_recursive_unlock(*__mutex);
}

static inline int
__gthread_recursive_mutex_destroy (__gthread_recursive_mutex_t *__mutex)
{
    return os_mutex_recursive_destroy(*__mutex);
}

#define __GTHREAD_ONCE_INIT ATOMIC_VAR_INIT(GTHREAD_ONCE_STATE_NOT_INITIALIZED)
#define __GTHREAD_MUTEX_INIT_FUNCTION(mx)  os_mutex_create(mx)
#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION(mx) os_mutex_recursive_create(mx)

#endif /* __cplusplus */

#endif  /* GTHR_DEFAULT_H */
