/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_DEBUG_H
#define INCLUDED_WWD_DEBUG_H

#include "wiced_defaults.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef DEBUG
#include "platform_assert.h"
    #define WICED_BREAK_IF_DEBUG( ) WICED_ASSERTION_FAIL_ACTION()
#else
    #define WICED_BREAK_IF_DEBUG( )
#endif

#ifdef WPRINT_PLATFORM_PERMISSION
int platform_wprint_permission(void);
#define WPRINT_PLATFORM_PERMISSION_FUNC() platform_wprint_permission()
#else
#define WPRINT_PLATFORM_PERMISSION_FUNC() 1
#endif

/******************************************************
 *             Print declarations
 ******************************************************/

#define WPRINT_MACRO(args) do {if (WPRINT_PLATFORM_PERMISSION_FUNC()) printf args;} while(0==1)

// extern void log_printf_wiced(const char *fmt, ...); 
// #define WPRINT_MACRO(args) do { log_printf_wiced args;} while(0==1)

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
        #define SUPPLICANT_DEBUG(args)
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
