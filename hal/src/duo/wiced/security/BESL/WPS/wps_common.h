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
