/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "p2p_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define P2P_WILDCARD_SSID     "DIRECT-"

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
 *         P2P to Host Function Declarations
 ******************************************************/

/* IE management functions */
extern void p2p_host_add_vendor_ie   ( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask);
extern void p2p_host_remove_vendor_ie( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask);

extern besl_result_t p2p_send_action_frame( p2p_workspace_t* workspace, p2p_discovered_device_t* device, p2p_action_frame_writer_t writer, uint32_t channel );

extern p2p_discovered_device_t* p2p_host_find_device(p2p_workspace_t* workspace, const besl_mac_t* mac);

extern void p2p_host_negotiation_complete( p2p_workspace_t* workspace );

/******************************************************
 *         Host to P2P Function Declarations
 ******************************************************/

extern besl_result_t            p2p_init                 ( p2p_workspace_t* workspace, const char* device_name );
extern besl_result_t            p2p_deinit               ( p2p_workspace_t* workspace );
extern besl_result_t            p2p_process_probe_request( p2p_workspace_t* workspace, const uint8_t* data, uint32_t data_length );
extern void                     p2p_process_action_frame ( p2p_workspace_t* workspace, p2p_discovered_device_t* device, const uint8_t* data, uint32_t data_length );
extern p2p_discovered_device_t* p2p_find_device          ( p2p_workspace_t* workspace, const besl_mac_t* mac);
extern besl_result_t            p2p_process_new_device_data( p2p_workspace_t* workspace, p2p_discovered_device_t* device, const uint8_t* data, uint16_t data_length );

extern uint8_t* p2p_write_go_request( p2p_workspace_t* workspace, p2p_discovered_device_t* device, void* buffer );

extern besl_result_t p2p_write_association_request_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_probe_response_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_beacon_ie( p2p_workspace_t* workspace );

#ifdef __cplusplus
} /*extern "C" */
#endif
