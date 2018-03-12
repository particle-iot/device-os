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

#include "tlv.h"
#include "besl_host_interface.h"
#include "p2p_constants.h"
#include "wps_structures.h"
#include "platform_dct.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define ETHERNET_ADDRESS_LENGTH        (6)
#define P2P_MAX_SUFFIX_LENGTH         (16)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/* i/f request */

//typedef struct
//{
//    wl_ether_addr_t   da;
//    uint16_t          len;
//    uint32_t          packetId;
//    uint8_t           data[ACTION_FRAME_SIZE];
//} wl_action_frame_t;

typedef struct
{
    uint32_t          channel;
    int32_t           dwell_time;
    wl_ether_addr_t   BSSID;
    wl_action_frame_t action_frame;
} wl_af_params_t;

#pragma pack(1)

typedef struct
{
    uint8_t type;
    uint8_t flags;
    uint16_t duration;
    uint8_t address1[ETHERNET_ADDRESS_LENGTH];
    uint8_t address2[ETHERNET_ADDRESS_LENGTH];
    uint8_t address3[ETHERNET_ADDRESS_LENGTH];
    uint16_t ether_type;
} ieee80211_header_t;

typedef struct
{
    uint8_t  type;
    uint16_t length;
} p2p_tlv_header_t;

typedef struct
{
    uint8_t  type;
    uint16_t length;
    uint8_t  data[1];
} p2p_tlv_data_t;

typedef struct
{
    uint8_t   type;    // 2
    uint16_t  length;  // 2
    uint8_t   device_capability;
    uint8_t   group_capability;
} p2p_capability_tlv_t;

typedef struct
{
    uint8_t    type;    // 3
    uint16_t   length;  // 6
    besl_mac_t mac_address;
} p2p_device_id_tlv_t;

typedef struct
{
    uint8_t   type;   // 4
    uint16_t  length; // 1
    uint8_t   intent;
} p2p_group_owner_intent_tlv_t;

typedef struct
{
    uint8_t   type;   // 5
    uint16_t  length; // 2
    uint8_t   go_configuration_timeout;
    uint8_t   client_configuration_timeout;
} p2p_configuration_timeout_tlv_t;

typedef struct
{
    uint8_t   type;   // 6
    uint16_t  length; // 5
    uint8_t   country_string[3];
    uint8_t   operating_class;
    uint8_t   channel;
} p2p_listen_channel_tlv_t;

typedef struct
{
    uint8_t   country_string[3];
    uint8_t   operating_class;
    uint8_t   channel;
} p2p_channel_info_t;

typedef struct
{
    uint8_t   country_string[3];
    uint8_t   operating_class;
    uint8_t   number_of_channels; // 3
    uint8_t   channel_list[13];
} p2p_channel_list_info_t;

typedef struct
{
    uint8_t   type;   // 9
    uint16_t  length; // 6
    uint8_t   mac_address[6];
} p2p_intended_interface_tlv_t;

typedef struct
{
    uint8_t   type;   // 0
    uint16_t  length; // 1
    uint8_t   status;
} p2p_status_tlv_t;

typedef struct
{
    uint8_t   type;   // 11
    uint16_t  length;
    uint8_t   country_string[3];
    uint8_t   operating_class;
    uint8_t   number_of_channels;
    // List of channels
} p2p_channel_list_tlv_t;

typedef struct
{
    uint8_t   type;   // 17
    uint16_t  length; // 5
    uint8_t   country_string[3];
    uint8_t   operating_class;
    uint8_t   channel;
} p2p_operating_channel_tlv_t;

typedef struct
{
    uint8_t   type;   // 15
    uint16_t  length;
    besl_mac_t mac_address;
    // SSID of length 0 - 32
} p2p_group_id_tlv_t;

typedef struct
{
    uint8_t   index;
    uint8_t   ct_window;
    // Notice of Absence Descriptors go here
} p2p_notice_of_absence_t;

typedef struct
{
    uint8_t               type;   // 13
    uint16_t              length;
    besl_mac_t            mac_address;
    uint16_t              config_methods;
    primary_device_type_t primary_type;
    uint8_t               number_of_secondary_devices; // 0
    tlv16_header_t        device_name;
    // device name goes here
} p2p_device_info_tlv_t;

