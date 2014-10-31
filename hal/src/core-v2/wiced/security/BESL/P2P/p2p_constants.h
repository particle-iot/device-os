/*
 * Copyright 2014, Broadcom Corporation
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

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/


/* P2P IE Subelement IDs from WiFi P2P Technical Spec 1.00 */
typedef enum
{
    P2P_SEID_STATUS                     = 0,   /* Status */
    P2P_SEID_MINOR_REASON_CODE          = 1,   /* Minor Reason Code */
    P2P_SEID_P2P_CAPABILITY_INFO        = 2,   /* P2P Capability (capabilities info) */
    P2P_SEID_DEVICE_ID                  = 3,   /* P2P Device ID */
    P2P_SEID_GROUP_OWNER_INTENT         = 4,   /* Group Owner Intent */
    P2P_SEID_CONFIGURATION_TIMEOUT      = 5,   /* Configuration Timeout */
    P2P_SEID_CHANNEL                    = 6,   /* Channel */
    P2P_SEID_GROUP_BSSID                = 7,   /* P2P Group BSSID */
    P2P_SEID_EXTENDED_LISTEN_TIMING     = 8,   /* Extended Listen Timing */
    P2P_SEID_INTENDED_INTERFACE_ADDRESS = 9,   /* Intended P2P Interface Address */
    P2P_SEID_P2P_MANAGEABILITY          = 10,  /* P2P Manageability */
    P2P_SEID_CHANNEL_LIST               = 11,  /* Channel List */
    P2P_SEID_NOTICE_OF_ABSENCE          = 12,  /* Notice of Absence */
    P2P_SEID_DEVICE_INFO                = 13,  /* Device Info */
    P2P_SEID_GROUP_INFO                 = 14,  /* Group Info */
    P2P_SEID_GROUP_ID                   = 15,  /* Group ID */
    P2P_SEID_P2P_INTERFACE              = 16,  /* P2P Interface */
    P2P_SEID_OP_CHANNEL                 = 17,
    P2P_SEID_INVITE_FLAGS               = 18,
} p2p_subelement_id_t;

typedef enum
{
    P2P_CLIENT_MODE      = 0,
    P2P_GROUP_OWNER_MODE = 1,
} p2p_mode_t;

typedef enum
{
    P2P_DISCOVERY_STATE_SCAN    = 0,
    P2P_DISCOVERY_STATE_LISTEN  = 1,
    P2P_DISCOVERY_STATE_SEARCH  = 2,
} p2p_discovery_state_t;


typedef enum
{
    P2P_STATE_DISCOVERING,
    P2P_STATE_NEGOTIATING,
    P2P_STATE_SCANNING,
    P2P_STATE_COMPLETE,
    P2P_STATE_ABORTED,
} p2p_state_machine_state_t;

typedef enum
{
    P2P_EVENT_START_REQUESTED,
    P2P_EVENT_STOP_REQUESTED,
    P2P_EVENT_SCAN_COMPLETE,
    P2P_EVENT_DISCOVERY_COMPLETE,
    P2P_EVENT_PACKET_TO_BE_SENT,
    P2P_EVENT_NEGOTIATION_COMPLETE,
    P2P_EVENT_NEW_DEVICE_DISCOVERED,
} p2p_event_type_t;

typedef enum
{
    P2P_DEVICE_INVALID,
    P2P_DEVICE_DISCOVERED,
    P2P_DEVICE_REQUESTED,
    P2P_DEVICE_NOT_READY,
    P2P_DEVICE_DECLINED,
    P2P_DEVICE_ACCEPTED,
} p2p_device_status_t;

typedef enum
{
     P2P_GO_NEGOTIATION_REQUEST           = 0,
     P2P_GO_NEGOTIATION_RESPONSE          = 1,
     P2P_GO_NEGOTIATION_CONFIRMATION      = 2,
     P2P_INVITATION_REQUEST               = 3,
     P2P_INVITATION_RESPONSE              = 4,
     P2P_DEVICE_DISCOVERABILITY_REQUEST   = 5,
     P2P_DEVICE_DISCOVERABILITY_RESPONSE  = 6,
     P2P_PROVISION_DISCOVERY_REQUEST      = 7,
     P2P_PROVISION_DISCOVERY_RESPONSE     = 8,
} p2p_public_action_frame_type_t;

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

#ifdef __cplusplus
} /*extern "C" */
#endif
