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

#define WICED_TRIGGER_BREAKPOINT( ) __asm__("bkpt")

#endif /* #if defined ( LINT ) */

#endif /* #if defined ( __clang__ ) */

#define WICED_ASSERTION_FAIL_ACTION() WICED_TRIGGER_BREAKPOINT()

#define WICED_DISABLE_INTERRUPTS() { __asm__("CPSID i"); }

#define WICED_ENABLE_INTERRUPTS() { __asm__("CPSIE i"); }

#elif defined ( __IAR_SYSTEMS_ICC__ )

#include <cmsis_iar.h>

#define WICED_TRIGGER_BREAKPOINT() __asm("bkpt 0")

#define WICED_ASSERTION_FAIL_ACTION() WICED_TRIGGER_BREAKPOINT()

#define WICED_DISABLE_INTERRUPTS() { __asm("CPSID i"); }

#define WICED_ENABLE_INTERRUPTS() { __asm("CPSIE i"); }

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
