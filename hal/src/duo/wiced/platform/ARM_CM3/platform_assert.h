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
 * Defines macros for defining asserts for ARM-Cortex-M3 CPU
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#if defined ( __GNUC__ )

#if defined ( __clang__ )

static inline /*@noreturn@*/void WICED_TRIGGER_BREAKPOINT( void ) __attribute__((analyzer_noreturn))
{
    __asm__("bkpt");
}

#else

#if defined ( LINT ) /* Lint requires a prototype */

extern /*@noreturn@*/ void WICED_TRIGGER_BREAKPOINT( void );

#else /* #if defined ( LINT ) */

#define WICED_TRIGGER_BREAKPOINT( ) do { __asm__("bkpt"); } while (0)

#endif /* #if defined ( LINT ) */

#endif /* #if defined ( __clang__ ) */

#define WICED_ASSERTION_FAIL_ACTION() WICED_TRIGGER_BREAKPOINT()

#define WICED_DISABLE_INTERRUPTS() do { __asm__("CPSID i"); } while (0)

#define WICED_ENABLE_INTERRUPTS() do { __asm__("CPSIE i"); } while (0)

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WICED_TRIGGER_BREAKPOINT() do { __asm("bkpt 0"); } while (0)

#define WICED_ASSERTION_FAIL_ACTION() WICED_TRIGGER_BREAKPOINT()

#define WICED_DISABLE_INTERRUPTS() do { __asm("CPSID i"); } while (0)

#define WICED_ENABLE_INTERRUPTS() do { __asm("CPSIE i"); } while (0)

#endif

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
