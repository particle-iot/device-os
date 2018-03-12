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

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WEAK
#ifndef __MINGW32__
#define WEAK             __attribute__((weak))
#else
/* MinGW doesn't support weak */
#define WEAK
#endif
#endif

#ifndef MAY_BE_UNUSED
#define MAY_BE_UNUSED    __attribute__((unused))
#endif

#ifndef NORETURN
#define NORETURN         __attribute__((noreturn))
#endif

#ifndef ALIGNED
#define ALIGNED(size)    __attribute__((aligned(size)))
#endif

#ifndef SECTION
#define SECTION(name)    __attribute__((section(name)))
#endif

#ifndef NEVER_INLINE
#define NEVER_INLINE     __attribute__((noinline))
#endif

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE    __attribute__((always_inline))
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

#ifndef __linux__
void *memrchr( const void *s, int c, size_t n );
#endif

/* Windows doesn't come with support for strlcpy */
#if defined( WIN32 ) || defined( __linux__ ) || defined( __NUTTX__ )
size_t strlcpy (char *dest, const char *src, size_t size);
#endif /* WIN32 */

void platform_toolchain_sbrk_prepare(void* ptr, int incr);

#ifdef __cplusplus
} /* extern "C" */
#endif

