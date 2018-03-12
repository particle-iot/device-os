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

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define P2P_MAX_DISCOVERED_DEVICES               (10)                        /* P2P devices */
#define P2P_MAX_DISCOVERED_LEGACY_DEVICES        (5)                         /* Non-P2P devices */
#define P2P_PUBLIC_ACTION_FRAME_CATEGORY_CODE    0x04                        /* P2P_PUB_AF_CATEGORY in dongle code */
#define P2P_ACTION_FRAME_CATEGORY_CODE           0x7f                        /* P2P_AF_CATEGORY */
#define P2P_PUBLIC_ACTION_FRAME_TYPE             0x09                        /* P2P_PUB_AF_ACTION */
#define P2P_OUI_SUB_TYPE                         0x09                        /* P2P_VER */
#define P2P_OUI                                  "\x50\x6F\x9A"              /* P2P_OUI which is the same as WFA OUI */
#define P2P_OUI_TYPE                             (9)                         /* P2P_OUI which differentiates it from the WFA OUI */
#define P2P_INVITATION_REINVOKE_PERSISTENT_GROUP 0x01                        /* P2P invitation flag for reinvoking a persistent group */
#define P2P_DEFRAGMENTATION_BUFFER_LENGTH        (256)                       /* P2P buffer length for defragmenting data spread across multiple P2P or WPS IEs */
#define P2P_SSID_PREFIX                          "DIRECT-"                   /* P2P SSID prefix */
#define P2P_SSID_PREFIX_LENGTH                   (sizeof(P2P_SSID_PREFIX)-1) /* P2P SSID prefix Length */
#define P2P_WILDCARD_SSID                        P2P_SSID_PREFIX             /* P2P Wildcard SSID */

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
    P2P_SEID_OP_CHANNEL                 = 17,  /* Operating channel */
    P2P_SEID_INVITATION_FLAGS           = 18,  /* Invitation flags */
} p2p_subelement_id_t;

/* P2P Status Codes from P2P Technical Spec 1.4, Table 8 */
typedef enum
{
    P2P_STATUS_SUCCESS                  = 0,   /* Success */
    P2P_STATUS_INFORMATION_UNAVAILABLE  = 1,   /* Fail; information is currently unavailable */
    P2P_STATUS_INCOMPATIBLE_PARAMETERS  = 2,   /* Fail; incompatible parameters */
    P2P_STATUS_LIMIT_REACHED            = 3,   /* Fail; limit reached */
    P2P_STATUS_INVALID_PARAMETERS       = 4,   /* Fail; invalid parameters */
    P2P_STATUS_UNABLE_TO_ACCOMMODATE    = 5,   /* Fail; unable to accommodate request */
    P2P_STATUS_PROTOCOL_ERROR           = 6,   /* Fail; previous protocol error, or disruptive behavior */
    P2P_STATUS_NO_COMMON_CHANNEL        = 7,   /* Fail; no common channels */
    P2P_STATUS_UNKNOWN_P2P_GROUP        = 8,   /* Fail; unknown P2P Group */
    P2P_STATUS_BOTH_DEVICES_MUST_BE_GO  = 9,   /* Fail: both P2P Devices indicated an Intent of 15 in Group Owner Negotiation */
    P2P_STATUS_INCOMPATIBLE_WPS_METHOD  = 10,  /* Fail; incompatible provisioning method */
    P2P_STATUS_REJECTED_BY_USER         = 11,  /* Fail: rejected by user */
    P2P_STATUS_ACCEPTED_BY_USER         = 12,  /* Success: Accepted by user */
} p2p_status_code_t;

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
    P2P_STATE_UNINITIALIZED,           /* P2P workspace is uninitialized */
    P2P_STATE_DISCOVERY,               /* Device is alternating between scanning and listening for other devices */
    P2P_STATE_NEGOTIATING,             /* Device has either sent or received a Negotiation Request to start a group */
    P2P_STATE_NEGOTIATION_COMPLETE,    /* Device has completed negotiation successfully and will become either a client or group owner */
    P2P_STATE_CONNECTION_WPS_ENROLLEE, /* Device completed negotiation successfully as a client and is waiting for WPS enrollee handshake to complete */
    P2P_STATE_ABORTED,                 /* P2P thread has been aborted by the application */
    P2P_STATE_GROUP_OWNER,             /* Device is group owner after successful negotiation or autonomous start */
    P2P_STATE_GROUP_CLIENT,            /* Device is client after successful negotiation and/or WPS and DHCP transactions */
    P2P_STATE_FAILURE,                 /* General failure due to memory allocation failure or other system failures */
} p2p_state_machine_state_t;

