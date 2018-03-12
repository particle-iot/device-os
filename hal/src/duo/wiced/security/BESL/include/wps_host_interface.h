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
#include "wps_host.h"
#include "wps_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AP_LIST_SIZE             10
#define CREDENTIAL_LIST_LENGTH   5

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint8_t  ssid_length;
    uint8_t  ssid[32];
    uint16_t encryption_type;
    uint16_t authentication_type;
    uint8_t  network_key_length;
    uint8_t  network_key[64];
} wps_credential_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *          WPS to Host Function Declarations
 ******************************************************/

/* Association functions */
extern wps_result_t wps_host_join( void* workspace, wps_ap_t* ap, wwd_interface_t interface );
extern wps_result_t wps_host_leave( wwd_interface_t interface );

/* Timing functions */
extern void wps_host_start_timer( void* workspace, uint32_t timeout );
extern void wps_host_stop_timer ( void* workspace );

/* IE management functions */
extern void wps_host_add_vendor_ie   ( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask );
extern void wps_host_remove_vendor_ie( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask );

/* Scanning functions */
extern void         wps_host_scan                 ( wps_agent_t* workspace, wps_scan_handler_t result_handler, wwd_interface_t interface );
extern wps_ap_t*    wps_host_store_ap             ( void* workspace, wl_escan_result_t* scan_result, wps_uuid_t* uuid );
extern wps_ap_t*    wps_host_retrieve_ap          ( void* workspace );
extern uint16_t     wps_host_get_ap_list_size     ( void* workspace);
extern wps_result_t wps_enrollee_pbc_overlap_check( wps_agent_t* workspace );

/* Credential management functions */
extern void wps_host_store_credential   ( void* workspace, wps_credential_t* credential );
extern void wps_host_retrieve_credential( void* workspace, wps_credential_t* credential );

/* Authorized MACs API */
extern void wps_host_get_authorized_macs( void* workspace, besl_mac_t** mac_list, uint8_t* mac_list_length );

/******************************************************
 *          Host to WPS Function Declarations
 ******************************************************/

extern wps_result_t wps_process_event( wps_agent_t* workspace, besl_event_message_t* event );
extern void wps_init_workspace       ( wps_agent_t* workspace );
extern void wps_deinit_workspace     ( wps_agent_t* workspace );
extern void wps_reset_workspace      ( wps_agent_t* workspace, wwd_interface_t interface );
extern void wps_scan_result_handler  ( wl_escan_result_t* result, void* user_data );
extern void wps_prepare_workspace_crypto   ( wps_agent_t* workspace );
extern wps_result_t wps_advertise_registrar( wps_agent_t* workspace, uint8_t selected_registrar );
extern void wps_register_result_callback( wps_agent_t* workspace, void (*wps_result_callback)(wps_result_t*) );

void wps_register_internal_result_callback( wps_agent_t* workspace, void (*wps_internal_result_callback)(wps_result_t*) );

#ifdef __cplusplus
} /*extern "C" */
#endif
