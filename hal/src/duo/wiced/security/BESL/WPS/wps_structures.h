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
#include "wps_constants.h"
#include <stdint.h>
#include "tlv.h"
#include "crypto_constants.h"
#include "crypto_structures.h"
#include "wps_host.h"
#include "besl_host.h"
#include "besl_structures.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint8_t   wps_bool_t;

/******************************************************
 *                Packed Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
    uint8_t nonce[SIZE_128_BITS];
} wps_nonce_t;

typedef struct
{
    uint8_t octet[SIZE_256_BITS];
} wps_hash_t;

typedef struct
{
    uint8_t octect[SIZE_128_BITS]; /* Half of a hash */
} wps_psk_t;

typedef struct
{
    uint8_t octet[16];
} wps_uuid_t;

typedef struct
{
    uint8_t octet[SIZE_256_BITS];
} auth_key_t;

typedef struct
{
    uint8_t octet[SIZE_128_BITS];
} wps_iv_t;

typedef struct
{
    uint8_t octet[SIZE_128_BITS];
} key_wrap_key_t;

typedef struct
{
    uint8_t octet[SIZE_256_BITS];
} emsk_t;

typedef struct
{
    uint8_t key[SIZE_1536_BITS];
} public_key_t;

typedef struct
{
    uint8_t octet[SIZE_ETHERNET_ADDRESS];
} ethernet_address_t;

typedef struct
{
    wps_nonce_t         enrollee_nonce;
    ethernet_address_t  enrollee_mac;
    wps_nonce_t         registrar_nonce;
} kdk_input_t;

typedef struct
{
    auth_key_t     auth_key;
    key_wrap_key_t key_wrap_key;
    emsk_t         emsk;
} session_keys_t;

typedef union
{
    session_keys_t keys;
    uint8_t        data[WPS_PADDED_AES_ROUND_UP( sizeof(session_keys_t), SIZE_256_BITS )];
} session_key_derivation_output_t;

typedef struct
{
    uint8_t iv[SIZE_128_BITS];
    uint8_t data[1];
} encryption_data_t;

typedef struct
{
    uint8_t vendor_id[3];
    uint8_t data[1];
} general_vendor_ext_t;

typedef struct
{
    uint8_t      vendor_id[3];
    tlv8_uint8_t subid_version2;
} vendor_ext_t;

typedef struct
{
    uint8_t      vendor_id[3];
    tlv8_uint8_t subid_version2;
    tlv8_uint8_t request_to_enroll;
} m1_vendor_ext_t;

typedef struct
{
    wps_nonce_t  secret;  /* This will be es1/rs1 or es2/rs2 */
    wps_psk_t    psk;     /* This will be  psk1   or  psk2 */
    public_key_t enrollee_public_key;
    public_key_t registrar_public_key;
} wps_hash_input_t;

typedef struct
{
    uint16_t category;
    uint32_t oui;
    uint16_t sub_category;
} primary_device_type_t;

typedef struct
{
    ether_header_t        ethernet;
    eapol_header_t        eapol;
    eap_header_t          eap;
    eap_expanded_header_t eap_expanded;
    uint8_t               data[1];
} wps_msg_packet_t;

typedef struct
{
    ether_header_t        ethernet;
    eapol_header_t        eapol;
    eap_header_t          eap;
    eap_expanded_header_t eap_expanded;
} wps_msg_packet_header_t;

#pragma pack()


/******************************************************
 *                Unpacked Structures
 ******************************************************/
/*
 * This structure contains data that is unique for both agent but contained by both.
 * Each agent starts with their public_key and nonce and progressively generates the secret nonces and hashes while also
 * learning the other agent's public key, nonces and hashes.
 *
 * psk : Generated from the password
 */
typedef struct
{
    public_key_t public_key;
    wps_nonce_t  nonce;
    wps_nonce_t  secret_nonce[2];
    wps_hash_t   secret_hash[2];
    besl_mac_t   mac_address;
    uint8_t      supported_version;
    uint16_t     authTypeFlags;
    uint16_t     encrTypeFlags;
} wps_agent_data_t;