typedef enum
{
    P2P_EVENT_START_REQUESTED,
    P2P_EVENT_STOP_REQUESTED,
    P2P_EVENT_SCAN_COMPLETE,
    P2P_EVENT_DISCOVERY_COMPLETE,
    P2P_EVENT_PACKET_TO_BE_SENT,
    P2P_EVENT_NEGOTIATION_COMPLETE,
    P2P_EVENT_START_NEGOTIATION,
    P2P_EVENT_START_REGISTRAR,
    P2P_EVENT_TIMER_TIMEOUT,
    P2P_EVENT_FIND_GROUP_OWNER,
    P2P_EVENT_FOUND_TARGET_DEVICE,
    P2P_EVENT_DEVICE_AWAKE,
    P2P_EVENT_P2P_DEVICE_ASSOCIATED,
    P2P_EVENT_LEGACY_DEVICE_CONNECTION_REQUEST,
    P2P_EVENT_LEGACY_DEVICE_ASSOCIATED,
    P2P_EVENT_WPS_ENROLLEE_ASSOCIATED,
    P2P_EVENT_CONNECTION_REQUESTED,
    P2P_EVENT_DEVICE_DISASSOCIATED,
    P2P_EVENT_WPS_ENROLLEE_COMPLETED,
} p2p_event_type_t;

typedef enum
{
    P2P_DEVICE_INVALID    = 0,
    P2P_DEVICE_DISCOVERED = 1,
    P2P_DEVICE_REQUESTED_TO_FORM_GROUP = 2,
    P2P_DEVICE_NOT_READY,
    P2P_DEVICE_DECLINED_INVITATION,
    P2P_DEVICE_MUST_ALSO_BE_GROUP_OWNER,
    P2P_DEVICE_INCOMPATIBLE_WPS_METHOD,
    P2P_DEVICE_WAS_INVITED_TO_FORM_GROUP,
    P2P_DEVICE_ACCEPTED_GROUP_FORMATION,
    P2P_DEVICE_ASSOCIATED_CLIENT,
    P2P_DEVICE_INVITATION_REQUEST,
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

typedef enum
{
    P2P_NOTICE_OF_ABSENCE                 = 0,
    P2P_PRESENCE_REQUEST                  = 1,
    P2P_PRESENCE_RESPONSE                 = 2,
    P2P_GO_DISCOVERABILITY_REQUEST        = 3,
} p2p_action_frame_type_t;

typedef enum
{
    /* Device capabilities */
    P2P_DEVICE_CAPABILITY_SERVICE_DISCOVERY         = (1 << 0),
    P2P_DEVICE_CAPABILITY_CLIENT_DISCOVERABLITY     = (1 << 1),
    P2P_DEVICE_CAPABILITY_CONCURRENT_OPERATION      = (1 << 2),
    P2P_DEVICE_CAPABILITY_INFRASTRUCTURE_MANAGER    = (1 << 3),
    P2P_DEVICE_CAPABILITY_DEVICE_LIMIT              = (1 << 4),
    P2P_DEVICE_CAPABILITY_INVITATION_PROCEDURE      = (1 << 5),

    /* Group Capabilities */
    P2P_GROUP_CAPABILITY_P2P_GROUP_OWNER            = (1 << 8),
    P2P_GROUP_CAPABILITY_P2P_PERSISTENT_GROUP       = (1 << 9),
    P2P_GROUP_CAPABILITY_P2P_GROUP_LIMIT            = (1 << 10),
    P2P_GROUP_CAPABILITY_P2P_INTRA_BSS_DISTRIBUTION = (1 << 11),
    P2P_GROUP_CAPABILITY_P2P_CROSS_CONNECTION       = (1 << 12),
    P2P_GROUP_CAPABILITY_P2P_PERSISTENT_RECONNECT   = (1 << 13),
    P2P_GROUP_CAPABILITY_P2P_GROUP_FORMATION        = (1 << 14),
} p2p_capabilities_t;

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
