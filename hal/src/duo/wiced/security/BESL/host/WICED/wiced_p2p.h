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

#include "besl_host.h"
#include "p2p_host_interface.h"

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

/******************************************************
 *               Function Declarations
 ******************************************************/

extern besl_result_t            besl_p2p_init                                           ( p2p_workspace_t* workspace, const besl_p2p_device_detail_t* device_details );
extern besl_result_t            besl_p2p_init_common                                    ( p2p_workspace_t* workspace, const besl_p2p_device_detail_t* device_details );
extern besl_result_t            besl_p2p_deinit                                         ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_start                                          ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_get_result                                     ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_get_progress                                   ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_start_negotiation                              ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_find_group_owner                               ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_get_discovered_peers                           ( p2p_workspace_t* workspace, p2p_discovered_device_t** devices, uint8_t* device_count );
extern besl_result_t            besl_p2p_group_owner_start                              ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_client_enable_powersave                        ( p2p_workspace_t* workspace, uint32_t power_save_mode );
extern besl_result_t            besl_p2p_send_action_frame                              ( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, p2p_action_frame_writer_t writer, uint32_t channel, uint32_t dwell_time );
extern besl_result_t            besl_p2p_start_registrar                                ( void );
extern p2p_discovered_device_t* besl_p2p_host_find_device                               ( p2p_workspace_t* workspace, const besl_mac_t* mac );
extern wiced_bool_t             besl_p2p_group_owner_is_up                              ( void );
extern void                     besl_p2p_host_negotiation_complete                      ( p2p_workspace_t* workspace );
extern void                     besl_p2p_register_p2p_device_connection_callback        ( p2p_workspace_t* workspace, void ( *p2p_connection_request_callback)(p2p_discovered_device_t*) );
extern void                     besl_p2p_register_legacy_device_connection_callback     ( p2p_workspace_t* workspace, void ( *p2p_legacy_device_connection_request_callback)(p2p_legacy_device_t*) );
extern void                     besl_p2p_register_group_formation_result_callback       ( p2p_workspace_t* workspace, void ( *p2p_group_formation_result_callback)(void*) );
extern void                     besl_p2p_register_wpa2_client_association_callback      ( p2p_workspace_t* workspace, void ( *p2p_client_wpa2_association_callback)(besl_mac_t*) );
extern void                     besl_p2p_register_wps_enrollee_association_callback     ( p2p_workspace_t* workspace, void ( *p2p_wps_enrollee_association_callback)(besl_mac_t*) );
extern void                     besl_p2p_register_wps_result_callback                   ( p2p_workspace_t* workspace, void (*p2p_wps_result_callback)(wps_result_t*) );
extern void                     besl_p2p_register_p2p_device_disassociation_callback    ( p2p_workspace_t* workspace, void (*p2p_device_disassociation_callback)(besl_mac_t*) );
extern void                     besl_p2p_register_legacy_device_disassociation_callback ( p2p_workspace_t* workspace, void (*p2p_non_p2p_device_disassociation_callback)(besl_mac_t*) );
extern besl_result_t            besl_p2p_get_group_formation_progress                   ( p2p_workspace_t* workspace );
extern besl_result_t            besl_p2p_go_get_client_wps_progress                     ( p2p_workspace_t* workspace );

#ifdef __cplusplus
} /* extern "C" */
#endif
