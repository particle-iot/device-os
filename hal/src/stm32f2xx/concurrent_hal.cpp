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

#include "core_hal_stm32f2xx.h"
#include "static_assert.h"
#include "delay_hal.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "stm32f2xx.h"
#include "interrupts_hal.h"
#include <mutex>
#include <atomic>
#include "flash_acquire.h"
#include "periph_lock.h"
#include "core_hal.h"
#include "logging.h"
#include "atomic_flag_mutex.h"
#include "static_recursive_mutex.h"
#include "service_debug.h"

#if PLATFORM_ID == 6 || PLATFORM_ID == 8
# include "wwd_rtos_interface.h"
#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8

// For OpenOCD FreeRTOS support
extern const int  __attribute__((used)) uxTopUsedPriority = configMAX_PRIORITIES;

// use the newer name
typedef xTaskHandle TaskHandle_t;
typedef xQueueHandle QueueHandle_t;
typedef xSemaphoreHandle SemaphoreHandle_t;
typedef portTickType TicksType_t;

typedef unsigned portBASE_TYPE UBaseType_t;

static_assert(sizeof(TaskHandle_t)==sizeof(__gthread_t), "__gthread_t should be the same size as TaskHandle_t");

static_assert(sizeof(uint32_t)==sizeof(void*), "Requires uint32_t to be same size as void* for function cast to wiced_thread_function_t");

#if defined(configENABLE_BACKWARD_COMPATIBILITY) && configENABLE_BACKWARD_COMPATIBILITY
#define _CREATE_NAME_TYPE const char
#else
#define _CREATE_NAME_TYPE const signed char
#endif

namespace {

StaticRecursiveMutex g_periphMutex;

} // namespace

/**
 * Creates a new thread.
 * @param thread            Receives the created thread handle. Will be set to NULL if the thread cannot be created.
 * @param name              The name of the thread. May be used if the underlying RTOS supports it. Can be null.
 * @param priority          The thread priority. It's best to stick to a small range of priorities, e.g. +/- 7.
 * @param fun               The function to execute in a separate thread.
 * @param thread_param      The parameter to pass to the thread function.
 * @param stack_size        The size of the stack to create. The stack is allocated on the heap.
 * @return an error code. 0 if the thread was successfully created.
 */
os_result_t os_thread_create(os_thread_t* thread, const char* name, os_thread_prio_t priority, os_thread_fn_t fun, void* thread_param, size_t stack_size)
{
    *thread = NULL;
    if(priority >= configMAX_PRIORITIES) {
      priority = configMAX_PRIORITIES - 1;
    }
    signed portBASE_TYPE result = xTaskCreate( (pdTASK_CODE)fun, (_CREATE_NAME_TYPE* const) name, (stack_size/sizeof(portSTACK_TYPE)), thread_param, (unsigned portBASE_TYPE) priority, thread);
    return ( result != (signed portBASE_TYPE) pdPASS );
}


/**
 * Determines if the given thread is the one executing.
 * @param   The thread to test.
 * @return {@code true} if the thread given is the one currently executing. {@code false} otherwise.
 */
bool os_thread_is_current(os_thread_t thread)
{
    return thread==xTaskGetCurrentTaskHandle();
}


os_result_t os_thread_yield(void)
{
    taskYIELD();
    return 0;
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
    return true;
}

/**
 * Waits indefinitely for the given thread to finish.
 * @param thread    The thread to wait for.
 * @return 0 if the thread has successfully terminated. non-zero if the thread handle is not valid.
 */
os_result_t os_thread_join(os_thread_t thread)
{
#if PLATFORM_ID!=10
    while (xTaskIsTaskFinished(thread) != pdTRUE)
    {
        HAL_Delay_Milliseconds(10);
    }
    return 0;
#else
    while (eTaskGetState(thread) != eDeleted)
    {
        HAL_Delay_Milliseconds(10);
    }
    return 0;
#endif
}

