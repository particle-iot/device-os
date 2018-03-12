/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WWD_TOOLCHAIN_H
#define INCLUDED_WWD_TOOLCHAIN_H

#include <stddef.h>
#include "intrinsics.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WEAK __weak

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE
#endif

#ifndef MAY_BE_UNUSED
#define MAY_BE_UNUSED
#endif

#ifndef NORETURN
#define NORETURN
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
void *memmem( const void *haystack, size_t haystacklen, const void *needle, size_t needlelen );
void *memrchr( const void *s, int c, size_t n );
size_t strlcpy( char *dest, const char *src, size_t size );

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* #ifndef INCLUDED_WWD_TOOLCHAIN_H */
