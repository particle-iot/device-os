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

#define NO_STATIC_ASSERT
#include "static_assert.h"
#include "delay_hal.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "interrupts_hal.h"
#include <mutex>
#include <atomic>
#include "flash_acquire.h"
#include "periph_lock.h"
#include "core_hal.h"
#include "logging.h"
#include "static_recursive_mutex.h"
#include "service_debug.h"

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

typedef struct {
    os_thread_dump_callback_t callback;
    os_thread_t thread;
    void* data;
 } os_thread_dump_helper_t;

namespace {

StaticRecursiveMutex g_periphMutex;
StaticRecursiveMutex g_flashMutex;

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
    signed portBASE_TYPE result = xTaskCreate( (pdTASK_CODE)fun, (_CREATE_NAME_TYPE*) name,
            (stack_size/sizeof(portSTACK_TYPE)), thread_param, (unsigned portBASE_TYPE) priority,
            reinterpret_cast<TaskHandle_t*>(thread) );
    return ( result != (signed portBASE_TYPE) pdPASS );
}

os_thread_t os_thread_current(void* reserved)
{
    return xTaskGetCurrentTaskHandle();
}

/**
 * Determines if the given thread is the one executing.
 * @param   The thread to test.
 * @return {@code true} if the thread given is the one currently executing. {@code false} otherwise.
 */
bool os_thread_is_current(os_thread_t thread)
{
    return static_cast<TaskHandle_t>(thread) == xTaskGetCurrentTaskHandle();
}


os_result_t os_thread_yield(void)
{
    taskYIELD();
    return 0;
}

static BaseType_t os_thread_dump_helper(TaskStatus_t* const status, void* data)
{
    os_thread_dump_helper_t* h = (os_thread_dump_helper_t*)data;
    os_thread_dump_info_t info;
    memset( &info, 0, sizeof(os_thread_dump_info_t) );
    info.thread = status->xHandle;
    info.name = status->pcTaskName;
    info.id = status->xTaskNumber;
    info.stack_high_watermark = status->usStackHighWaterMark;
    info.priority = status->uxCurrentPriority;
    info.base_priority = status->uxBasePriority;

    void *stack_end = NULL;
    vTaskGetStackInfoParticle(status->xHandle, &info.stack_base, &info.stack_current, &stack_end);

    //calculate the stack size
    info.stack_size = (size_t)stack_end - (size_t)info.stack_base;

    if (h->callback && (h->thread == OS_THREAD_INVALID_HANDLE || h->thread == status->xHandle)) {
        return (BaseType_t)h->callback(&info, h->data);
    }

    return 0;
}

os_result_t os_thread_dump(os_thread_t thread, os_thread_dump_callback_t callback, void* ptr)
{
    // NOTE: callback is executed with thread scheduling disabled
    // returning anything other than 0 in the callback will prevent further callback invocations,
    // stopping iteration over the threads.
    TaskStatus_t status;
    memset( &status, 0, sizeof(TaskStatus_t) );
    os_thread_dump_helper_t data = {callback, thread, ptr};
    uxTaskGetSystemStateParticle(&status, 1, nullptr, os_thread_dump_helper, (void*)&data);

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
    while (eTaskGetState(static_cast<TaskHandle_t>(thread)) != eDeleted)
    {
        HAL_Delay_Milliseconds(10);
    }
    return 0;
}

/**
 * Terminate thread.
 * @param thread    The thread to terminate, or NULL to terminate current thread.
 * @return 0 if the thread has successfully terminated. non-zero in case of an error.
 */
os_result_t os_thread_exit(os_thread_t thread)
{
    vTaskDelete(static_cast<TaskHandle_t>(thread));
    return 0;
}

/**
 * Cleans up resources used by a terminated thread.
 * @param thread    The thread to clean up.
 * @return 0 on success.
 */
