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

#include "p2p_structures.h"
#include "wwd_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

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

extern const wwd_event_num_t p2p_discovery_events[];
extern const wwd_event_num_t p2p_group_owner_events[];

/******************************************************
 *              Function Declarations
 ******************************************************/

extern besl_result_t                 p2p_stop                                 ( p2p_workspace_t* workspace );
extern p2p_client_info_descriptor_t* p2p_host_find_associated_p2p_device      ( p2p_workspace_t* workspace, const besl_mac_t* p2p_device_address );
extern besl_mac_t*                   p2p_host_find_associated_legacy_device   ( p2p_workspace_t* workspace, const besl_mac_t* mac_address );
extern besl_result_t                 p2p_host_notify_connection_request       ( p2p_workspace_t* workspace, p2p_discovered_device_t* device );
extern besl_result_t                 p2p_host_send_message                    ( p2p_message_t* message, uint32_t timeout_ms );
extern void                          p2p_thread_start                         ( p2p_workspace_t* workspace );
extern void                          p2p_host_add_vendor_ie                   ( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask );
extern void                          p2p_host_remove_vendor_ie                ( uint32_t interface, void* data, uint16_t data_length, uint32_t packet_mask );
extern void*                         p2p_event_handler                        ( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data );
extern void                          p2p_rewrite_group_owner_management_ies   ( void* wps_agent_owner );
extern besl_result_t                 p2p_process_probe_request                ( p2p_workspace_t* workspace, const uint8_t* data, uint32_t data_length );
extern void                          p2p_process_action_frame                 ( p2p_workspace_t* workspace, const uint8_t* data, const wwd_event_header_t* event_header );
extern p2p_discovered_device_t*      p2p_find_device                          ( p2p_workspace_t* workspace, besl_mac_t* mac);
extern besl_result_t                 p2p_process_new_device_data              ( p2p_workspace_t* workspace, p2p_discovered_device_t* device );
extern besl_result_t                 p2p_process_association_request          ( p2p_workspace_t* workspace, const uint8_t* data, const wwd_event_header_t* event_header );
extern besl_result_t                 p2p_update_devices                       ( p2p_workspace_t* workspace );
extern besl_result_t                 p2p_deinit                               ( p2p_workspace_t* workspace );
extern void                          p2p_stop_timer                           ( void* workspace );
extern void                          p2p_clear_event_queue                    ( void );

#ifdef __cplusplus
} /*extern "C" */
#endif
