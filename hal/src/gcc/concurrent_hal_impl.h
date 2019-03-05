/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
typedef uintptr_t os_unique_id_t;

#define OS_THREAD_PRIORITY_DEFAULT (0)
#define OS_THREAD_STACK_SIZE_DEFAULT (0)
