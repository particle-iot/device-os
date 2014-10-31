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
 *  Definitions for the FreeRTOS implementation of the Wiced RTOS
 *  abstraction layer.
 *
 */

#ifndef INCLUDED_WWD_RTOS_H_
#define INCLUDED_WWD_RTOS_H_

#include "platform_isr.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wwd_FreeRTOS_systick.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern void vPortSVCHandler    ( void );
extern void xPortPendSVHandler ( void );
extern void xPortSysTickHandler( void );

/* Define interrupt handlers needed by FreeRTOS. These defines are used by the
 * vector table.
 */
#define SVC_irq     vPortSVCHandler
#define PENDSV_irq  xPortPendSVHandler
#define SYSTICK_irq xPortSysTickHandler


/* Use this macro to define an RTOS-aware interrupt handler where RTOS
 * primitives can be safely accessed
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR( my_irq_handler )
 * {
 *     // Do something here
 * }
 */
#if defined( __GNUC__ )

#define WWD_RTOS_DEFINE_ISR( function ) \
        void function( void ); \
        __attribute__(( interrupt, used, section(IRQ_SECTION) )) void function( void )

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WWD_RTOS_DEFINE_ISR( function ) \
        void function( void ); \
        __irq __root void function( void )

#else

#define WWD_RTOS_DEFINE_ISR( function ) \
        void function( void );

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
        __attribute__(( alias( #function ))) void isr ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WWD_RTOS_MAP_ISR( function, isr ) \
        extern void isr( void ); \
        _Pragma( TO_STRING( weak isr=function ) )

#else

#define WWD_RTOS_MAP_ISR( function, isr )

#endif


#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x < RTOS_HIGHEST_PRIORITY ? x+1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x > RTOS_LOWEST_PRIORITY ? x-1 : RTOS_LOWEST_PRIORITY)
#define RTOS_LOWEST_PRIORITY             (0)
#define RTOS_HIGHEST_PRIORITY            (configMAX_PRIORITIES-1)
#define RTOS_DEFAULT_THREAD_PRIORITY     (1)

#define RTOS_USE_DYNAMIC_THREAD_STACK

#define malloc_get_current_thread( ) xTaskGetCurrentTaskHandle()
typedef xTaskHandle malloc_thread_handle;

#ifdef DEBUG
#define WWD_THREAD_STACK_SIZE        (732)   /* Stack checking requires a larger stack */
#else /* ifdef DEBUG */
#define WWD_THREAD_STACK_SIZE        (544)
#endif /* ifdef DEBUG */

/******************************************************
 *             Structures
 ******************************************************/

typedef xSemaphoreHandle    /*@abstract@*/ /*@only@*/ host_semaphore_type_t;  /** FreeRTOS definition of a semaphore */
typedef xTaskHandle         /*@abstract@*/ /*@only@*/ host_thread_type_t;     /** FreeRTOS definition of a thread handle */
typedef xQueueHandle        /*@abstract@*/ /*@only@*/ host_queue_type_t;      /** FreeRTOS definition of a message queue */

/*@external@*/ extern void vApplicationMallocFailedHook( void );
/*@external@*/ extern void vApplicationIdleSleepHook( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_RTOS_H_ */
