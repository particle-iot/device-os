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
 * Defines macros for defining and mapping interrupt handlers to the vector table of ARM-Cortex-M3 CPU
 */
#pragma once
#include "platform_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/* Section where IRQ handlers are placed */
#define IRQ_SECTION ".text.irq"

/* Macro for defining an interrupt handler (non-RTOS-aware)
 *
 * @warning:
 * Do NOT call any RTOS primitive functions from here. If you need to call them,
 * define your interrupt handler using WWD_RTOS_DEFINE_ISR()
 *
 * @usage:
 * PLATFORM_DEFINE_ISR( my_irq )
 * {
 *     // Do something here
 * }
 *
 */
#if defined ( __GNUC__ )
/* GCC */
#define PLATFORM_DEFINE_NAKED_ISR( function ) \
        void function( void ); \
        __attribute__(( naked, interrupt, used, section( IRQ_SECTION ) )) void function( void )

#define PLATFORM_DEFINE_ISR( function ) \
        void function( void ); \
        __attribute__(( interrupt, used, section( IRQ_SECTION ) )) void function( void )

#elif defined ( __IAR_SYSTEMS_ICC__ )
/* IAR Systems */
#define PLATFORM_DEFINE_ISR( function ) \
        void function( void ); \
        __irq __root void function( void )

#else

#define PLATFORM_DEFINE_ISR( name )

#endif


/* Macro for mapping a defined function to an interrupt handler declared in
 * <Wiced-SDK>/WICED/platform/<Arch>/<Family>/platform_isr_interface.h
 *
 * @usage:
 * PLATFORM_MAP_ISR( my_irq, USART1_irq )
 */
#if defined( __GNUC__ )

#define PLATFORM_MAP_ISR( function, isr ) \
        extern void isr( void ); \
        __attribute__(( alias( #function ))) void isr ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define PLATFORM_MAP_ISR( function, isr ) \
        extern void isr( void ); \
        _Pragma( TO_STRING( weak isr=function ) )

#else

#define PLATFORM_MAP_ISR( function, isr )

#endif


/* Macro for declaring a default handler for an unhandled interrupt
 *
 * @usage:
 * PLATFORM_SET_DEFAULT_ISR( USART1_irq, default_handler )
 */
#if defined( __GNUC__ )

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler ) \
        __attribute__(( weak, alias( #default_handler ))) void irq_handler ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler ) \
        _Pragma( TO_STRING( weak irq_handler=default_handler ) )

#else

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler )

#endif

/******************************************************
 *                    Constants
 ******************************************************/

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

#ifdef __cplusplus
} /*extern "C" */
#endif
