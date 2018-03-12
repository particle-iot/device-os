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

#include "platform_isr.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/* Use this macro to define an RTOS-aware interrupt handler where RTOS
 * primitives can be safely accessed.
 * Current port has no vectored interrupt controller, so single entry point
 * to all interrupts is used. And this entry is wrapped with RTOS-aware code,
 * so all interrupts are actually RTOS-aware.
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR( my_irq_handler )
 * {
 *     // Do something here
 * }
 */

#if defined( __GNUC__ )

#define WWD_RTOS_DEFINE_ISR( function )     \
    void function( void ); \
    PLATFORM_DEFINE_NAKED_ISR( __tx_ ## function ) \
    { \
        __asm__( "PUSH {lr}" ); \
        __asm__( "bl _tx_thread_context_save" ); \
        function(); \
        __asm__( "b  _tx_thread_context_restore" ); \
    } \
    PLATFORM_DEFINE_ISR( function)

#elif defined ( __IAR_SYSTEMS_ICC__ )

/* IAR will complain that the inline asm cannot see _tx_XXX symbols,
 * so include prototypes.  If labels aren't local to the translation
 * unit IAR will complain, so it's not enough to include prototypes
 * for inline assembly to work.  Instead, call _tx_XXX functions
 * directly.  _tx_thread_context_restore will return from exception,
 * so BL is okay here.
 */
#define WWD_RTOS_DEFINE_ISR( function )     \
    void function( void ); \
    PLATFORM_DEFINE_NAKED_ISR( __tx_ ## function ) \
    { \
        extern void _tx_thread_context_save(void); \
        extern void _tx_thread_context_restore(void); \
        __asm volatile( "PUSH {lr}" ); \
        _tx_thread_context_save(); \
        function(); \
        _tx_thread_context_restore(); \
    } \
    PLATFORM_DEFINE_ISR( function)

#endif

/* Macro for mapping a function defined using WWD_RTOS_DEFINE_ISR
 * to an interrupt handler declared in
 * <Wiced-SDK>/WICED/platform/MCU/<Family>/platform_isr_interface.h
 *
 * @usage:
 * WWD_RTOS_MAP_ISR( my_irq, USART1_irq )
 */
#define WWD_RTOS_MAP_ISR( function, isr )        PLATFORM_MAP_ISR( __tx_ ## function, isr )


/* Use this macro to define function which serves as ISR demuxer.
 * It is used when no vectored interrupt controller, and single
 * vector triggered for all interrupts.
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR_DEMUXER( my_irq_handler )
 * {
 *     // Do something here
 * }
 */
#define WWD_RTOS_DEFINE_ISR_DEMUXER( function )  PLATFORM_DEFINE_ISR( function )

/* Macro to declare that function is ISR demuxer.
 * Function has to be defined via WWD_RTOS_DEFINE_ISR_DEMUXER
 *
 * @usage:
 * WWD_RTOS_MAP_ISR_DEMUXER( my_irq_demuxer )
 */
#define WWD_RTOS_MAP_ISR_DEMUXER( function )     PLATFORM_MAP_ISR( function, _tx_platform_irq_demuxer )

/******************************************************
 *                    Constants
 ******************************************************/

/* Define interrupt handlers needed by ThreadX. These defines are used by the vector table. */
#define SVC_irq       __tx_SVCallHandler
#define PENDSV_irq    __tx_PendSVHandler
#define SYSTICK_irq   __tx_SysTickHandler

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern void __tx_SVCallHandler ( void );
extern void __tx_PendSVHandler ( void );
extern void __tx_SysTickHandler ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