/**
 * Terminate thread.
 * @param thread    The thread to terminate, or NULL to terminate current thread.
 * @return 0 if the thread has successfully terminated. non-zero in case of an error.
 */
os_result_t os_thread_exit(os_thread_t thread)
{
    vTaskDelete(thread);
    return 0;
}

/**
 * Cleans up resources used by a terminated thread.
 * @param thread    The thread to clean up.
 * @return 0 on success.
 */
os_result_t os_thread_cleanup(os_thread_t thread)
{
#if PLATFORM_ID == 6 || PLATFORM_ID == 8
    if (!thread || os_thread_is_current(thread))
        return 1;
    host_rtos_delete_terminated_thread((host_thread_type_t*)&thread);
#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8
    return 0;
}

/**
 * Delays the current task until a specified time to set up periodic tasks
 * @param previousWakeTime The time the thread last woke up.  May not be NULL.
 *                         Set to the current time on first call. Will be updated
 *                         when the task wakes up
 * @param timeIncrement    The cycle time period
 * @return 0 on success. 1 if previousWakeTime is NULL
 */
os_result_t os_thread_delay_until(system_tick_t *previousWakeTime, system_tick_t timeIncrement)
{
    if(previousWakeTime == NULL) {
        return 1;
    }
    vTaskDelayUntil(previousWakeTime, timeIncrement);
    return 0;
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

int os_condition_variable_create(condition_variable_t* cond)
{
    return (*cond = new ConditionVariable(10))==NULL;
}

void os_condition_variable_destroy(condition_variable_t cond)
{
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->~ConditionVariable();
}

void os_condition_variable_wait(condition_variable_t cond, void* v)
{
    std::unique_lock<std::mutex>* lock = (std::unique_lock<std::mutex>*)v;
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->wait(lock);
}

void os_condition_variable_notify_one(condition_variable_t cond)
{
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->signal();
}

void os_condition_variable_notify_all(condition_variable_t cond)
{
    ConditionVariable* cv = (ConditionVariable*)cond;
    cv->broadcast();
}


int os_queue_create(os_queue_t* queue, size_t item_size, size_t item_count, void*)
{
    *queue = xQueueCreate(item_count, item_size);
    return *queue==NULL;
}

static_assert(portMAX_DELAY==CONCURRENT_WAIT_FOREVER, "expected portMAX_DELAY==CONCURRENT_WAIT_FOREVER");

int os_queue_put(os_queue_t queue, const void* item, system_tick_t delay, void*)
{
    if (HAL_IsISR()) {
        BaseType_t woken = pdFALSE;
        int res = xQueueSendFromISR(queue, item, &woken) != pdTRUE;
        portYIELD_FROM_ISR(woken);
        return res;
    } else {
        return xQueueSend(queue, item, delay)!=pdTRUE;
    }
}

int os_queue_take(os_queue_t queue, void* item, system_tick_t delay, void*)
{
    return xQueueReceive(queue, item, delay)!=pdTRUE;
}

int os_queue_destroy(os_queue_t queue, void*)
{
    vQueueDelete(queue);
    return 0;
}


int os_mutex_create(os_mutex_t* mutex)
{
    return (*mutex = xSemaphoreCreateMutex())==NULL;
}

int os_mutex_destroy(os_mutex_t mutex)
{
    vSemaphoreDelete(mutex);
    return 0;
}

int os_mutex_lock(os_mutex_t mutex)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    return 0;
}

int os_mutex_trylock(os_mutex_t mutex)
{
    return xSemaphoreTake(mutex, 0)==pdFALSE;
}


int os_mutex_unlock(os_mutex_t mutex)
{
    xSemaphoreGive(mutex);
    return 0;
}

int os_mutex_recursive_create(os_mutex_recursive_t* mutex)
{
    return (*mutex = xSemaphoreCreateRecursiveMutex())==NULL;
}

int os_mutex_recursive_destroy(os_mutex_recursive_t mutex)
{
    vSemaphoreDelete(mutex);
    return 0;
}

int os_mutex_recursive_lock(os_mutex_recursive_t mutex)
{
    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
    return 0;
}

