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

//#include "p2p_structures.h"
//#include "wwd_events.h"
//#include "wwd_rtos.h"

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

extern uint8_t*      p2p_write_invitation_request( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_provision_discovery_response( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_negotiation_response( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_invitation_response( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_presence_response( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_negotiation_confirmation( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_device_discoverability_response( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_go_discoverability_request( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_provision_discovery_request( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern uint8_t*      p2p_write_negotiation_request( p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer );
extern besl_result_t p2p_write_association_request_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_wps_probe_response_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_wps_probe_request_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_probe_request_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_probe_response_ie( p2p_workspace_t* workspace );
extern besl_result_t p2p_write_beacon_ie( p2p_workspace_t* workspace );
extern void          p2p_remove_ies( p2p_workspace_t* workspace );

#ifdef __cplusplus
} /*extern "C" */
#endif
