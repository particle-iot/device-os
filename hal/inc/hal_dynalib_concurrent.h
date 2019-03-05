/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef HAL_DYNALIB_CONCURRENT_H
#define	HAL_DYNALIB_CONCURRENT_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "concurrent_hal.h"
#endif

DYNALIB_BEGIN(hal_concurrent)

#if PLATFORM_THREADING
DYNALIB_FN(0, hal_concurrent, __gthread_equal, bool(__gthread_t, __gthread_t))
DYNALIB_FN(1, hal_concurrent, os_thread_create, os_result_t(os_thread_t*, const char*, os_thread_prio_t, os_thread_fn_t, void*, size_t))
DYNALIB_FN(2, hal_concurrent, os_thread_is_current, bool(os_thread_t))
DYNALIB_FN(3, hal_concurrent, os_thread_yield, os_result_t(void))
DYNALIB_FN(4, hal_concurrent, os_thread_join, os_result_t(os_thread_t))
DYNALIB_FN(5, hal_concurrent, os_thread_cleanup, os_result_t(os_thread_t))
DYNALIB_FN(6, hal_concurrent, os_thread_delay_until, os_result_t(system_tick_t*, system_tick_t))
DYNALIB_FN(7, hal_concurrent, os_thread_scheduling, void(bool, void*))

DYNALIB_FN(8, hal_concurrent, os_timer_create, int(os_timer_t*, unsigned, void(*)(os_timer_t), void*, bool, void*))
DYNALIB_FN(9, hal_concurrent, os_timer_destroy, int(os_timer_t, void*))
DYNALIB_FN(10, hal_concurrent, os_timer_get_id, int(os_timer_t, void**))
DYNALIB_FN(11, hal_concurrent, os_timer_change, int(os_timer_t, os_timer_change_t, bool, unsigned, unsigned, void*))

DYNALIB_FN(12, hal_concurrent, os_mutex_create, int(os_mutex_t*))
DYNALIB_FN(13, hal_concurrent, os_mutex_destroy, int(os_mutex_t))
DYNALIB_FN(14, hal_concurrent, os_mutex_lock, int(os_mutex_t))
DYNALIB_FN(15, hal_concurrent, os_mutex_trylock, int(os_mutex_t))
DYNALIB_FN(16, hal_concurrent, os_mutex_unlock, int(os_mutex_t))

DYNALIB_FN(17, hal_concurrent, os_mutex_recursive_create, int(os_mutex_recursive_t*))
DYNALIB_FN(18, hal_concurrent, os_mutex_recursive_destroy, int(os_mutex_recursive_t))
DYNALIB_FN(19, hal_concurrent, os_mutex_recursive_lock, int(os_mutex_recursive_t))
DYNALIB_FN(20, hal_concurrent, os_mutex_recursive_trylock, int(os_mutex_recursive_t))
DYNALIB_FN(21, hal_concurrent, os_mutex_recursive_unlock, int(os_mutex_recursive_t))

DYNALIB_FN(22, hal_concurrent, os_timer_is_active, int(os_timer_t, void*))

DYNALIB_FN(23, hal_concurrent, os_queue_create, int(os_queue_t*, size_t, size_t, void*))
DYNALIB_FN(24, hal_concurrent, os_queue_destroy, int(os_queue_t, void*))
DYNALIB_FN(25, hal_concurrent, os_queue_put, int(os_queue_t, const void* item, system_tick_t, void*))
DYNALIB_FN(26, hal_concurrent, os_queue_take, int(os_queue_t, void* item, system_tick_t, void*))
DYNALIB_FN(27, hal_concurrent, os_thread_exit, os_result_t(os_thread_t))

DYNALIB_FN(28, hal_concurrent, os_timer_set_id, int(os_timer_t, void*))
#endif // PLATFORM_THREADING

DYNALIB_END(hal_concurrent)



#endif	/* HAL_DYNALIB_CONCURRENT_H */

