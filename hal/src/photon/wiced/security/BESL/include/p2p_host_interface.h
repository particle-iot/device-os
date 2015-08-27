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
