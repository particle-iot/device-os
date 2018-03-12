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

#include "tx_api.h"
#include "wiced_result.h"
#include "wiced_utilities.h"
#include "wwd_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_HARDWARE_IO_WORKER_THREAD             ((wiced_worker_thread_t*)&wiced_hardware_io_worker_thread)
#define WICED_NETWORKING_WORKER_THREAD              ((wiced_worker_thread_t*)&wiced_networking_worker_thread )
#define LOWER_THAN_PRIORITY_OF( thread )            ((thread).handle.tx_thread_priority + 1)
#define HIGHER_THAN_PRIORITY_OF( thread )           ((thread).handle.tx_thread_priority - 1)

#define WICED_PRIORITY_TO_NATIVE_PRIORITY(priority) ( priority )
#define WICED_END_OF_THREAD(thread)                 malloc_leak_check( &(thread).handle, LEAK_CHECK_THREAD); (void)(thread)
#define WICED_END_OF_CURRENT_THREAD( )              malloc_leak_check( NULL, LEAK_CHECK_THREAD)
#define WICED_END_OF_CURRENT_THREAD_NO_LEAK_CHECK( )

#define WICED_TO_MALLOC_THREAD( x )                 ((malloc_thread_handle) &((x)->handle ))

#define WICED_GET_THREAD_HANDLE( thread )           (&(( thread )->handle ))

#define WICED_GET_QUEUE_HANDLE( queue )             (&(( queue )->handle ))

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
#ifdef DEBUG
#define HARDWARE_IO_WORKER_THREAD_STACK_SIZE     (768) /* debug builds can use larger stack for example because of compiled-in asserts, switched off optimisation, etc */
#else
#define HARDWARE_IO_WORKER_THREAD_STACK_SIZE     (512)
#endif
#endif
#define HARDWARE_IO_WORKER_THREAD_QUEUE_SIZE      (10)

#ifndef NETWORKING_WORKER_THREAD_STACK_SIZE
#define NETWORKING_WORKER_THREAD_STACK_SIZE   (6*1024)
#endif
#define NETWORKING_WORKER_THREAD_QUEUE_SIZE       (15)

#define RTOS_NAME                     "ThreadX"
#define RTOS_VERSION                  ThreadX_VERSION

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef TX_EVENT_FLAGS_GROUP wiced_event_flags_t;

typedef host_semaphore_type_t wiced_semaphore_t;

typedef TX_MUTEX wiced_mutex_t;

typedef void (*timer_handler_t)( void* arg );

typedef TX_TIMER wiced_timer_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t last_update;
    uint32_t longest_delay;
} thread_monitor_info_t;

typedef struct
{
    TX_THREAD handle;
    void*     stack;
} wiced_thread_t;

typedef struct
{
    TX_QUEUE handle;
    void*    buffer;
} wiced_queue_t;

typedef struct
{
    wiced_thread_t        thread;
    wiced_queue_t         event_queue;
    thread_monitor_info_t monitor_info;
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
