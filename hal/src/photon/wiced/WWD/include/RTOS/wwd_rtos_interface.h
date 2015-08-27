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

/** @file
 *  Defines the WICED RTOS Interface.
 *
 *  Provides prototypes for functions that allow WICED to use RTOS resources
 *  such as threads, semaphores & timing functions in an abstract way.
 */

#ifndef INCLUDED_WWD_RTOS_INTERFACE_H_
#define INCLUDED_WWD_RTOS_INTERFACE_H_

#include <stdint.h>
#include "wwd_rtos.h"
#include "wwd_structures.h"

#ifndef RTOS_HIGHER_PRIORTIY_THAN
#error wwd_rtos.h must define RTOS_HIGHER_PRIORTIY_THAN(x)
#endif
#ifndef RTOS_LOWER_PRIORTIY_THAN
#error wwd_rtos.h must define RTOS_LOWER_PRIORTIY_THAN(x)
#endif
#ifndef RTOS_LOWEST_PRIORITY
#error wwd_rtos.h must define RTOS_LOWEST_PRIORITY
#endif
#ifndef RTOS_HIGHEST_PRIORITY
#error wwd_rtos.h must define RTOS_HIGHEST_PRIORITY
#endif
#ifndef RTOS_DEFAULT_THREAD_PRIORITY
#error wwd_rtos.h must define RTOS_DEFAULT_THREAD_PRIORITY
#endif
#if !(defined(RTOS_USE_DYNAMIC_THREAD_STACK) || defined(RTOS_USE_STATIC_THREAD_STACK))
#error wwd_rtos.h must define either RTOS_USE_DYNAMIC_THREAD_STACK or RTOS_USE_STATIC_THREAD_STACK
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Constants
 ******************************************************/

#define NEVER_TIMEOUT ((uint32_t)0xffffffffUL)  /* Used with host_rtos_get_semaphore */

/******************************************************
 *             Structures & Typedefs
 ******************************************************/

/** @endcond */


/** @addtogroup rtosif RTOS Interface
 * Allows WICED to use use RTOS resources
 *  such as threads, semaphores & timing functions in an abstract way.
 *  @{
 */

/******************************************************
 *             Function declarations
 ******************************************************/

/* Thread management functions */

/**
 * Create a thread
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to create a new thread and start it running.
 *
 * @param thread         : pointer to a variable which will receive the new thread handle
 * @param entry_function : function pointer which points to the main function for the new thread
 * @param name           : a string thread name used for a debugger
 * @param stack_size     : the size of the thread stack in bytes
 * @param priority       : the priority of the thread
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t host_rtos_create_thread( /*@out@*/ host_thread_type_t* thread, void(*entry_function)( uint32_t arg ), const char* name, /*@null@*/ void* stack, uint32_t stack_size, uint32_t priority ) /*@modifies *thread@*/;


/**
 * Create a thread with specific thread argument
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 *
 * @param thread         : pointer to a variable which will receive the new thread handle
 * @param entry_function : function pointer which points to the main function for the new thread
 * @param name           : a string thread name used for a debugger
 * @param stack_size     : the size of the thread stack in bytes
 * @param priority       : the priority of the thread
 * @param arg            : the argument to pass to the new thread
 *
 * @return WWD result code
 */
extern wwd_result_t host_rtos_create_thread_with_arg( /*@out@*/ host_thread_type_t* thread, void(*entry_function)( uint32_t ), const char* name, /*@null@*/ void* stack, uint32_t stack_size, uint32_t priority, uint32_t arg );

/**
 * Exit a thread
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to exit the current thread just before its main
 * function would otherwise return. Some RTOSs allow threads to exit by simply returning
 * from their main function. If this is the case, then the implementation of
 * this function should be empty.
 *
 * @param thread         : Pointer to the current thread handle
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t host_rtos_finish_thread( host_thread_type_t* thread ) /*@modifies *thread@*/;


/**
 * Deletes a terminated thread
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * Some RTOS implementations require that another thread deletes any terminated thread
 * If RTOS does not require this, leave empty
 *
 * @param thread         : handle of the terminated thread to delete
 *
 * @returns WWD_SUCCESS on success, Error code otherwise
 */
extern wwd_result_t host_rtos_delete_terminated_thread( host_thread_type_t* thread );


/**
 * Waits for a thread to complete
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 *
 * @param thread         : handle of the thread to wait for
 *
 * @returns WWD_SUCCESS on success, Error code otherwise
 */
extern wwd_result_t host_rtos_join_thread( host_thread_type_t* thread );