typedef struct
{
        wiced_scan_result_t scan_result;
        wps_uuid_t          uuid;
} wps_ap_t;

typedef struct _wps_agent_t wps_agent_t;

typedef wps_result_t (*wps_event_handler_t)(wps_agent_t* workspace, besl_event_message_t* message);

struct _wps_agent_t
{
    /* Persistent variables between WPS attempts:
     * - device_details
     * - wps_mode
     * - agent_type
     * - wps_host_workspace
     * - password
     * - interface
     */
    const besl_wps_device_detail_t* device_details;
    wps_mode_t       wps_mode;
    wps_agent_type_t agent_type;
    void*            wps_host_workspace;
    wwd_interface_t  interface;

    /* Variables that need to be refactored */
    uint8_t   connTypeFlags;
    uint8_t   rfBand;
    uint8_t   scState;
    uint16_t  association_state;
    uint32_t  osVersion;
    wps_uuid_t    uuid;
    primary_device_type_t primary_device;

    /* WPS cryptography data for both WPS participants */
    wps_agent_data_t my_data;
    wps_agent_data_t their_data;

    /* Pointers to allow a single structure type be used for both enrollee and registrar */
    wps_agent_data_t* registrar_data;
    wps_agent_data_t* enrollee_data;

    /* Natural Number versions of keys used for quick calculations */
    wps_NN_t my_private_key;

    /* Password and derived PSK */
    const char* password;
    uint16_t    device_password_id;
    wps_psk_t   psk[2];

    wps_result_t wps_result;

    /* Session keys */
    auth_key_t     auth_key;
    key_wrap_key_t key_wrap_key;
    emsk_t         emsk;

    /* Progressive HMAC workspace */
    sha2_context hmac;

    /* State machine stages */
    wps_main_stage_t current_main_stage;
    uint8_t          current_sub_stage; /* Either a value from wps_eap_state_machine_stage_t or wps_state_machine_stage_t */
    uint8_t          retry_counter;

    uint32_t available_crypto_material;

    /* The ID of the last received packet we should use when replying */
    uint8_t last_received_id;

    /* Fragment packet processing flag */
    uint8_t  processing_fragmented_packet;
    uint8_t* fragmented_packet;
    uint16_t fragmented_packet_length;
    uint16_t fragmented_packet_length_max;

    /* Event handler for all events that occur during the INITIALIZING and IN_EAP_HANDSHAKE stages */
    wps_event_handler_t event_handler;

    /* Pointer to the owner of the WPS agent, for example a P2P group owner workspace */
    void* wps_agent_owner;

    /* IE elements for both Enrollee and Registrar */
    union
    {
        struct
        {
            besl_ie_t probe_request;
            besl_ie_t association_request;
        } enrollee;
        struct
        {
            besl_ie_t probe_response;
            besl_ie_t association_response;
            besl_ie_t beacon;
        } registrar;
        besl_ie_t common[3];
    } ie;

    uint8_t  in_reverse_registrar_mode;

    uint32_t start_time;

    /*
     * Enrollee only variables
     */
    /* Current target AP */
    wps_ap_t*         ap;
    uint32_t          directed_wps_max_attempts;
    uint8_t           ap_join_attempts;
    uint8_t           identity_request_received_count;
    wiced_band_list_t band_list;

    /* Copy of M1 to be used for hashing when we receive M2 */
    uint8_t* m1_copy;
    uint16_t m1_copy_length;

    /* P2P related variables */
    uint8_t is_p2p_enrollee;
    uint8_t is_p2p_registrar;

    void (*wps_result_callback)         (wps_result_t*); /*!< WPS result callback for applications */
    void (*wps_internal_result_callback)(wps_result_t*); /*!< WPS internal result callback for use by P2P and other BESL features */
};

#ifdef __cplusplus
} /*extern "C" */
#endif
