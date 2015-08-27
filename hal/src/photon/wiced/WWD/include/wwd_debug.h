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

#ifndef INCLUDED_WWD_DEBUG_H
#define INCLUDED_WWD_DEBUG_H

#include "wiced_defaults.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif



#ifdef DEBUG
#include "platform_assert.h"
    #define WICED_BREAK_IF_DEBUG( ) WICED_ASSERTION_FAIL_ACTION()
#else
    #define WICED_BREAK_IF_DEBUG( )
#endif

/******************************************************
 *             Print declarations
 ******************************************************/

#define WPRINT_MACRO(args) do {printf args;} while(0==1)

/* WICED printing macros for general SDK/Library functions*/
#ifdef WPRINT_ENABLE_LIB_INFO
    #define WPRINT_LIB_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_LIB_INFO(args)
#endif
#ifdef WPRINT_ENABLE_LIB_DEBUG
    #define WPRINT_LIB_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_LIB_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_LIB_ERROR
    #define WPRINT_LIB_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_LIB_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for the Webserver*/
#ifdef WPRINT_ENABLE_WEBSERVER_INFO
    #define WPRINT_WEBSERVER_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WEBSERVER_INFO(args)
#endif
#ifdef WPRINT_ENABLE_WEBSERVER_DEBUG
    #define WPRINT_WEBSERVER_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WEBSERVER_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_WEBSERVER_ERROR
    #define WPRINT_WEBSERVER_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_WEBSERVER_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for Applications*/
#ifdef WPRINT_ENABLE_APP_INFO
    #define WPRINT_APP_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_APP_INFO(args)
#endif
#ifdef WPRINT_ENABLE_APP_DEBUG
    #define WPRINT_APP_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_APP_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_APP_ERROR
    #define WPRINT_APP_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_APP_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for Network Stacks */
#ifdef WPRINT_ENABLE_NETWORK_INFO
    #define WPRINT_NETWORK_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_NETWORK_INFO(args)
#endif
#ifdef WPRINT_ENABLE_NETWORK_DEBUG
    #define WPRINT_NETWORK_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_NETWORK_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_NETWORK_ERROR
    #define WPRINT_NETWORK_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_NETWORK_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for the RTOS*/
#ifdef WPRINT_ENABLE_RTOS_INFO
    #define WPRINT_RTOS_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_RTOS_INFO(args)
#endif
#ifdef WPRINT_ENABLE_RTOS_DEBUG
    #define WPRINT_RTOS_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_RTOS_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_RTOS_ERROR
    #define WPRINT_RTOS_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_RTOS_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for the Security stack*/
#ifdef WPRINT_ENABLE_SECURITY_INFO
    #define WPRINT_SECURITY_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_SECURITY_INFO(args)
#endif
#ifdef WPRINT_ENABLE_SECURITY_DEBUG
    #define WPRINT_SECURITY_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_SECURITY_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_SECURITY_ERROR
    #define WPRINT_SECURITY_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_SECURITY_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for the WPS stack*/
#ifdef WPRINT_ENABLE_WPS_INFO
    #define WPS_INFO(args) WPRINT_MACRO(args)
#else
    #define WPS_INFO(args)
#endif
#ifdef WPRINT_ENABLE_WPS_DEBUG
    #define WPS_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPS_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_WPS_ERROR
    #define WPS_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPS_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for the Supplicant stack*/
#ifdef WPRINT_ENABLE_SUPPLICANT_INFO
        #define SUPPLICANT_INFO(args) WPRINT_MACRO(args)
    #else
        #define SUPPLICANT_INFO(args)
    #endif
    #ifdef WPRINT_ENABLE_SUPPLICANT_DEBUG
        #define SUPPLICANT_DEBUG(args) WPRINT_MACRO(args)
    #else
        #define SUPPLCAINT_DEBUG(args)
    #endif
    #ifdef WPRINT_ENABLE_SUPPLICANT_ERROR
        #define SUPPLICANT_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
    #else
        #define SUPPLICANT_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
    #endif


/* WICED printing macros for Platforms*/
#ifdef WPRINT_ENABLE_PLATFORM_INFO
    #define WPRINT_PLATFORM_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_PLATFORM_INFO(args)
#endif
#ifdef WPRINT_ENABLE_PLATFORM_DEBUG
    #define WPRINT_PLATFORM_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_PLATFORM_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_PLATFORM_ERROR
    #define WPRINT_PLATFORM_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_PLATFORM_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif

/* WICED printing macros for Wiced Internal functions*/
#ifdef WPRINT_ENABLE_WICED_INFO
    #define WPRINT_WICED_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WICED_INFO(args)
#endif
#ifdef WPRINT_ENABLE_WICED_DEBUG
    #define WPRINT_WICED_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WICED_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_WICED_ERROR
    #define WPRINT_WICED_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_WICED_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif



/* WICED printing macros for Wiced Wi-Fi Driver*/
#ifdef WPRINT_ENABLE_WWD_INFO
    #define WPRINT_WWD_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WWD_INFO(args)
#endif
#ifdef WPRINT_ENABLE_WWD_DEBUG
    #define WPRINT_WWD_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_WWD_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_WWD_ERROR
    #define WPRINT_WWD_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_WWD_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_DEBUG_H */
