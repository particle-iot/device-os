/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines functions to access functions provided by the RTOS
 *  in a generic way
 *
 */

#pragma once


/* Include the actual RTOS definitions for:
 * - wiced_timed_event_t
 * - timed_event_handler_t
 */
#include "rtos.h"
#include "wiced_result.h"
#include "RTOS/wwd_rtos_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond            Enumerations
 ******************************************************/

typedef enum
{
    WAIT_FOR_ANY_EVENT,
    WAIT_FOR_ALL_EVENTS,
} wiced_event_flags_wait_option_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef wwd_thread_arg_t wiced_thread_arg_t;

typedef void (*wiced_thread_function_t)( wiced_thread_arg_t arg );

/******************************************************
 *             Function declarations
 * @endcond
 ******************************************************/

/*****************************************************************************/
/** @defgroup rtos       RTOS
 *
 *  WICED Real-Time Operating System Functions
 */
/*****************************************************************************/


/*****************************************************************************/
/** @addtogroup threads       Threads
 *  @ingroup rtos
 *
 * Thread management functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Creates and starts a new thread
 *
 * Creates and starts a new thread
 *
 * @param thread     : Pointer to variable that will receive the thread handle
 * @param priority   : A priority number or WICED_DEFAULT_APP_THREAD_PRIORITY.
 * @param name       : a text name for the thread (can be null)
 * @param function   : the main thread function
 * @param stack_size : stack size for this thread
 * @param arg        : argument which will be passed to thread function
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_create_thread( wiced_thread_t* thread, uint8_t priority, const char* name, wiced_thread_function_t function, uint32_t stack_size, void* arg );


/** Creates and starts a new thread with user provided stack
 *
 * Creates and starts a new thread with user provided stack
 *
 * @param thread     : Pointer to variable that will receive the thread handle
 * @param priority   : A priority number or WICED_DEFAULT_APP_THREAD_PRIORITY.
 * @param name       : a text name for the thread (can be null)
 * @param function   : the main thread function
 * @param stack      : the stack for this thread
 * @param stack_size : stack size for this thread
 * @param arg        : argument which will be passed to thread function
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_create_thread_with_stack( wiced_thread_t* thread, uint8_t priority, const char* name, wiced_thread_function_t function, void* stack, uint32_t stack_size, void* arg );


/** Deletes a terminated thread
 *
 * @param thread     : the handle of the thread to delete
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_delete_thread( wiced_thread_t* thread );


/** Sleep for a given period of milliseconds
 *
 * Causes the current thread to sleep for AT LEAST the
 * specified number of milliseconds. If the processor is heavily loaded
 * with higher priority tasks, the delay may be much longer than requested.
 *
 * @param milliseconds : the time to sleep in milliseconds
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_delay_milliseconds( uint32_t milliseconds );

/** Delay for a given period of microseconds
 *
 * Causes the current thread to block for AT LEAST the
 * specified number of microseconds. If the processor is heavily loaded
 * with higher priority tasks, the delay may be much longer than requested.
 *
 * NOTE: All threads with equal or lower priority than the current thread
 *       will not be able to run while the delay is occurring.
 *
 * @param microseconds : the time to delay in microseconds
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_delay_microseconds( uint32_t microseconds );

/** Sleeps until another thread has terminated
 *
 * Causes the current thread to sleep until the specified other thread
 * has terminated. If the processor is heavily loaded
 * with higher priority tasks, this thread may not wake until significantly
 * after the thread termination.
 *
 * @param thread : the handle of the other thread which will terminate
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_thread_join( wiced_thread_t* thread );


/** Forcibly wakes another thread
 *
 * Causes the specified thread to wake from suspension. This will usually
 * cause an error or timeout in that thread, since the task it was waiting on
 * is not complete.
 *
 * @param thread : the handle of the other thread which will be woken
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_thread_force_awake( wiced_thread_t* thread );


/** Checks if a thread is the current thread
 *
 * Checks if a specified thread is the currently running thread
 *
 * @param thread : the handle of the other thread against which the current thread will be compared
 *
 * @return    WICED_SUCCESS : specified thread is the current thread
 * @return    WICED_ERROR   : specified thread is not currently running
 */
wiced_result_t wiced_rtos_is_current_thread( wiced_thread_t* thread );


/** Checks the stack of the current thread
 *
 * @return    WICED_SUCCESS : if the current thread stack is within limits
 * @return    WICED_ERROR   : if the current thread stack has extended beyond its limits
 */
wiced_result_t wiced_rtos_check_stack( void );