typedef struct
{
    besl_mac_t            p2p_device_address;
    uint16_t              config_methods;
    primary_device_type_t primary_type;
    uint8_t               number_of_secondary_devices; // 0
//    tlv16_header_t        device_name;
//    char                  device_name_data[];
} p2p_device_info_t;

typedef struct
{
    p2p_capability_tlv_t     capability;
    p2p_listen_channel_tlv_t listen_channel;
} p2p_probe_request_ie_t;

typedef struct
{
    p2p_capability_tlv_t  capability;
    p2p_device_info_tlv_t device_info;
    /* device name goes here */
} p2p_probe_response_ie_t;

typedef struct
{
    p2p_capability_tlv_t  capability;
    p2p_device_id_tlv_t   device_id;
} p2p_beacon_ie_t;

typedef struct
{
    tlv16_uint8_t         version;
    tlv16_uint8_t         request_type;
    tlv16_uint16_t        config_methods;
    tlv16_header_t        uuid;
    wps_uuid_t            uuid_data;
    tlv16_header_t        primary_device;
    primary_device_type_t primary_device_data;
    tlv16_uint8_t         rf_band;
    tlv16_uint16_t        assoc_state;
    tlv16_uint16_t        config_error;
    tlv16_uint16_t        pwd_id;
    tlv16_header_t        manufacturer;
    tlv16_header_t        model_name;
    tlv16_header_t        model_number;
    tlv16_header_t        device_name;
    tlv16_header_t        vendor_ext;
    m1_vendor_ext_t       vendor_ext_data;
} p2p_wps2_probe_request_ie_t;

typedef struct
{
    p2p_capability_tlv_t         capability;
    p2p_group_owner_intent_tlv_t group_owner_intent;
    p2p_listen_channel_tlv_t     listen_channel;
    p2p_intended_interface_tlv_t intended_interface;
    p2p_channel_list_tlv_t       channel_list;
    p2p_device_info_tlv_t        device_info;
    p2p_operating_channel_tlv_t  operating_channel;

    tlv16_uint8_t         version;
    tlv16_uint16_t        device_password_id;
} p2p_go_request_t;

typedef struct
{
    besl_mac_t                 p2p_device_address;
    besl_mac_t                 p2p_interface_address;
    besl_mac_t                 group_owner_device_address; // For invitation procedure support
    uint16_t                   listen_channel; // XXX should be channel info type ?
    p2p_channel_info_t         operating_channel;
    p2p_channel_list_info_t    channel_list;
    p2p_device_status_t        status;
    uint8_t                    dialog_token;
    uint8_t                    group_owner_intent;
    uint8_t                    tie_breaker;
    uint8_t                    go_configuration_timeout; // in units of 10 milliseconds
    uint8_t                    client_configuration_timeout;
    uint8_t                    device_capability;
    uint8_t                    group_owner_capability;
    uint8_t                    invitation_flags;
    uint16_t                   preferred_config_method;
    besl_time_t                last_seen_time; // Last time a probe request was seen
    wps_mode_t                 p2p_wps_mode;
    wps_device_password_id_t   p2p_wps_device_password_id;
    char                       device_name[32]; // XXX do we handle max length fields properly
    char                       ssid[32];
} p2p_discovered_device_t;

typedef struct
{
    besl_mac_t               mac_address;
    besl_time_t              last_alert_time; // Last time an alert was sent to the application
    wps_device_password_id_t wps_device_password_id;
    uint8_t                  in_use;
    char                     device_name[32]; // XXX do we handle max length fields properly
} p2p_legacy_device_t;

typedef struct
{
    uint8_t               length;
    besl_mac_t            p2p_device_address;
    besl_mac_t            p2p_interface_address;
    uint8_t               device_capability;
    uint16_t              config_methods;
    primary_device_type_t primary_type;
    uint8_t               number_of_secondary_devices; // 0
    tlv16_header_t        device_name;
    char                  device_name_data[32];
} p2p_client_info_descriptor_t;

typedef struct
{
    uint8_t               type;   // 14
    uint16_t              length;
    // client descriptors go here
} p2p_group_info_t;

typedef struct
{
    besl_mac_t         bssid; // p2p interface address
    besl_mac_t         p2p_device_address;
    p2p_channel_info_t operating_channel;
    uint8_t            configuration_timeout;
    uint16_t           p2p_capability;
    uint8_t            ssid_length;
    uint8_t            ssid[32];
} p2p_group_details_t;

