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
 *  Deep-sleep support functions.
 *
 */

#pragma once

#include <stdint.h>
#include "platform_config.h"
#include "platform_map.h"
#include "platform_mcu_peripheral.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Macros
 ******************************************************/

#ifndef PLATFORM_DEEP_SLEEP_HEADER_INCLUDED
#error "Header file must not be included directly. Please use wiced_deep_sleep.h instead."
#endif

#define WICED_DEEP_SLEEP_STR_EXPAND( name )                      #name

#define WICED_DEEP_SLEEP_SECTION_NAME_SAVED_VAR( name )          ".deep_sleep_saved_vars."WICED_DEEP_SLEEP_STR_EXPAND( name )
#define WICED_DEEP_SLEEP_SECTION_NAME_EVENT_HANDLER( name )      ".deep_sleep_event_handlers."WICED_DEEP_SLEEP_STR_EXPAND( name )
#define WICED_DEEP_SLEEP_SECTION_NAME_EVENT_REGISTRATION( name ) ".deep_sleep_event_registrations."WICED_DEEP_SLEEP_STR_EXPAND( name )

#define WICED_DEEP_SLEEP_IS_WARMBOOT( ) \
    platform_mcu_powersave_is_warmboot( )

#if PLATFORM_APPS_POWERSAVE

#define WICED_DEEP_SLEEP_SAVED_VAR( var ) \
    SECTION( WICED_DEEP_SLEEP_SECTION_NAME_SAVED_VAR( var ) ) var

#define WICED_DEEP_SLEEP_EVENT_HANDLER( func_name ) \
    static void SECTION( WICED_DEEP_SLEEP_SECTION_NAME_EVENT_HANDLER( func_name ) ) func_name( wiced_deep_sleep_event_type_t event ); \
    const wiced_deep_sleep_event_registration_t SECTION( WICED_DEEP_SLEEP_SECTION_NAME_EVENT_REGISTRATION( func_name ) ) func_name##_registration = { .handler = &func_name }; \
    static void func_name( wiced_deep_sleep_event_type_t event )

#define WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( cond, event ) \
    do { if ( cond ) wiced_deep_sleep_call_event_handlers( event ); } while( 0 )

#define WICED_DEEP_SLEEP_IS_ENABLED( )                           1

#endif /* PLATFORM_APPS_POWERSAVE */

#define WICED_DEEP_SLEEP_IS_AON_SEGMENT( segment_addr, segment_size ) \
    ( ( (segment_addr) >= PLATFORM_SOCSRAM_CH0_AON_RAM_BASE(0x0)) && ( (segment_addr) + (segment_size) <= PLATFORM_SOCSRAM_CH0_AON_RAM_BASE(PLATFORM_SOCSRAM_AON_RAM_SIZE) ) )

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Enumerations
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    wiced_deep_sleep_event_handler_t handler;
} wiced_deep_sleep_event_registration_t;

typedef struct
{
    uint32_t entry_point;
    uint32_t app_address;
} wiced_deep_sleep_tiny_bootloader_config_t;

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

void SECTION( WICED_DEEP_SLEEP_SECTION_NAME_EVENT_HANDLER( wiced_deep_sleep_call_event_handlers ) ) wiced_deep_sleep_call_event_handlers( wiced_deep_sleep_event_type_t event );

#ifdef __cplusplus
} /* extern "C" */
#endif
