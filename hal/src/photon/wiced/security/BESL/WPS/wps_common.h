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

#ifdef __cplusplus
extern "C" {
#endif

#include "besl_constants.h"
#include "wps_structures.h"
#include "wps_host.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef DEBUG
#define BESL_LIBRARY_INFO(x)           BESL_INFO(x)
#define BESL_LIBRARY_DEBUG(x)          BESL_DEBUG(x)
#define BESL_LIBRARY_ERROR(x)          BESL_ERROR(x)
#define BESL_LIBRARY_ASSERT(string, x) BESL_ASSERT(string, x)
#else
#define BESL_LIBRARY_INFO(x)
#define BESL_LIBRARY_DEBUG(x)
#define BESL_LIBRARY_ERROR(x)
#define BESL_LIBRARY_ASSERT(string, x)
#endif

/******************************************************
 *                    Constants
 ******************************************************/

#define WPS_TEMPLATE_UUID        "\x77\x5b\x66\x80\xbf\xde\x11\xd3\x8d\x2f"
#define DOT11_MNG_VS_ID          221 /* d11 management Vendor Specific IE */

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint8_t* (*wps_packet_generator_t)(wps_agent_t* workspace, uint8_t* iter);

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint8_t                valid_message_type;
    uint8_t                outgoing_message_type;
    uint32_t               tlv_mask;
    uint32_t               encrypted_tlv_mask;
    wps_packet_generator_t packet_generator;
} wps_state_machine_state_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern void         wps_send_eapol_packet       ( besl_packet_t packet, wps_agent_t* workspace, eapol_packet_type_t type, besl_mac_t* their_mac_address, uint16_t content_size );
extern void         wps_abort                   ( wps_agent_t* workspace );
extern wps_result_t wps_send_basic_packet       ( wps_agent_t* workspace, uint8_t type, uint16_t optional_config_error );
extern void         wps_enrollee_init           ( wps_agent_t* workspace );
extern void         wps_enrollee_start          ( wps_agent_t* workspace, wwd_interface_t interface );
extern void         wps_enrollee_reset          ( wps_agent_t* workspace, wwd_interface_t interface );
extern void         wps_registrar_init          ( wps_agent_t* workspace );
extern void         wps_registrar_start         ( wps_agent_t* workspace );
extern void         wps_registrar_reset         ( wps_agent_t* workspace );
extern wps_result_t wps_pbc_overlap_check       ( const besl_mac_t* data );
extern void         wps_clear_pbc_overlap_array ( void );
extern void         wps_record_last_pbc_enrollee( const besl_mac_t* mac );
extern void         wps_update_pbc_overlap_array( wps_agent_t* workspace, const besl_mac_t* mac );
extern void         wps_pbc_overlap_array_notify( const besl_mac_t* mac );
#ifdef __cplusplus
} /*extern "C" */
#endif