/** @} */
/*****************************************************************************/
/** @addtogroup semaphores       Semaphores
 *  @ingroup rtos
 *
 * Semaphore management functionss
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a semaphore
 *
 * Initialises a counting semaphore
 *
 * @param semaphore : a pointer to the semaphore handle to be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_init_semaphore( wiced_semaphore_t* semaphore );


/** Set (post/put/increment) a semaphore
 *
 * Set (post/put/increment) a semaphore
 *
 * @param semaphore : a pointer to the semaphore handle to be set
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_set_semaphore( wiced_semaphore_t* semaphore );


/** Get (wait/decrement) a semaphore
 *
 * Attempts to get (wait/decrement) a semaphore. If semaphore is at zero already,
 * then the calling thread will be suspended until another thread sets the
 * semaphore with @ref wiced_rtos_set_semaphore
 *
 * @param semaphore : a pointer to the semaphore handle
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_get_semaphore( wiced_semaphore_t* semaphore, uint32_t timeout_ms );


/** De-initialise a semaphore
 *
 * Deletes a semaphore created with @ref wiced_rtos_init_semaphore
 *
 * @param semaphore : a pointer to the semaphore handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deinit_semaphore( wiced_semaphore_t* semaphore );

/** @} */
/*****************************************************************************/
/** @addtogroup mutexes       Mutexes
 *  @ingroup rtos
 *
 * Mutex management functionss
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a mutex
 *
 * Initialises a mutex
 * A mutex is different to a semaphore in that a thread that already holds
 * the lock on the mutex can request the lock again (nested) without causing
 * it to be suspended.
 *
 * @param mutex : a pointer to the mutex handle to be initialised
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_init_mutex( wiced_mutex_t* mutex );


/** Obtains the lock on a mutex
 *
 * Attempts to obtain the lock on a mutex. If the lock is already held
 * by another thead, the calling thread will be suspended until
 * the mutex lock is released by the other thread.
 *
 * @param mutex : a pointer to the mutex handle to be locked
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_lock_mutex( wiced_mutex_t* mutex );


/** Releases the lock on a mutex
 *
 * Releases a currently held lock on a mutex. If another thread
 * is waiting on the mutex lock, then it will be resumed.
 *
 * @param mutex : a pointer to the mutex handle to be unlocked
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_unlock_mutex( wiced_mutex_t* mutex );


/** De-initialise a mutex
 *
 * Deletes a mutex created with @ref wiced_rtos_init_mutex
 *
 * @param mutex : a pointer to the mutex handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deinit_mutex( wiced_mutex_t* mutex );


/** @} */
/*****************************************************************************/
/** @addtogroup queues       Queues
 *  @ingroup rtos
 *
 * Queue management functionss
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a queue
 *
 * Initialises a FIFO queue
 *
 * @param queue : a pointer to the queue handle to be initialised
 * @param name  : a text string name for the queue (NULL is allowed)
 * @param message_size : size in bytes of objects that will be held in the queue
 * @param number_of_messages : depth of the queue - i.e. max number of objects in the queue
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_init_queue( wiced_queue_t* queue, const char* name, uint32_t message_size, uint32_t number_of_messages );


/** Pushes an object onto a queue
 *
 * Pushes an object onto a queue
 *
 * @param queue : a pointer to the queue handle
 * @param message : the object to be added to the queue. Size is assumed to be
 *                  the size specified in @ref wiced_rtos_init_queue
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error or timeout occurred
 */
wiced_result_t wiced_rtos_push_to_queue( wiced_queue_t* queue, void* message, uint32_t timeout_ms );


/** Pops an object off a queue
 *
 * Pops an object off a queue
 *
 * @param queue : a pointer to the queue handle
 * @param message : pointer to a buffer that will receive the object being
 *                  popped off the queue. Size is assumed to be
 *                  the size specified in @ref wiced_rtos_init_queue , hence
 *                  you must ensure the buffer is long enough or memory
 *                  corruption will result
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error or timeout occurred
 */
wiced_result_t wiced_rtos_pop_from_queue( wiced_queue_t* queue, void* message, uint32_t timeout_ms );


/** De-initialise a queue
 *
 * Deletes a queue created with @ref wiced_rtos_init_queue
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deinit_queue( wiced_queue_t* queue );


/** Check if a queue is empty
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    WICED_SUCCESS : queue is empty.
 * @return    WICED_ERROR   : queue is not empty.
 */
wiced_result_t wiced_rtos_is_queue_empty( wiced_queue_t* queue );


/** Check if a queue is full
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    WICED_SUCCESS : queue is full.
 * @return    WICED_ERROR   : queue is not full.
 */
wiced_result_t wiced_rtos_is_queue_full( wiced_queue_t* queue );


