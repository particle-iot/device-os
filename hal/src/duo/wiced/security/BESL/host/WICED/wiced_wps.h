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

extern besl_result_t besl_wps_init                        ( wps_agent_t* workspace, const besl_wps_device_detail_t* details, wps_agent_type_t type, wwd_interface_t interface );
extern besl_result_t besl_wps_get_result                  ( wps_agent_t* workspace );
extern besl_result_t besl_wps_deinit                      ( wps_agent_t* workspace );
extern besl_result_t besl_wps_start                       ( wps_agent_t* workspace, besl_wps_mode_t mode, const char* password, besl_wps_credential_t* credentials, uint16_t credential_length );
extern besl_result_t besl_p2p_wps_start                   ( wps_agent_t* workspace );
extern besl_result_t besl_wps_restart                     ( wps_agent_t* workspace );
extern besl_result_t besl_wps_reset_registrar             ( wps_agent_t* workspace, besl_mac_t* mac );
extern besl_result_t besl_wps_wait_till_complete          ( wps_agent_t* workspace );
extern besl_result_t besl_wps_abort                       ( wps_agent_t* workspace );
extern besl_result_t besl_wps_management_set_event_handler( wps_agent_t* workspace, wiced_bool_t enable );
extern besl_result_t besl_wps_scan                        ( wps_agent_t* workspace, wps_ap_t** ap_array, uint16_t* ap_array_size, wwd_interface_t interface );
extern besl_result_t besl_wps_set_directed_wps_target     ( wps_agent_t* workspace, wps_ap_t* ap, uint32_t maximum_join_attempts );
extern void           besl_wps_generate_pin                ( char*        wps_pin_string );
extern int            besl_wps_validate_pin_checksum       ( const char* str );
#ifdef __cplusplus
} /*extern "C" */
#endif
