/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_THREAD_INTERNAL_H
#define INCLUDED_WWD_THREAD_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "wwd_assert.h"
/******************************************************
 *                      Macros
 ******************************************************/

#define WWD_WLAN_KEEP_AWAKE() \
    do { \
        wwd_result_t verify_result; \
        wwd_wlan_status.keep_wlan_awake++; \
        verify_result = wwd_ensure_wlan_bus_is_up( ); \
        wiced_assert( "Could not bring bus up", ( verify_result == WWD_SUCCESS )); \
    } while (0)
#define WWD_WLAN_LET_SLEEP() \
    do { \
        wwd_wlan_status.keep_wlan_awake--; \
        if ( wwd_wlan_status.keep_wlan_awake == 0 ) \
            wwd_thread_notify( ); \
    } while (0)
#define WWD_WLAN_MAY_SLEEP() \
    ( ( wwd_wlan_status.keep_wlan_awake == 0 ) && ( wwd_wlan_status.state == WLAN_UP ) )

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_THREAD_INTERNAL_H */
