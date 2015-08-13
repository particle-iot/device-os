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
extern besl_result_t            besl_p2p_send_action_frame                              ( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, p2p_action_frame_writer_t writer, uint32_t channel, wwd_interface_t interface, uint32_t dwell_time );
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