/* Semaphore management functions */

/**
 * Create a Semaphore
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to create a semaphore
 *
 * @param semaphore         : Pointer to the semaphore handle to be initialized
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t host_rtos_init_semaphore(  /*@special@*/ /*@out@*/ host_semaphore_type_t* semaphore ) /*@allocates *semaphore@*/  /*@defines **semaphore@*/;

/**
 * Get a semaphore
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to get a semaphore.
 * - If the semaphore value is greater than zero, the sempahore value is decremented and the function returns immediately.
 * - If the semaphore value is zero, the current thread is put on a queue of threads waiting on the
 *   semaphore, and sleeps until another thread sets the semaphore and causes it to wake. Upon waking, the
 *   semaphore is decremented and the function returns.
 *
 * @note : This function must not be called from an interrupt context as it may block.
 *
 * @param semaphore   : Pointer to the semaphore handle
 * @param timeout_ms  : Maximum number of milliseconds to wait while attempting to get
 *                      the semaphore. Use the NEVER_TIMEOUT constant to wait forever.
 * @param will_set_in_isr : True if the semaphore will be set in an ISR. Currently only used for NoOS/NoNS
 *
 * @return wwd_result_t : WWD_SUCCESS if semaphore was successfully acquired
 *                     : WICED_TIMEOUT if semaphore was not acquired before timeout_ms period
 */
extern wwd_result_t host_rtos_get_semaphore( host_semaphore_type_t* semaphore, uint32_t timeout_ms, wiced_bool_t will_set_in_isr ) /*@modifies internalState@*/;

/**
 * Set a semaphore
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to set a semaphore.
 * The value of the semaphore is incremented, and if there are any threads waiting on the semaphore,
 * then the first waiting thread is woken.
 *
 * Some RTOS implementations require different processing when setting a semaphore from within an ISR.
 * A parameter is provided to allow this.
 *
 * @param semaphore       : Pointer to the semaphore handle
 * @param called_from_ISR : Value of WICED_TRUE indicates calling from interrupt context
 *                          Value of WICED_FALSE indicates calling from normal thread context
 *
 * @return wwd_result_t : WWD_SUCCESS if semaphore was successfully set
 *                        : Error code if an error occurred
 *
 */
extern wwd_result_t host_rtos_set_semaphore( host_semaphore_type_t* semaphore, wiced_bool_t called_from_ISR );

/**
 * Deletes a semaphore
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to delete a semaphore.
 *
 * @param semaphore         : Pointer to the semaphore handle
 *
 * @return wwd_result_t : WWD_SUCCESS if semaphore was successfully deleted
 *                        : Error code if an error occurred
 */
extern wwd_result_t host_rtos_deinit_semaphore( /*@special@*/ host_semaphore_type_t* semaphore ) /*@releases *semaphore@*/;


/* Time management functions */

/**
 * Gets time in milliseconds since RTOS start
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to retrieve the current time.
 *
 * @note: Since this is only 32 bits, it will roll over every 49 days, 17 hours, 2 mins, 47.296 seconds
 *
 * @returns Time in milliseconds since the RTOS started.
 */
extern wwd_time_t host_rtos_get_time( void )  /*@modifies internalState@*/;

/**
 * Delay for a number of milliseconds
 *
 * Implemented in the WICED RTOS interface which is specific to the
 * RTOS in use.
 * WICED uses this function to delay processing
 * Processing of this function depends on the minimum sleep
 * time resolution of the RTOS.
 * The current thread should sleep for the longest period possible which
 * is less than the delay required, then makes up the difference
 * with a tight loop
 *
 * @return wwd_result_t : WWD_SUCCESS if delay was successful
 *                        : Error code if an error occurred
 *
 */
extern wwd_result_t host_rtos_delay_milliseconds( uint32_t num_ms );


/* Message queue management functions */

extern wwd_result_t host_rtos_init_queue( /*@special@*/ /*@out@*/ host_queue_type_t* queue, void* buffer, uint32_t buffer_size, uint32_t message_size ) /*@allocates *queue@*/  /*@defines **queue@*/;

extern wwd_result_t host_rtos_push_to_queue( host_queue_type_t* queue, void* message, uint32_t timeout_ms );

extern wwd_result_t host_rtos_pop_from_queue( host_queue_type_t* queue, void* message, uint32_t timeout_ms );

extern wwd_result_t host_rtos_deinit_queue( /*@special@*/host_queue_type_t* queue ) /*@releases *queue@*/;

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_RTOS_INTERFACE_H_ */
