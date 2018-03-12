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
 * Defines macros describe cache
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef PLATFORM_L1_CACHE_SHIFT

#define PLATFORM_L1_CACHE_BYTES             (1U << PLATFORM_L1_CACHE_SHIFT)
#define PLATFORM_L1_CACHE_LINE_MASK         (PLATFORM_L1_CACHE_BYTES - 1)
#define PLATFORM_L1_CACHE_ROUND_UP(a)       (((a) + PLATFORM_L1_CACHE_LINE_MASK) & ~(PLATFORM_L1_CACHE_LINE_MASK))
#define PLATFORM_L1_CACHE_ROUND_DOWN(a)     ((a) & ~(PLATFORM_L1_CACHE_LINE_MASK))
#define PLATFORM_L1_CACHE_PTR_ROUND_UP(p)   ((void*)PLATFORM_L1_CACHE_ROUND_UP((uint32_t)(p)))
#define PLATFORM_L1_CACHE_PTR_ROUND_DOWN(p) ((void*)PLATFORM_L1_CACHE_ROUND_DOWN((uint32_t)(p)))
#define PLATFORM_L1_CACHE_LINE_OFFSET(a)    ((uint32_t)(a) & (PLATFORM_L1_CACHE_LINE_MASK) )

#else

#define PLATFORM_L1_CACHE_BYTES             (0)
#define PLATFORM_L1_CACHE_LINE_MASK         (0)
#define PLATFORM_L1_CACHE_ROUND_UP(a)       (a)
#define PLATFORM_L1_CACHE_ROUND_DOWN(a)     (a)
#define PLATFORM_L1_CACHE_PTR_ROUND_UP(a)   (a)
#define PLATFORM_L1_CACHE_PTR_ROUND_DOWN(a) (a)
#define PLATFORM_L1_CACHE_LINE_OFFSET(a)    (0)

#endif /* PLATFORM_L1_CACHE_SHIFT */

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
