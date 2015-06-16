/**
 ******************************************************************************
 * @file    concurrency_hal.cpp
 * @authors Matthew McGowan
 * @date    03 March 2015
 ******************************************************************************
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

#include "concurrent_hal.h"
#include "wiced.h"
#include "static_assert.h"
#include "task.h"
#include <mutex>

// use the newer name
typedef xTaskHandle TaskHandle_t;
typedef xQueueHandle QueueHandle_t;
typedef xSemaphoreHandle SemaphoreHandle_t;
typedef portTickType TicksType_t;

typedef unsigned portBASE_TYPE UBaseType_t;

static_assert(sizeof(TaskHandle_t)==sizeof(__gthread_t), "__gthread_t should be the same size as TaskHandle_t");

static_assert(sizeof(uint32_t)==sizeof(void*), "Requires uint32_t to be same size as void* for function cast to wiced_thread_function_t");

/**
 * Creates a new thread.
 * @param result            Receives the created thread handle. Will be set to NULL if the thread cannot be created.
 * @param name              The name of the thread. May be used if the underlying RTOS supports it. Can be null.
 * @param priority          The thread priority. It's best to stick to a small range of priorities, e.g. +/- 7.
 * @param fun               The function to execute in a separate thread.
 * @param thread_param      The parameter to pass to the thread function.
 * @param stack_size        The size of the stack to create. The stack is allocated on the heap.
 * @return an error code. 0 if the thread was successfully created.
 */
os_result_t os_thread_create(os_thread_t* result, const char* name, os_thread_prio_t priority, os_thread_fn_t fun, void* thread_param, size_t stack_size)
{
    wiced_thread_t* pthread = new wiced_thread_t();
    wiced_result_t error = wiced_rtos_create_thread(pthread, priority, name, wiced_thread_function_t(fun), stack_size, thread_param );
    if (error) {
        delete pthread;
        *result = NULL;
    }
    else {
        *result = pthread;
    }
    return error;
}

/**
 *
 * @param result            Receives the created thread handle. Will be set to NULL if the thread cannot be created.
 * @param name              The name of the thread. May be used if the underlying RTOS supports it.
 * @param priority          The thread priority. It's best to stick to a small range of priorities, e.g. +/- 7.
 * @param fun               The function to execute in a separate thread.
 * @param thread_param      The parameter to pass to the thread function.
 * @param stack_size        The size of the stack to create. The stack is allocated on the heap.
 * @param stack             The location of the bottom of the stack. The top of the stack is at location stack + stack_size.
 */
os_result_t os_thread_create_with_stack(os_thread_t* result, const char* name, os_thread_prio_t priority, os_thread_fn_t fun, void* thread_param, size_t stack_size, void* stack)
{
    wiced_thread_t* pthread = new wiced_thread_t();
    wiced_result_t error = wiced_rtos_create_thread_with_stack(pthread, priority, name, wiced_thread_function_t(fun), stack, stack_size, thread_param);
    if (error) {
        delete pthread;
        *result = NULL;
    }
    else {
        *result = pthread;
    }
    return error;
}

/**
 * Determines if the given thread is the one executing.
 * @param   The thread to test.
 * @return {@code true} if the thread given is the one currently executing. {@code false} otherwise.
 */
bool os_thread_current(os_thread_t* thread)
{
    return wiced_rtos_is_current_thread((wiced_thread_t*)thread)==WICED_SUCCESS;
}

/**
 * Determines if the thread stack is still within the allocated region.
 * @param thread    The thread to check
 * @return      {@code true} if the thread is still within the allocated stack region.
 *  {@code false} if the thread has gone through the bottom of the stack. Depending upon where the stack was
 *  allocated this may cause corruption of system or user data. As a precaution, the device should be reset unless
 * measures have been taken to ensure the region of memory the stack grows into is unused (such as overallocating the stack size.)
 */
bool os_thread_current_within_stack()
{
    return wiced_rtos_check_stack()==WICED_SUCCESS;
}

/**
 * Waits indefinitely for the given thread to finish.
 * @param thread    The thread to wait for.
 * @return 0 if the thread has successfully terminated. non-zero if the thread handle is not valid.
 */
os_result_t os_thread_join(os_thread_t* thread)
{
    return wiced_rtos_thread_join((wiced_thread_t*)thread);
}

/**
 * Cleans up resources used by a terminated thread.
 * @param thread    The thread to clean up.
 * @return 0 on success.
 */
os_result_t os_thread_cleanup(os_thread_t* thread)
{
    return wiced_rtos_delete_thread((wiced_thread_t*)thread);
}

/**
 * Map gthread handles to FreeRTOS task handles.
 */
typedef TaskHandle_t __gthread_t;


class ThreadQueue
{
    QueueHandle_t queue;

public:

    ThreadQueue(UBaseType_t max_size) {
        queue = xQueueCreate(max_size, sizeof(TaskHandle_t));
    }

    ~ThreadQueue() {
        vQueueDelete(queue);
    }


    void enqueue(TicksType_t ticks=portMAX_DELAY) {
        TaskHandle_t current = xTaskGetCurrentTaskHandle();
        xQueueSend(queue, &current, ticks);
    }

    void sleep() {
        enqueue();
        vTaskSuspend(NULL);
    }

    bool wake() {
        TaskHandle_t next;
        if (xQueueReceive(queue, &next, 0)==pdTRUE) {
            vTaskResume(next);
            return true;
        }
        return false;
    }

    void wakeAll()
    {
        while (wake()) {}
    }
};


class Lock
{
    SemaphoreHandle_t semaphore;

public:

    Lock() {
        semaphore = xSemaphoreCreateMutex();
    }

    void acquire() {
        while (xSemaphoreTake(semaphore, portMAX_DELAY)==pdFALSE);
    }

    void release() {
        xSemaphoreGive(semaphore);
    }

    ~Lock() {
        vSemaphoreDelete(semaphore);
    }
};


class ConditionVariable
{
    ThreadQueue queue;

    typedef std::unique_lock<std::mutex> lock_t;
public:
    ConditionVariable(UBaseType_t max_size) : queue(max_size) {}

    void wait(lock_t* lock)
    {
        taskENTER_CRITICAL();
        lock->unlock();
        queue.enqueue();
        taskEXIT_CRITICAL();
        vTaskSuspend(NULL);
        lock->lock();
    }

    void signal()
    {
        taskENTER_CRITICAL();
        queue.wake();
        taskEXIT_CRITICAL();
    }

    void broadcast()
    {
        taskENTER_CRITICAL();
        queue.wakeAll();
        taskEXIT_CRITICAL();
    }
};

static_assert(sizeof(__gthread_cond_t)==sizeof(ConditionVariable), "__gthread_cond_t must be the same size as ConditionVariable");


bool __gthread_equal(__gthread_t t1, __gthread_t t2)
{
    return t1==t2;
}

__gthread_t __gthread_self()
{
    return xTaskGetCurrentTaskHandle();
}

void os_condition_variable_create(condition_variable_t* cond)
{
    new(cond) ConditionVariable(10);
}

void os_condition_variable_dispose(condition_variable_t* cond)
{
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->~ConditionVariable();
}

void os_condition_variable_wait(condition_variable_t* cond, void* v)
{
    std::unique_lock<std::mutex>* lock = (std::unique_lock<std::mutex>*)v;
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->wait(lock);
}

void os_condition_variable_notify_one(condition_variable_t* cond)
{
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->signal();
}


