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

#include "tlv.h"
#include "besl_host_interface.h"
#include "p2p_constants.h"
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

#define P2P_MAX_NUMBER_OF_DEVICES    (10)
#define ETHERNET_ADDRESS_LENGTH      (6)
#define P2P_MAX_SUFFIX_LENGTH        (16)

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
typedef struct
{
    besl_mac_t  mac_address;
    p2p_mode_t  interface_type;  /* see i/f type */
    uint16_t    chan_spec;       /* for p2p_ifadd GO */
} wl_p2p_if_t;

typedef struct
{
    uint8_t  state; /* see p2p_discovery_state_t */
    uint16_t chanspec; /* valid in listen state */
    uint16_t dwell_time_ms; /* valid in listen state, in ms */
} wl_p2p_disc_st_t;

/* scan request */
typedef struct
{
    uint8_t type; /* 'S' for WLC_SCAN, 'E' for "escan" */
    uint8_t reserved[3];

    /* escan params */
    wl_escan_params_t escan;
} wl_p2p_scan_t;

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
    uint8_t   channel_list[3];
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
    besl_mac_t            mac_address;
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
    p2p_device_id_tlv_t device_info;
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
    besl_mac_t          mac;
    uint16_t            channel;
    p2p_device_status_t status;
    uint8_t             dialog_token;
    uint8_t             group_owner_intent;
    uint16_t            preferred_config_methods;
    char                device_name[32];
} p2p_discovered_device_t;

typedef struct
{
    besl_mac_t   bssid;
    uint32_t     group_id;
    uint8_t      channel;
    wiced_ssid_t ssid;
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
    uint8_t sub_type;
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
    uint8_t                  group_owner_intent;
    const char*              ap_ssid_suffix;
    const char*              device_name;
} besl_p2p_device_detail_t;

typedef struct
{
    p2p_channel_info_t      channel_info;
    p2p_channel_list_info_t channel_list;
    p2p_device_info_t       device_info;
    char                    device_name[32];
    uint8_t                 device_name_length;
    uint8_t                 group_owner_tie_breaker;

    const besl_wps_device_detail_t* wps_device_details;

    uint32_t        p2p_action_frame_counter;
    uint32_t        p2p_interface;
    p2p_state_machine_state_t p2p_current_state;
    p2p_discovered_device_t   discovered_devices[P2P_MAX_NUMBER_OF_DEVICES];
    uint8_t                   discovered_device_count;

    p2p_group_details_t group_candidate;
    uint8_t             i_am_group_owner;
    char                p2p_ap_suffix[P2P_MAX_SUFFIX_LENGTH];
    const char*         p2p_name;

    uint16_t p2p_capability;
    uint8_t  group_owner_intent;

    besl_mac_t intended_mac_address;

    besl_result_t p2p_result;

    uint8_t* p2p_probe_request_ie;
    uint32_t p2p_probe_request_ie_length;
    uint8_t* p2p_association_request_ie;
    uint32_t p2p_association_request_ie_length;
    uint8_t* p2p_probe_response_ie;
    uint32_t p2p_probe_response_ie_length;
    p2p_beacon_ie_t* p2p_beacon_ie;
    uint32_t p2p_beacon_ie_length;

    /* This IE applies to both request and response */
    uint8_t* wps_probe_ie;
    uint32_t wps_probe_ie_length;
} p2p_workspace_t;

typedef uint8_t* (*p2p_action_frame_writer_t)(p2p_workspace_t* workspace, p2p_discovered_device_t* device, void* buffer);

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