os_result_t os_thread_cleanup(os_thread_t thread)
{
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

os_thread_notify_t os_thread_wait(system_tick_t ms, void* reserved)
{
    return ulTaskNotifyTake(pdTRUE, ms);
}

int os_thread_notify(os_thread_t thread, void* reserved)
{
    if (!hal_interrupt_is_isr()) {
        return xTaskNotifyGive(static_cast<TaskHandle_t>(thread)) != pdTRUE;
    } else {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(static_cast<TaskHandle_t>(thread), &woken);
        portYIELD_FROM_ISR(woken);
        return 0;
    }
}

bool __gthread_equal(__gthread_t t1, __gthread_t t2)
{
    return t1==t2;
}

__gthread_t __gthread_self()
{
    return xTaskGetCurrentTaskHandle();
}

int os_queue_create(os_queue_t* queue, size_t item_size, size_t item_count, void*)
{
    *queue = xQueueCreate(item_count, item_size);
    return *queue==NULL;
}

static_assert(portMAX_DELAY==CONCURRENT_WAIT_FOREVER, "expected portMAX_DELAY==CONCURRENT_WAIT_FOREVER");

int os_queue_put(os_queue_t queue, const void* item, system_tick_t delay, void*)
{
    if (!hal_interrupt_is_isr()) {
        return xQueueSend(static_cast<QueueHandle_t>(queue), item, delay)!=pdTRUE;
    } else {
        BaseType_t woken = pdFALSE;
        int res = xQueueSendFromISR(static_cast<QueueHandle_t>(queue), item, &woken) != pdTRUE;
        portYIELD_FROM_ISR(woken);
        return res;
    }
}

int os_queue_take(os_queue_t queue, void* item, system_tick_t delay, void*)
{
    if (!hal_interrupt_is_isr()) {
        return xQueueReceive(static_cast<QueueHandle_t>(queue), item, delay)!=pdTRUE;
    } else {
        BaseType_t woken = pdFALSE;
        int res = xQueueReceiveFromISR(static_cast<QueueHandle_t>(queue), item, &woken) != pdTRUE;
        portYIELD_FROM_ISR(woken);
        return res;
    }
}

int os_queue_peek(os_queue_t queue, void* item, system_tick_t delay, void*)
{
    if (!hal_interrupt_is_isr()) {
        return xQueuePeek(static_cast<QueueHandle_t>(queue), item, delay)!=pdTRUE;
    } else {
        // Delay is ignored
        return xQueuePeekFromISR(static_cast<QueueHandle_t>(queue), item)!=pdTRUE;
    }
}

int os_queue_destroy(os_queue_t queue, void*)
{
    vQueueDelete(static_cast<QueueHandle_t>(queue));
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
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(mutex), portMAX_DELAY);
    return 0;
}

int os_mutex_trylock(os_mutex_t mutex)
{
    return xSemaphoreTake(static_cast<SemaphoreHandle_t>(mutex), 0)==pdFALSE;
}


int os_mutex_unlock(os_mutex_t mutex)
{
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(mutex));
    return 0;
}

int os_mutex_recursive_create(os_mutex_recursive_t* mutex)
{
    return (*mutex = xSemaphoreCreateRecursiveMutex())==NULL;
}

int os_mutex_recursive_destroy(os_mutex_recursive_t mutex)
{
    vSemaphoreDelete(static_cast<SemaphoreHandle_t>(mutex));
    return 0;
}

int os_mutex_recursive_lock(os_mutex_recursive_t mutex)
{
    xSemaphoreTakeRecursive(static_cast<SemaphoreHandle_t>(mutex), portMAX_DELAY);
    return 0;
}

int os_mutex_recursive_trylock(os_mutex_recursive_t mutex)
{
    return (xSemaphoreTakeRecursive(static_cast<SemaphoreHandle_t>(mutex), 0)!=pdTRUE);
}

int os_mutex_recursive_unlock(os_mutex_recursive_t mutex)
{
    return xSemaphoreGiveRecursive(static_cast<SemaphoreHandle_t>(mutex))!=pdTRUE;
}

void os_thread_scheduling(bool enabled, void* reserved)
{
    if (enabled)
        xTaskResumeAll();
    else
        vTaskSuspendAll();
}

os_scheduler_state_t os_scheduler_get_state(void* reserved)
{
    return (os_scheduler_state_t)xTaskGetSchedulerState();
}

int os_semaphore_create(os_semaphore_t* semaphore, unsigned max, unsigned initial)
{
    *semaphore = xSemaphoreCreateCounting( ( max ), ( initial ) );
    return *semaphore==NULL;
}

