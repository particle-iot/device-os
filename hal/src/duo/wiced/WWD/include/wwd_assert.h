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
 *  Defines macro for assertions
 *
 */
#pragma once

#include "wwd_debug.h"
#include "platform_assert.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Macros
 ******************************************************/

#ifdef DEBUG
    #ifdef WPRINT_ENABLE_ERROR
        #define WPRINT_ERROR(args)                      do { WPRINT_MACRO(args); WICED_ASSERTION_FAIL_ACTION(); } while(0)
        #define wiced_assert( error_string, assertion ) do { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION(); } } while(0)
        #define wiced_minor_assert( error_string, assertion )   do { if ( !(assertion) ) WPRINT_MACRO( error_string ); } while(0)
    #else
        #define WPRINT_ERROR(args)                      do { WICED_ASSERTION_FAIL_ACTION();} while(0)
        #define wiced_assert( error_string, assertion ) do { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION();} } while(0)
        #define wiced_minor_assert( error_string, assertion )   do { (void)(assertion); } while(0)
    #endif
#else
    #define wiced_assert( error_string, assertion )         do { (void)(assertion); } while(0)
    #define wiced_minor_assert( error_string, assertion )   do { (void)(assertion); } while(0)
#endif

#ifdef __GNUC__
#define WICED_UNUSED_VAR __attribute__ ((unused))
#else
#define WICED_UNUSED_VAR
#endif

#define wiced_static_assert( descr, expr ) \
{ \
    /* Make sure the expression is constant. */ \
    typedef enum { _STATIC_ASSERT_NOT_CONSTANT = (expr) } _static_assert_e WICED_UNUSED_VAR; \
    /* Make sure the expression is true. */ \
    typedef char STATIC_ASSERT_FAIL_##descr[(expr) ? 1 : -1] WICED_UNUSED_VAR; \
}

/** @endcond */

#ifdef __cplusplus
} /* extern "C" */
#endif
