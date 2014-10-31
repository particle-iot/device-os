/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_INTERNAL_H
#define INCLUDED_WWD_INTERNAL_H

#include <stdint.h>
#include "wwd_constants.h" /* for wwd_result_t */
#include "network/wwd_network_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

typedef enum
{
    /* Note : If changing this, core_base_address must be changed also */
    ARM_CORE    = 0,
    SOCRAM_CORE = 1,
    SDIOD_CORE  = 2
} device_core_t;

typedef enum
{
    WLAN_DOWN,
    WLAN_UP
} wlan_state_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    wlan_state_t         state;
    wiced_country_code_t country_code;
    uint32_t             keep_wlan_awake;
} wwd_wlan_status_t;

#define WWD_WLAN_KEEP_AWAKE( )  do { wwd_wlan_status.keep_wlan_awake++; } while (0)
#define WWD_WLAN_LET_SLEEP( )   do { wwd_wlan_status.keep_wlan_awake--; } while (0)

/******************************************************
 *             Function declarations
 ******************************************************/

extern wiced_bool_t wwd_wifi_ap_is_up;
extern uint8_t      wwd_tos_map[8];


/* Device core control functions */
extern wwd_result_t wwd_disable_device_core    ( device_core_t core_id );
extern wwd_result_t wwd_reset_device_core      ( device_core_t core_id );
extern wwd_result_t wwd_device_core_is_up      ( device_core_t core_id );
extern wwd_result_t wwd_wifi_set_down          ( wwd_interface_t interface );
extern void         wwd_set_country            ( wiced_country_code_t code );

/******************************************************
 *             Global variables
 ******************************************************/

extern wwd_wlan_status_t wwd_wlan_status;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_INTERNAL_H */
