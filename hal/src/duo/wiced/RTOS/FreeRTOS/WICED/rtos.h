/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "wiced_result.h"
#include "wwd_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_HARDWARE_IO_WORKER_THREAD             ((wiced_worker_thread_t*)&wiced_hardware_io_worker_thread)
#define WICED_NETWORKING_WORKER_THREAD              ((wiced_worker_thread_t*)&wiced_networking_worker_thread )
#define LOWER_THAN_PRIORITY_OF( thread )            ((thread).handle.tx_thread_priority - 1)
#define HIGHER_THAN_PRIORITY_OF( thread )           ((thread).handle.tx_thread_priority + 1)

#define WICED_PRIORITY_TO_NATIVE_PRIORITY(priority) (uint8_t)(RTOS_HIGHEST_PRIORITY - priority)

#define WICED_END_OF_THREAD( thread )               malloc_leak_check(thread, LEAK_CHECK_THREAD); vTaskDelete(thread)
#define WICED_END_OF_CURRENT_THREAD( )              malloc_leak_check(NULL, LEAK_CHECK_THREAD); vTaskDelete(NULL)
#define WICED_END_OF_CURRENT_THREAD_NO_LEAK_CHECK( )   vTaskDelete(NULL)

#define WICED_TO_MALLOC_THREAD( x )                 ((malloc_thread_handle) *(x) )

#define WICED_GET_THREAD_HANDLE( thread )           ( thread )

#define WICED_GET_QUEUE_HANDLE( queue )             ( queue )

/******************************************************
 *                    Constants
 ******************************************************/

/* Configuration of Built-in Worker Threads
 *
 * 1. wiced_hardware_io_worker_thread is designed to handle deferred execution of quick, non-blocking hardware I/O operations.
 *    - priority         : higher than that of wiced_networking_worker_thread
 *    - stack size       : small. Consequently, no printf is allowed here.
 *    - event queue size : the events are quick; therefore, large queue isn't required.
 *
 * 2. wiced_networking_worker_thread is designed to handle deferred execution of networking operations
 *    - priority         : lower to allow wiced_hardware_io_worker_thread to preempt and run
 *    - stack size       : considerably larger than that of wiced_hardware_io_worker_thread because of the networking functions.
 *    - event queue size : larger than that of wiced_hardware_io_worker_thread because networking operation may block
 */

#ifndef HARDWARE_IO_WORKER_THREAD_STACK_SIZE
#define HARDWARE_IO_WORKER_THREAD_STACK_SIZE                                   (512)
#endif
#define HARDWARE_IO_WORKER_THREAD_QUEUE_SIZE                                    (10)
#define HARDWARE_IO_WORKER_THREAD_PRIORITY       (WICED_PRIORITY_TO_NATIVE_PRIORITY(WICED_DEFAULT_LIBRARY_PRIORITY))

#ifndef NETWORKING_WORKER_THREAD_STACK_SIZE
#define NETWORKING_WORKER_THREAD_STACK_SIZE                               (7 * 1024)
#endif
#define NETWORKING_WORKER_THREAD_QUEUE_SIZE                                     (15)
#define NETWORKING_WORKER_THREAD_PRIORITY        (WICED_PRIORITY_TO_NATIVE_PRIORITY(WICED_NETWORK_WORKER_PRIORITY))

#define RTOS_NAME                     "FreeRTOS"
#define RTOS_VERSION                  FreeRTOS_VERSION


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef uint32_t  wiced_event_flags_t;
typedef host_semaphore_type_t wiced_semaphore_t;

typedef SemaphoreHandle_t wiced_mutex_t;

typedef void (*timer_handler_t)( void* arg );

typedef struct
{
    TimerHandle_t    handle;
    timer_handler_t function;
    void*           arg;
}wiced_timer_t;

typedef TaskHandle_t wiced_thread_t;

typedef QueueHandle_t wiced_queue_t;

typedef struct
{
    wiced_thread_t thread;
    wiced_queue_t  event_queue;
} wiced_worker_thread_t;

typedef wiced_result_t (*event_handler_t)( void* arg );

typedef struct
{
    event_handler_t        function;
    void*                  arg;
    wiced_timer_t          timer;
    wiced_worker_thread_t* thread;
} wiced_timed_event_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern wiced_worker_thread_t wiced_hardware_io_worker_thread;
extern wiced_worker_thread_t wiced_networking_worker_thread;

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
