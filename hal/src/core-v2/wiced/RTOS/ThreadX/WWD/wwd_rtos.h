/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Definitions for the ThreadX implementation of the Wiced RTOS
 *  abstraction layer.
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include "platform_isr.h"
#include "tx_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern void __tx_SVCallHandler ( void );
extern void __tx_PendSVHandler ( void );
extern void __tx_SysTickHandler( void );

/* Define interrupt handlers needed by ThreadX. These defines are used by the
 * vector table.
 */
#define SVC_irq     __tx_SVCallHandler
#define PENDSV_irq  __tx_PendSVHandler
#define SYSTICK_irq __tx_SysTickHandler


/* Use this macro to define an RTOS-aware interrupt handler where RTOS
 * primitives can be safely accessed
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR( my_irq_handler )
 * {
 *     // Do something here
 * }
 */
#if defined ( __GNUC__ )

#define WWD_RTOS_DEFINE_ISR( function ) \
    void __tx_ ## function( void ); \
    void function( void ); \
    __attribute__(( naked, interrupt, used, section(IRQ_SECTION) )) void __tx_ ## function( void ) \
    { \
        __asm__( "PUSH {lr}" ); \
        __asm__( "bl _tx_thread_context_save" ); \
        function(); \
        __asm__( "b  _tx_thread_context_restore" ); \
    } \
    __attribute__(( interrupt, used, section(IRQ_SECTION) )) void function( void )

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WWD_RTOS_DEFINE_ISR( function ) \
    void rtos_ ## function( void ); \
    void function( void ); \
    __task __irq __root void rtos_ ## function( void ) \
    { \
        __asm( "PUSH {lr}" ); \
        __asm( "BL _tx_thread_context_save" ); \
        function(); \
        __asm( "B  _tx_thread_context_restore" ); \
    } \
    __irq __root void function( void )

#else

#define WWD_RTOS_DEFINE_ISR( function ) \
        void function( void )

#endif


/* Macro for mapping a function defined using WWD_RTOS_DEFINE_ISR
 * to an interrupt handler declared in
 * <Wiced-SDK>/WICED/platform/<Arch>/<Family>/platform_irq_handlers.h
 *
 * @usage:
 * WWD_RTOS_MAP_ISR( my_irq, USART1_irq )
 */
#if defined( __GNUC__ )

#define WWD_RTOS_MAP_ISR( function, isr ) \
        extern void isr( void ); \
        __attribute__(( alias( TO_STRING( __tx_ ## function )))) void isr ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WWD_RTOS_MAP_ISR( function, isr ) \
        extern void isr( void ); \
        _Pragma( TO_STRING( weak isr=__tx_ ## function ) )

#else

#define WWD_RTOS_MAP_ISR( function, isr )

#endif


#define malloc_get_current_thread( ) tx_thread_identify( )
typedef TX_THREAD* malloc_thread_handle;
#define wiced_thread_to_malloc_thread( thread ) ((malloc_thread_handle)(&(thread)->handle))


#define RTOS_HIGHER_PRIORTIY_THAN(x)     ((x) < RTOS_HIGHEST_PRIORITY ? (x)-1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      ((x) > RTOS_LOWEST_PRIORITY  ? (x)+1 : RTOS_LOWEST_PRIORITY )
#define RTOS_LOWEST_PRIORITY             (1023)
#define RTOS_HIGHEST_PRIORITY            (0)
#define RTOS_DEFAULT_THREAD_PRIORITY     (4)

#define RTOS_USE_STATIC_THREAD_STACK

#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (632)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (544)
#endif /* ifdef DEBUG */


/*
 * The number of system ticks per second
 */
#define SYSTICK_FREQUENCY  (1000)

/******************************************************
 *             Structures
 ******************************************************/

typedef TX_SEMAPHORE  host_semaphore_type_t; /** ThreadX definition of a semaphore */
typedef TX_THREAD     host_thread_type_t;    /** ThreadX definition of a thread handle */
typedef TX_QUEUE      host_queue_type_t;     /** ThreadX definition of a message queue */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