typedef struct
{
    uint8_t tag_number; // 0xdd
    uint8_t length;
    uint8_t oui[3]; // 50-6f-9a
    uint8_t specific_oui_type; // 9
} p2p_vendor_specific_tag_header_t;

typedef struct
{
    uint8_t tag_number; // 0xdd
    uint8_t length;
    uint8_t oui[3]; // 00-50-f2
    uint8_t specific_oui_type; // 4
} wps_vendor_specific_tag_header_t;

typedef struct
{
    // Fixed parameters
    uint8_t category_code;
    uint8_t public_action;
    uint8_t oui[3];
    uint8_t oui_sub_type;
    uint8_t p2p_sub_type;
    uint8_t p2p_dialog_token;
} p2p_public_action_frame_fixed_parameter_t;

typedef struct
{
    p2p_public_action_frame_fixed_parameter_t fixed;
    uint8_t                            data[1];
} p2p_public_action_frame_message_t;

typedef struct
{
    // Fixed parameters
    uint8_t category_code;
    uint8_t oui[3];
    uint8_t oui_sub_type;
    uint8_t p2p_sub_type;
    uint8_t p2p_dialog_token;
} p2p_action_frame_fixed_parameter_t;

typedef struct
{
    p2p_action_frame_fixed_parameter_t fixed;
    uint8_t                            data[1];
} p2p_action_frame_message_t;

#pragma pack()

typedef struct
{
    uint32_t type;
    void*    data;
} p2p_message_t;

typedef struct
{
    besl_wps_device_detail_t wps_device_details;
    p2p_channel_info_t       listen_channel;
    p2p_channel_info_t       operating_channel;
    p2p_channel_list_info_t  channel_list;
    uint8_t                  group_owner_intent;
    uint8_t                  go_configuration_timeout; // In units of 10 milliseconds
    uint8_t                  client_configuration_timeout;
    uint16_t                 device_password_id;
    besl_time_t              peer_device_timeout;      // For timing devices out of the p2p peer list
    besl_time_t              group_formation_timeout;  // For timing out a group formation attempt with an existing group owner or using negotiation
    uint16_t                 p2p_capability;
    const char*              ap_ssid_suffix;
} besl_p2p_device_detail_t;

