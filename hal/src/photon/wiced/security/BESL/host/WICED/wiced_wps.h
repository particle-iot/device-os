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
