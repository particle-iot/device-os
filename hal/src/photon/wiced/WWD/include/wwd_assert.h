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
    #else
        #define WPRINT_ERROR(args)                      do { WICED_ASSERTION_FAIL_ACTION();} while(0)
        #define wiced_assert( error_string, assertion ) do { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION();} } while(0)
    #endif
#else
    #define wiced_assert( error_string, assertion )
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
