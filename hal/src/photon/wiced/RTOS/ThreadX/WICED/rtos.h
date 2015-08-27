/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