int os_mutex_recursive_trylock(os_mutex_recursive_t mutex)
{
    return (xSemaphoreTakeRecursive(mutex, 0)!=pdTRUE);
}

int os_mutex_recursive_unlock(os_mutex_recursive_t mutex)
{
    return xSemaphoreGiveRecursive(mutex)!=pdTRUE;
}

void os_thread_scheduling(bool enabled, void* reserved)
{
    if (enabled)
        xTaskResumeAll();
    else
        vTaskSuspendAll();
}

int os_semaphore_create(os_semaphore_t* semaphore, unsigned max, unsigned initial)
{
    *semaphore = xSemaphoreCreateCounting( ( max ), ( initial ) );
    return *semaphore==NULL;
}

int os_semaphore_destroy(os_semaphore_t semaphore)
{
    vSemaphoreDelete(semaphore);
    return 0;
}

int os_semaphore_take(os_semaphore_t semaphore, system_tick_t timeout, bool reserved)
{
    return (xSemaphoreTake(semaphore, timeout)!=pdTRUE);
}

int os_semaphore_give(os_semaphore_t semaphore, bool reserved)
{
    return xSemaphoreGive(semaphore)!=pdTRUE;
}

/**
 * Create a new timer. Returns 0 on success.
 */
int os_timer_create(os_timer_t* timer, unsigned period, void (*callback)(os_timer_t timer), void* const timer_id, bool one_shot, void* reserved)
{
    *timer = xTimerCreate((_CREATE_NAME_TYPE*)"", period, !one_shot, timer_id, callback);
    return *timer==NULL;
}

int os_timer_get_id(os_timer_t timer, void** timer_id)
{
    *timer_id = pvTimerGetTimerID(timer);
    return 0;
}

int os_timer_set_id(os_timer_t timer, void* timer_id)
{
    vTimerSetTimerID(timer, timer_id);
    return 0;
}

int os_timer_change(os_timer_t timer, os_timer_change_t change, bool fromISR, unsigned period, unsigned block, void* reserved)
{
    portBASE_TYPE woken;
    switch (change)
    {
    case OS_TIMER_CHANGE_START:
        if (fromISR)
            return xTimerStartFromISR(timer, &woken)!=pdPASS;
        else
            return xTimerStart(timer, block)!=pdPASS;

    case OS_TIMER_CHANGE_RESET:
        if (fromISR)
            return xTimerResetFromISR(timer, &woken)!=pdPASS;
        else
            return xTimerReset(timer, block)!=pdPASS;

    case OS_TIMER_CHANGE_STOP:
        if (fromISR)
            return xTimerStopFromISR(timer, &woken)!=pdPASS;
        else
            return xTimerStop(timer, block)!=pdPASS;

    case OS_TIMER_CHANGE_PERIOD:
        if (fromISR)
            return xTimerChangePeriodFromISR(timer, period, &woken)!=pdPASS;
        else
            return xTimerChangePeriod(timer, period, block)!=pdPASS;
    }
    return -1;
}

int os_timer_destroy(os_timer_t timer, void* reserved)
{
    return xTimerDelete(timer, CONCURRENT_WAIT_FOREVER)!=pdPASS;
}

int os_timer_is_active(os_timer_t timer, void* reserved)
{
    return xTimerIsTimerActive(timer) != pdFALSE;
}

static AtomicFlagMutex<os_result_t, os_thread_yield> flash_lock;
void __flash_acquire() {
    if (!rtos_started) {
        return;
    }
    if (HAL_IsISR()) {
        PANIC(UsageFault, "Flash operation from IRQ");
    }
    flash_lock.lock();
}

void __flash_release() {
    if (!rtos_started) {
        return;
    }
    flash_lock.unlock();
}

void periph_lock() {
    if (!rtos_started) {
        return;
    }
    g_periphMutex.lock();
}

void periph_unlock() {
    if (!rtos_started) {
        return;
    }
    g_periphMutex.unlock();
}
