/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "concurrent_hal.h"

os_result_t os_thread_create(os_thread_t* thread, const char* name, os_thread_prio_t priority, os_thread_fn_t fun, void* thread_param, size_t stack_size)
{
    return 0;
}

os_thread_t os_thread_current(void* reserved)
{
    return nullptr;
}

bool os_thread_is_current(os_thread_t thread)
{
    return false;
}

os_result_t os_thread_yield(void)
{
    return 0;
}

bool os_thread_current_within_stack()
{
    return true;
}

os_result_t os_thread_join(os_thread_t thread)
{
    return 0;
}

os_result_t os_thread_exit(os_thread_t thread)
{
    return 0;
}

os_result_t os_thread_cleanup(os_thread_t thread)
{
    return 0;
}

os_result_t os_thread_delay_until(system_tick_t *previousWakeTime, system_tick_t timeIncrement)
{
    return 0;
}

os_thread_notify_t os_thread_wait(system_tick_t ms, void* reserved)
{
    return 0;
}

int os_thread_notify(os_thread_t thread, void* reserved)
{
    return 0;
}

int os_queue_create(os_queue_t* queue, size_t item_size, size_t item_count, void*)
{
    return 0;
}

int os_queue_put(os_queue_t queue, const void* item, system_tick_t delay, void*)
{
    return 0;
}

int os_queue_take(os_queue_t queue, void* item, system_tick_t delay, void*)
{
    return 0;
}

int os_queue_peek(os_queue_t queue, void* item, system_tick_t delay, void*)
{
    return 0;
}

int os_queue_destroy(os_queue_t queue, void*)
{
    return 0;
}

int os_mutex_create(os_mutex_t* mutex)
{
    return 0;
}

int os_mutex_destroy(os_mutex_t mutex)
{
    return 0;
}

int os_mutex_lock(os_mutex_t mutex)
{
    return 0;
}

int os_mutex_trylock(os_mutex_t mutex)
{
    return 0;
}

int os_mutex_unlock(os_mutex_t mutex)
{
    return 0;
}

int os_mutex_recursive_create(os_mutex_recursive_t* mutex)
{
    return 0;
}

int os_mutex_recursive_destroy(os_mutex_recursive_t mutex)
{
    return 0;
}

int os_mutex_recursive_lock(os_mutex_recursive_t mutex)
{
    return 0;
}

int os_mutex_recursive_trylock(os_mutex_recursive_t mutex)
{
    return 0;
}

int os_mutex_recursive_unlock(os_mutex_recursive_t mutex)
{
    return 0;
}

void os_thread_scheduling(bool enabled, void* reserved)
{
}

os_scheduler_state_t os_scheduler_get_state(void* reserved)
{
    return OS_SCHEDULER_STATE_NOT_STARTED;
}

int os_semaphore_create(os_semaphore_t* semaphore, unsigned max, unsigned initial)
{
    return 0;
}

int os_semaphore_destroy(os_semaphore_t semaphore)
{
    return 0;
}

int os_semaphore_take(os_semaphore_t semaphore, system_tick_t timeout, bool reserved)
{
    return 0;
}

int os_semaphore_give(os_semaphore_t semaphore, bool reserved)
{
    return 0;
}

/**
 * Create a new timer. Returns 0 on success.
 */
int os_timer_create(os_timer_t* timer, unsigned period, void (*callback)(os_timer_t timer), void* const timer_id, bool one_shot, void* reserved)
{
    return 0;
}

int os_timer_get_id(os_timer_t timer, void** timer_id)
{
    return 0;
}

int os_timer_set_id(os_timer_t timer, void* timer_id)
{
    return 0;
}

int os_timer_change(os_timer_t timer, os_timer_change_t change, bool fromISR, unsigned period, unsigned block, void* reserved)
{
    return 0;
}

int os_timer_destroy(os_timer_t timer, void* reserved)
{
    return 0;
}

int os_timer_is_active(os_timer_t timer, void* reserved)
{
    return 0;
}

void __flash_acquire() {
}

void __flash_release() {
}

void periph_lock() {
}

void periph_unlock() {
}
