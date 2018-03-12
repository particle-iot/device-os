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
#include <stdint.h>
#include "tls_types.h"
#include "besl_constants.h"
#include "supplicant_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void* tls_agent_packet_t;

/******************************************************
 *                Packed Structures
 ******************************************************/

/******************************************************
 *                Unpacked Structures
 ******************************************************/

typedef struct
{
    tls_agent_event_t           event_type;
    union
    {
        tls_agent_packet_t      packet;
        uint32_t                value;
    } data;
} tls_agent_event_message_t;

typedef struct
{
    void*                       tls_agent_host_workspace;
} tls_agent_workspace_t;

typedef struct supplicant_peap_state_s
{
    eap_type_t              eap_type;
    besl_result_t           result;
    supplicant_main_stage_t main_stage;
    uint8_t                 sub_stage;

    uint8_t                 identity[32];
    uint8_t                 identity_length;

    uint8_t                 password[32];
    uint8_t                 password_length;
} supplicant_peap_state_t;

typedef struct supplicant_mschapv2_identity_s
{
    uint8_t*                identity;
    uint8_t                 identity_length;

    /* in Windows password is UNICODE */
    uint8_t*                password;
    uint8_t                 password_length;
}supplicant_mschapv2_identity_t;

typedef struct supplicant_peap_workspace_s* supplicant_peap_workspace_ptr_t;

typedef struct
{
    eap_type_t                  eap_type;
    void*                       supplicant_host_workspace;
    uint32_t                    interface;
    besl_result_t               supplicant_result;

    /* State machine stages */
    supplicant_main_stage_t     current_main_stage;
    uint8_t                     current_sub_stage; /* Either a value from wps_eap_state_machine_stage_t or wps_state_machine_stage_t */

    /* The ID of the last received packet we should use when replying */
    uint8_t                     last_received_id;

    uint8_t                     have_packet;      /* Flags that a packet is already created for TLS records */
    uint32_t                    start_time;

    besl_mac_t                  supplicant_mac_address;
    besl_mac_t                  authenticator_mac_address;
    uint8_t                     outer_eap_identity[32];
    uint8_t                     outer_eap_identity_length;

    wiced_tls_context_t*        tls_context;
    tls_agent_workspace_t       tls_agent;
    uint8_t                     tls_length_overhead;    /* This is a workaround flag for eap_tls_packet lenght parameter */
    uint8_t*                    buffer;    // XXX temporary until we review how the TLS engine is working with EAP transport
    uint32_t                    buffer_size;
    uint8_t*                    data_start;
    uint8_t*                    data_end;

    /* If type is peap */
    supplicant_peap_workspace_ptr_t peap;
} supplicant_workspace_t;

#ifdef __cplusplus
} /*extern "C" */
#endif
