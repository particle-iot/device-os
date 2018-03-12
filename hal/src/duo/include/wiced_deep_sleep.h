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

#include <stdint.h>

#include "platform_toolchain.h"

#include "wwd_buffer.h"
#include "wwd_constants.h"

#include "wiced_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                    Callback types
 ******************************************************/

typedef enum
{
    WICED_DEEP_SLEEP_EVENT_ENTER,
    WICED_DEEP_SLEEP_EVENT_CANCEL,
    WICED_DEEP_SLEEP_EVENT_LEAVE,
    WICED_DEEP_SLEEP_EVENT_WLAN_RESUME
} wiced_deep_sleep_event_type_t;

typedef void( *wiced_deep_sleep_event_handler_t )( wiced_deep_sleep_event_type_t event );

/******************************************************
 *                 Platform definitions
 ******************************************************/

#ifdef PLATFORM_DEEP_SLEEP
#define PLATFORM_DEEP_SLEEP_HEADER_INCLUDED
#include "platform_deep_sleep.h"
#endif /* PLATFORM_DEEP_SLEEP */

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WICED_DEEP_SLEEP_SAVED_VAR
#define WICED_DEEP_SLEEP_SAVED_VAR( var )                                               var
#endif

#ifndef WICED_DEEP_SLEEP_EVENT_HANDLER
#ifdef  __IAR_SYSTEMS_ICC__
#define IAR_ROOT_FUNC __root
#else
#define IAR_ROOT_FUNC
#endif /* __IAR_SYSTEMS_ICC__ */
#define WICED_DEEP_SLEEP_EVENT_HANDLER( func_name ) \
    static IAR_ROOT_FUNC void MAY_BE_UNUSED func_name( wiced_deep_sleep_event_type_t event )
#endif /* ndef WICED_DEEP_SLEEP_EVENT_HANDLER */

#ifndef WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS
#define WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( cond, event )
#endif

#ifndef WICED_DEEP_SLEEP_IS_WARMBOOT
#define WICED_DEEP_SLEEP_IS_WARMBOOT( )                                                 0
#endif

#ifndef WICED_DEEP_SLEEP_IS_ENABLED
#define WICED_DEEP_SLEEP_IS_ENABLED( )                                                  0
#endif

#ifndef WICED_DEEP_SLEEP_IS_WARMBOOT_HANDLE
#define WICED_DEEP_SLEEP_IS_WARMBOOT_HANDLE( )                                          ( WICED_DEEP_SLEEP_IS_ENABLED( ) && WICED_DEEP_SLEEP_IS_WARMBOOT( ) )
#endif

#ifndef WICED_DEEP_SLEEP_SAVE_PACKETS_NUM
#define WICED_DEEP_SLEEP_SAVE_PACKETS_NUM                                               0
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

uint32_t     wiced_deep_sleep_ticks_since_enter( void );

wiced_bool_t wiced_deep_sleep_save_packet( wiced_buffer_t buffer, wwd_interface_t interface );

void         wiced_deep_sleep_set_networking_ready( void );

void         wiced_deep_sleep_application_init_on_networking_ready_handler( void );

wiced_bool_t wiced_deep_sleep_is_networking_idle( wiced_interface_t interface );

#ifdef __cplusplus
} /* extern "C" */
#endif