typedef struct
{
    p2p_channel_info_t              listen_channel;
    p2p_channel_info_t              operating_channel;
    p2p_channel_list_info_t         channel_list;
    uint32_t                        current_channel;
    p2p_device_info_t               device_info;
    char                            device_name[32];
    uint8_t                         device_name_length;
    uint8_t                         group_owner_tie_breaker;
    uint16_t                        configuration_timeout;
    uint32_t                        p2p_action_frame_cookie;
    uint32_t                        p2p_interface;               /* Wiced host interface for use when bringing up IP stack etc */
    p2p_state_machine_state_t       p2p_current_state;
    p2p_discovered_device_t         discovered_devices[P2P_MAX_DISCOVERED_DEVICES];
    p2p_discovered_device_t*        candidate_device;
    p2p_legacy_device_t             legacy_devices[P2P_MAX_DISCOVERED_LEGACY_DEVICES];
    uint8_t                         discovered_device_count;
    p2p_client_info_descriptor_t    associated_p2p_devices[P2P_MAX_ASSOCIATED_DEVICES];
    uint8_t                         associated_p2p_device_count; // Count of P2P devices only, does not include legacy devices
    besl_mac_t                      associated_legacy_devices[P2P_MAX_ASSOCIATED_DEVICES];
    besl_time_t                     peer_device_timeout;
    besl_time_t                     group_formation_timeout;
    besl_time_t                     group_formation_start_time;
    uint8_t                         scan_aborted;
    uint8_t                         scan_in_progress;

    besl_time_t                     timer_reference;
    uint32_t                        timer_timeout;

    p2p_group_details_t             group_candidate;
    p2p_group_details_t             invitation_candidate;
    uint8_t                         invitation_flags;
    uint8_t                         i_am_group_owner;  // Need this for the negotiation phase
    uint8_t                         group_owner_is_up; // Need this when the GO is running because the addressing requirements of public action frames and for deinit
    uint8_t                         group_client_is_up; // Need this for deinit
    uint8_t                         initiate_negotiation;
    uint8_t                         looking_for_group_owner;
    uint8_t                         ok_to_accept_negotiation;
    uint8_t                         p2p_initialised;
    uint8_t                         p2p_thread_running;

    uint8_t                         sent_go_discoverability_request;
    uint8_t                         sent_negotiation_request;
    uint8_t                         sent_negotiation_confirm;
    uint8_t                         sent_invitation_request;
    uint8_t                         sent_provision_discovery_request;
    uint8_t                         discovery_dialog_token;
    p2p_client_info_descriptor_t*   discovery_target;
    besl_mac_t                      discovery_requestor;

    besl_mac_t                      original_mac_address;
    besl_mac_t                      p2p_interface_address;  // Intended MAC address
    besl_mac_t                      p2p_device_address;
    char                            p2p_ap_suffix[P2P_MAX_SUFFIX_LENGTH];
    uint8_t                         p2p_passphrase[64];
    uint16_t                        p2p_passphrase_length;

    uint16_t                        provisioning_config_method;
    uint16_t                        allowed_configuration_methods;
    char                            p2p_wps_pin[9];
    const besl_wps_device_detail_t* wps_device_details;
    uint16_t                        p2p_wps_device_password_id;
    wps_mode_t                      p2p_wps_mode;
    besl_wps_credential_t           p2p_wps_credential;
    wps_agent_t*                    p2p_wps_agent;

    uint16_t                        p2p_capability;
    uint8_t                         group_owner_intent;

    /* Persistent Group support */
    uint8_t                         form_persistent_group;
    uint8_t                         reinvoking_group;
    wiced_config_soft_ap_t          persistent_group;

    /* P2P client WPS credential and group owner storage */
    besl_wps_credential_t           wps_credential;
    wps_ap_t                        group_owner;

    besl_result_t p2p_result;

    /* Defragmentation for WPS and P2P IEs */
    uint8_t                         p2p_data[P2P_DEFRAGMENTATION_BUFFER_LENGTH];
    uint8_t                         wps_data[P2P_DEFRAGMENTATION_BUFFER_LENGTH];
    uint32_t                        p2p_data_length;
    uint32_t                        wps_data_length;

    /* P2P IEs */
    uint8_t* p2p_probe_request_ie;
    uint32_t p2p_probe_request_ie_length;
    uint8_t* p2p_association_request_ie;
    uint32_t p2p_association_request_ie_length;
    uint8_t* p2p_probe_response_ie;
    uint32_t p2p_probe_response_ie_length;
    uint8_t* p2p_beacon_ie;
    uint32_t p2p_beacon_ie_length;

    /* WPS IEs */
    uint8_t* wps_probe_request_ie;
    uint32_t wps_probe_request_ie_length;
    uint8_t* wps_probe_response_ie;
    uint32_t wps_probe_response_ie_length;

    /* MAC address storage for use with relevant call backs */
    besl_mac_t wpa2_client_mac;
    besl_mac_t wps_enrollee_mac;
    besl_mac_t disassociating_client_mac;

    void (*p2p_connection_request_callback)              (p2p_discovered_device_t*); /*!< Connection request callback for P2P devices */
    void (*p2p_legacy_device_connection_request_callback)(p2p_legacy_device_t*);     /*!< Connection request callback for legacy non-P2P devices */
    void (*p2p_wpa2_client_association_callback)         (besl_mac_t*);              /*!< WPA2 association callback for all client devices */
    void (*p2p_wps_enrollee_association_callback)        (besl_mac_t*);              /*!< WPS enrollee association callback for all client devices */
    void (*p2p_group_formation_result_callback)          (void*);                    /*!< Group formation result callback */
    void (*p2p_wps_result_callback)                      (wps_result_t*);            /*!< WPS enrollee association callback for all client devices */
    void (*p2p_device_disassociation_callback)           (besl_mac_t*);              /*!< P2P device disassociation callback */
    void (*p2p_legacy_device_disassociation_callback)    (besl_mac_t*);              /*!< Legacy device disassociation callback */
} p2p_workspace_t;

typedef uint8_t* (*p2p_action_frame_writer_t)(p2p_workspace_t* workspace, const p2p_discovered_device_t* device, void* buffer);

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif

