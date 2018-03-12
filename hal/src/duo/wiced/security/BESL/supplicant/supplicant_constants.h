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

#include "besl_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define EAP_TLS_FLAG_LENGTH_INCLUDED 0x80
#define EAP_TLS_FLAG_MORE_FRAGMENTS  0x40

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    TLS_AGENT_EVENT_EAPOL_PACKET,
    TLS_AGENT_EVENT_ABORT_REQUESTED,
} tls_agent_event_t;

/* High level states
 * INITIALISING ( scan, join )
 * EAP_HANDSHAKE ( go through EAP state machine )
 * WPS_HANDSHAKE ( go through WPS state machine )
 */
typedef enum
{
    SUPPLICANT_INITIALISING,
    SUPPLICANT_IN_EAP_METHOD_HANDSHAKE,
    SUPPLICANT_CLOSING_EAP,
} supplicant_main_stage_t;

typedef enum
{
    SUPPLICANT_EAP_START         = 0,    /* (EAP start ) */
    SUPPLICANT_EAP_IDENTITY      = 1,    /* (EAP identity request, EAP identity response) */
    SUPPLICANT_EAP_NAK           = 2,
    SUPPLICANT_EAP_METHOD        = 3,
} supplicant_state_machine_stage_t;


#ifdef __cplusplus
} /*extern "C" */
#endif