/** Get the queue occupancy
 *
 * @param queue : a pointer to the queue handle
 * @param count : pointer to integer for storing occupancy count
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_get_queue_occupancy( wiced_queue_t* queue, uint32_t *count );


/** @} */
/*****************************************************************************/
/** @addtogroup rtostmr       RTOS timers
 *  @ingroup rtos
 *
 * RTOS timer management functions
 * These timers are based on the RTOS time-slice scheduling, so are not
 * highly accurate. They are also affected by high loading on the processor.
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a RTOS timer
 *
 * Initialises a RTOS timer
 * Timer does not start running until @ref wiced_rtos_start_timer is called
 *
 * @param timer    : a pointer to the timer handle to be initialised
 * @param time_ms  : Timer period in milliseconds
 * @param function : the callback handler function that is called each
 *                   time the timer expires
 * @param arg      : an argument that will be passed to the callback
 *                   function
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_init_timer( wiced_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg );


/** Starts a RTOS timer running
 *
 * Starts a RTOS timer running. Timer must have been previously
 * initialised with @ref wiced_rtos_init_timer
 *
 * @param timer    : a pointer to the timer handle to start
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_start_timer( wiced_timer_t* timer );


/** Stops a running RTOS timer
 *
 * Stops a running RTOS timer. Timer must have been previously
 * started with @ref wiced_rtos_start_timer
 *
 * @param timer    : a pointer to the timer handle to stop
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_stop_timer( wiced_timer_t* timer );


/** De-initialise a RTOS timer
 *
 * Deletes a RTOS timer created with @ref wiced_rtos_init_timer
 *
 * @param timer : a pointer to the RTOS timer handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deinit_timer( wiced_timer_t* timer );


/** Check if an RTOS timer is running
 *
 * @param timer : a pointer to the RTOS timer handle
 *
 * @return    WICED_SUCCESS : if running.
 * @return    WICED_ERROR   : if not running
 */
wiced_result_t wiced_rtos_is_timer_running( wiced_timer_t* timer );

/** @} */
/*****************************************************************************/
/** @addtogroup worker       Worker Threads
 *  @ingroup rtos
 *
 * Worker thread management functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Creates a worker thread
 *
 * Creates a worker thread
 * A worker thread is a thread in whose context timed and asynchronous events
 * execute.
 *
 * @param worker_thread    : a pointer to the worker thread to be created
 * @param priority         : thread priority
 * @param stack_size       : thread's stack size in number of bytes
 * @param event_queue_size : number of events can be pushed into the queue
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_create_worker_thread( wiced_worker_thread_t* worker_thread, uint8_t priority, uint32_t stack_size, uint32_t event_queue_size );


/** Deletes a worker thread
 *
 * Deletes a worker thread
 *
 * @param worker_thread : a pointer to the worker thread to be created
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_delete_worker_thread( wiced_worker_thread_t* worker_thread );

/** @} */
/*****************************************************************************/
/** @addtogroup events       Events
 *  @ingroup rtos
 *
 * Event management functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Requests a function be called at a regular interval
 *
 * This function registers a function that will be called at a regular
 * interval. Since this is based on the RTOS time-slice scheduling, the
 * accuracy is not high, and is affected by processor load.
 *
 * @param event_object  : pointer to a event handle which will be initialised
 * @param worker_thread : pointer to the worker thread in whose context the
 *                        callback function runs on
 * @param function      : the callback function that is to be called regularly
 * @param time_ms       : the time period between function calls in milliseconds
 * @param arg           : an argument that will be supplied to the function when
 *                        it is called
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_register_timed_event( wiced_timed_event_t* event_object, wiced_worker_thread_t* worker_thread, event_handler_t function, uint32_t time_ms, void* arg );


/** Removes a request for a regular function execution
 *
 * This function de-registers a function that has previously been set-up
 * with @ref wiced_rtos_register_timed_event.
 *
 * @param event_object : the event handle used with @ref wiced_rtos_register_timed_event
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deregister_timed_event( wiced_timed_event_t* event_object );


/** Sends an asynchronous event to the associated worker thread
 *
 * Sends an asynchronous event to the associated worker thread
 *
 * @param worker_thread :the worker thread in which context the callback should execute from
 * @param function      : the callback function to be called from the worker thread
 * @param arg           : the argument to be passed to the callback function
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_send_asynchronous_event( wiced_worker_thread_t* worker_thread, event_handler_t function, void* arg );

/** @} */
/*****************************************************************************/
/** @addtogroup eventflags       Event Flags
 *  @ingroup rtos
 *
 * Event flags management functions
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialise an event flags
 *
 * @param event_flags : a pointer to the event flags handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_init_event_flags( wiced_event_flags_t* event_flags );


/** Wait for event flags to be set
 *
 * @param event_flags        : a pointer to the event flags handle
 * @param flags_to_wait_for  : a group of event flags (ORed bit-fields) to wait for
 * @param flags_set          : event flag(s) set
 * @param clear_set_flags    : option to clear set flag(s)
 * @param wait_option        : wait option
 * @param timeout_ms         : timeout in milliseconds
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_wait_for_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_wait_for, uint32_t* flags_set, wiced_bool_t clear_set_flags, wiced_event_flags_wait_option_t wait_option, uint32_t timeout_ms );


/** Set event flags
 *
 * @param event_flags  : a pointer to the event flags handle
 * @param flags_to_set : a group of event flags (ORed bit-fields) to set
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_set_event_flags( wiced_event_flags_t* event_flags, uint32_t flags_to_set );


/** De-initialise an event flags
 *
 * @param event_flags : a pointer to the event flags handle
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_rtos_deinit_event_flags( wiced_event_flags_t* event_flags );

/** @} */


#ifdef __cplusplus
} /*extern "C" */
#endif