int os_semaphore_destroy(os_semaphore_t semaphore)
{
    vSemaphoreDelete(static_cast<SemaphoreHandle_t>(semaphore));
    return 0;
}

int os_semaphore_take(os_semaphore_t semaphore, system_tick_t timeout, bool reserved)
{
    if (!hal_interrupt_is_isr()) {
        return (xSemaphoreTake(static_cast<SemaphoreHandle_t>(semaphore), timeout)!=pdTRUE);
    } else {
        BaseType_t woken = pdFALSE;
        int res = xSemaphoreTakeFromISR(static_cast<SemaphoreHandle_t>(semaphore), &woken) != pdTRUE;
        portYIELD_FROM_ISR(woken);
        return res;
    }
}

int os_semaphore_give(os_semaphore_t semaphore, bool reserved)
{
    if (!hal_interrupt_is_isr()) {
        return xSemaphoreGive(static_cast<SemaphoreHandle_t>(semaphore))!=pdTRUE;
    } else {
        BaseType_t woken = pdFALSE;
        int res = xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(semaphore), &woken) != pdTRUE;
        portYIELD_FROM_ISR(woken);
        return res;
    }
}

/**
 * Create a new timer. Returns 0 on success.
 */
int os_timer_create(os_timer_t* timer, unsigned period, void (*callback)(os_timer_t timer), void* const timer_id, bool one_shot, void* reserved)
{
    *timer = xTimerCreate((_CREATE_NAME_TYPE*)"", period, !one_shot, timer_id, reinterpret_cast<TimerCallbackFunction_t>(callback));
    return *timer==NULL;
}

int os_timer_get_id(os_timer_t timer, void** timer_id)
{
    *timer_id = pvTimerGetTimerID(static_cast<TimerHandle_t>(timer));
    return 0;
}

int os_timer_set_id(os_timer_t timer, void* timer_id)
{
    vTimerSetTimerID(static_cast<TimerHandle_t>(timer), timer_id);
    return 0;
}

int os_timer_change(os_timer_t timer, os_timer_change_t change, bool fromISR, unsigned period, unsigned block, void* reserved)
{
    portBASE_TYPE woken;
    switch (change)
    {
    case OS_TIMER_CHANGE_START:
        if (fromISR)
            return xTimerStartFromISR(static_cast<TimerHandle_t>(timer), &woken)!=pdPASS;
        else
            return xTimerStart(static_cast<TimerHandle_t>(timer), block)!=pdPASS;

    case OS_TIMER_CHANGE_RESET:
        if (fromISR)
            return xTimerResetFromISR(static_cast<TimerHandle_t>(timer), &woken)!=pdPASS;
        else
            return xTimerReset(static_cast<TimerHandle_t>(timer), block)!=pdPASS;

    case OS_TIMER_CHANGE_STOP:
        if (fromISR)
            return xTimerStopFromISR(static_cast<TimerHandle_t>(timer), &woken)!=pdPASS;
        else
            return xTimerStop(static_cast<TimerHandle_t>(timer), block)!=pdPASS;

    case OS_TIMER_CHANGE_PERIOD:
        if (fromISR)
            return xTimerChangePeriodFromISR(static_cast<TimerHandle_t>(timer), period, &woken)!=pdPASS;
        else
            return xTimerChangePeriod(static_cast<TimerHandle_t>(timer), period, block)!=pdPASS;
    }
    return -1;
}

int os_timer_destroy(os_timer_t timer, void* reserved)
{
    return xTimerDelete(static_cast<TimerHandle_t>(timer), CONCURRENT_WAIT_FOREVER)!=pdPASS;
}

int os_timer_is_active(os_timer_t timer, void* reserved)
{
    return xTimerIsTimerActive(static_cast<TimerHandle_t>(timer)) != pdFALSE;
}

void __flash_acquire() {
    if (hal_interrupt_is_isr()) {
        PANIC(UsageFault, "Flash operation from IRQ");
    }
    g_flashMutex.lock();
}

void __flash_release() {
    g_flashMutex.unlock();
}

void periph_lock() {
    g_periphMutex.lock();
}

void periph_unlock() {
    g_periphMutex.unlock();
}
