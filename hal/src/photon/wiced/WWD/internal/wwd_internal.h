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

#ifndef INCLUDED_WWD_INTERNAL_H
#define INCLUDED_WWD_INTERNAL_H

#include <stdint.h>
#include "wwd_constants.h" /* for wwd_result_t */
#include "network/wwd_network_interface.h"
#include "tlv.h"
#include "wwd_eapol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

typedef enum
{
    /* Note : If changing this, core_base_address must be changed also */
    WLAN_ARM_CORE = 0,
    SOCRAM_CORE   = 1,
    SDIOD_CORE    = 2
} device_core_t;

typedef enum
{
    WLAN_DOWN,
    WLAN_UP
} wlan_state_t;

typedef enum
{
    WLAN_CORE_FLAG_NONE,
    WLAN_CORE_FLAG_CPU_HALT,
} wlan_core_flag_t;

/* 802.11 Information Element Identification Numbers (as per section 8.4.2.1 of 802.11-2012) */
typedef enum
{
    DOT11_IE_ID_SSID                                 = 0,
    DOT11_IE_ID_SUPPORTED_RATES                      = 1,
    DOT11_IE_ID_FH_PARAMETER_SET                     = 2,
    DOT11_IE_ID_DSSS_PARAMETER_SET                   = 3,
    DOT11_IE_ID_CF_PARAMETER_SET                     = 4,
    DOT11_IE_ID_TIM                                  = 5,
    DOT11_IE_ID_IBSS_PARAMETER_SET                   = 6,
    DOT11_IE_ID_COUNTRY                              = 7,
    DOT11_IE_ID_HOPPING_PATTERN_PARAMETERS           = 8,
    DOT11_IE_ID_HOPPING_PATTERN_TABLE                = 9,
    DOT11_IE_ID_REQUEST                              = 10,
    DOT11_IE_ID_BSS_LOAD                             = 11,
    DOT11_IE_ID_EDCA_PARAMETER_SET                   = 12,
    DOT11_IE_ID_TSPEC                                = 13,
    DOT11_IE_ID_TCLAS                                = 14,
    DOT11_IE_ID_SCHEDULE                             = 15,
    DOT11_IE_ID_CHALLENGE_TEXT                       = 16,
    /* 17-31 Reserved */
    DOT11_IE_ID_POWER_CONSTRAINT                     = 32,
    DOT11_IE_ID_POWER_CAPABILITY                     = 33,
    DOT11_IE_ID_TPC_REQUEST                          = 34,
    DOT11_IE_ID_TPC_REPORT                           = 35,
    DOT11_IE_ID_SUPPORTED_CHANNELS                   = 36,
    DOT11_IE_ID_CHANNEL_SWITCH_ANNOUNCEMENT          = 37,
    DOT11_IE_ID_MEASUREMENT_REQUEST                  = 38,
    DOT11_IE_ID_MEASUREMENT_REPORT                   = 39,
    DOT11_IE_ID_QUIET                                = 40,
    DOT11_IE_ID_IBSS_DFS                             = 41,
    DOT11_IE_ID_ERP                                  = 42,
    DOT11_IE_ID_TS_DELAY                             = 43,
    DOT11_IE_ID_TCLAS_PROCESSING                     = 44,
    DOT11_IE_ID_HT_CAPABILITIES                      = 45,
    DOT11_IE_ID_QOS_CAPABILITY                       = 46,
    /* 47 Reserved */
    DOT11_IE_ID_RSN                                  = 48,
    /* 49 Reserved */
    DOT11_IE_ID_EXTENDED_SUPPORTED_RATES             = 50,
    DOT11_IE_ID_AP_CHANNEL_REPORT                    = 51,
    DOT11_IE_ID_NEIGHBOR_REPORT                      = 52,
    DOT11_IE_ID_RCPI                                 = 53,
    DOT11_IE_ID_MOBILITY_DOMAIN                      = 54,
    DOT11_IE_ID_FAST_BSS_TRANSITION                  = 55,
    DOT11_IE_ID_TIMEOUT_INTERVAL                     = 56,
    DOT11_IE_ID_RIC_DATA                             = 57,
    DOT11_IE_ID_DSE_REGISTERED_LOCATION              = 58,
    DOT11_IE_ID_SUPPORTED_OPERATING_CLASSES          = 59,
    DOT11_IE_ID_EXTENDED_CHANNEL_SWITCH_ANNOUNCEMENT = 60,
    DOT11_IE_ID_HT_OPERATION                         = 61,
    DOT11_IE_ID_SECONDARY_CHANNEL_OFFSET             = 62,
    DOT11_IE_ID_BSS_AVERAGE_ACCESS_DELAY             = 63,
    DOT11_IE_ID_ANTENNA                              = 64,
    DOT11_IE_ID_RSNI                                 = 65,
    DOT11_IE_ID_MEASUREMENT_PILOT_TRANSMISSION       = 66,
    DOT11_IE_ID_BSS_AVAILABLE_ADMISSION_CAPACITY     = 67,
    DOT11_IE_ID_BSS_AC_ACCESS_DELAY                  = 68,
    DOT11_IE_ID_TIME_ADVERTISEMENT                   = 69,
    DOT11_IE_ID_RM_ENABLED_CAPABILITIES              = 70,
    DOT11_IE_ID_MULTIPLE_BSSID                       = 71,
    DOT11_IE_ID_20_40_BSS_COEXISTENCE                = 72,
    DOT11_IE_ID_20_40_BSS_INTOLERANT_CHANNEL_REPORT  = 73,
    DOT11_IE_ID_OVERLAPPING_BSS_SCAN_PARAMETERS      = 74,
    DOT11_IE_ID_RIC_DESCRIPTOR                       = 75,
    DOT11_IE_ID_MANAGEMENT_MIC                       = 76,
    DOT11_IE_ID_EVENT_REQUEST                        = 78,
    DOT11_IE_ID_EVENT_REPORT                         = 79,
    DOT11_IE_ID_DIAGNOSTIC_REQUEST                   = 80,
    DOT11_IE_ID_DIAGNOSTIC_REPORT                    = 81,
    DOT11_IE_ID_LOCATION_PARAMETERS                  = 82,
    DOT11_IE_ID_NONTRANSMITTED_BSSID_CAPABILITY      = 83,
    DOT11_IE_ID_SSID_LIST                            = 84,
    DOT11_IE_ID_MULTIPLE_BSSID_INDEX                 = 85,
    DOT11_IE_ID_FMS_DESCRIPTOR                       = 86,
    DOT11_IE_ID_FMS_REQUEST                          = 87,
    DOT11_IE_ID_FMS_RESPONSE                         = 88,
    DOT11_IE_ID_QOS_TRAFFIC_CAPABILITY               = 89,
    DOT11_IE_ID_BSS_MAX_IDLE_PERIOD                  = 90,
    DOT11_IE_ID_TFS_REQUEST                          = 91,
    DOT11_IE_ID_TFS_RESPONSE                         = 92,
    DOT11_IE_ID_WNM_SLEEP_MODE                       = 93,
    DOT11_IE_ID_TIM_BROADCAST_REQUEST                = 94,
    DOT11_IE_ID_TIM_BROADCAST_RESPONSE               = 95,
    DOT11_IE_ID_COLLOCATED_INTERFERENCE_REPORT       = 96,
    DOT11_IE_ID_CHANNEL_USAGE                        = 97,
    DOT11_IE_ID_TIME_ZONE                            = 98,
    DOT11_IE_ID_DMS_REQUEST                          = 99,
    DOT11_IE_ID_DMS_RESPONSE                         = 100,
    DOT11_IE_ID_LINK_IDENTIFIER                      = 101,
    DOT11_IE_ID_WAKEUP_SCHEDULE                      = 102,
    /* 103 Reserved */
    DOT11_IE_ID_CHANNEL_SWITCH_TIMING                = 104,
    DOT11_IE_ID_PTI_CONTROL                          = 105,
    DOT11_IE_ID_TPU_BUFFER_STATUS                    = 106,
    DOT11_IE_ID_INTERWORKING                         = 107,
    DOT11_IE_ID_ADVERTISMENT_PROTOCOL                = 108,
    DOT11_IE_ID_EXPEDITED_BANDWIDTH_REQUEST          = 109,
    DOT11_IE_ID_QOS_MAP_SET                          = 110,
    DOT11_IE_ID_ROAMING_CONSORTIUM                   = 111,
    DOT11_IE_ID_EMERGENCY_ALERT_IDENTIFIER           = 112,
    DOT11_IE_ID_MESH_CONFIGURATION                   = 113,
    DOT11_IE_ID_MESH_ID                              = 114,
    DOT11_IE_ID_MESH_LINK_METRIC_REPORT              = 115,
    DOT11_IE_ID_CONGESTION_NOTIFICATION              = 116,
    DOT11_IE_ID_MESH_PEERING_MANAGEMENT              = 117,
    DOT11_IE_ID_MESH_CHANNEL_SWITCH_PARAMETERS       = 118,
    DOT11_IE_ID_MESH_AWAKE_WINDOW                    = 119,
    DOT11_IE_ID_BEACON_TIMING                        = 120,
    DOT11_IE_ID_MCCAOP_SETUP_REQUEST                 = 121,
    DOT11_IE_ID_MCCAOP_SETUP_REPLY                   = 122,
    DOT11_IE_ID_MCCAOP_ADVERTISMENT                  = 123,
    DOT11_IE_ID_MCCAOP_TEARDOWN                      = 124,
    DOT11_IE_ID_GANN                                 = 125,
    DOT11_IE_ID_RANN                                 = 126,
    DOT11_IE_ID_EXTENDED_CAPABILITIES                = 127,
    /* 128-129 Reserved */
    DOT11_IE_ID_PREQ                                 = 130,
    DOT11_IE_ID_PREP                                 = 131,
    DOT11_IE_ID_PERR                                 = 132,
    /* 133-136 Reserved */
    DOT11_IE_ID_PXU                                  = 137,
    DOT11_IE_ID_PXUC                                 = 138,
    DOT11_IE_ID_AUTHENTICATED_MESH_PEERING_EXCHANGE  = 139,
    DOT11_IE_ID_MIC                                  = 140,
    DOT11_IE_ID_DESTINATION_URI                      = 141,
    DOT11_IE_ID_U_APSD_COEXISTENCE                   = 142,
    /* 143-173 Reserved */
    DOT11_IE_ID_MCCAOP_ADVERTISMENT_OVERVIEW         = 174,
    /* 175-220 Reserved */
    DOT11_IE_ID_VENDOR_SPECIFIC                      = 221,
    /* 222-255 Reserved */
} dot11_ie_id_t;

/**
 * Enumeration of AKM (authentication and key management) suites. Table 8-140 802.11mc D3.0.
 */
typedef enum
{
    WICED_AKM_RESERVED                    = 0,
    WICED_AKM_8021X                       = 1,    /**< WPA2 enterprise                 */
    WICED_AKM_PSK                         = 2,    /**< WPA2 PSK                        */
    WICED_AKM_FT_8021X                    = 3,    /**< 802.11r Fast Roaming enterprise */
    WICED_AKM_FT_PSK                      = 4,    /**< 802.11r Fast Roaming PSK        */
    WICED_AKM_8021X_SHA256                = 5,
    WICED_AKM_PSK_SHA256                  = 6,
    WICED_AKM_TDLS                        = 7,    /**< Tunneled Direct Link Setup      */
    WICED_AKM_SAE_SHA256                  = 8,
    WICED_AKM_FT_SAE_SHA256               = 9,
    WICED_AKM_AP_PEER_KEY_SHA256          = 10,
    WICED_AKM_SUITEB_8021X_HMAC_SHA256    = 11,
    WICED_AKM_SUITEB_8021X_HMAC_SHA384    = 12,
    WICED_AKM_SUITEB_FT_8021X_HMAC_SHA384 = 13,
} wiced_akm_suite_t;

/**
 * Enumeration of cipher suites. Table 8-138 802.11mc D3.0.
 */
typedef enum
{
    WICED_CIPHER_GROUP                 = 0,   /**< Use group cipher suite                                        */
    WICED_CIPHER_WEP_40                = 1,   /**< WEP-40                                                        */
    WICED_CIPHER_TKIP                  = 2,   /**< TKIP                                                          */
    WICED_CIPHER_RESERVED              = 3,   /**< Reserved                                                      */
    WICED_CIPHER_CCMP_128              = 4,   /**< CCMP-128 - default pairwise and group cipher suite in an RSNA */
    WICED_CIPHER_WEP_104               = 5,   /**< WEP-104 - also known as WEP-128                               */
    WICED_CIPHER_BIP_CMAC_128          = 6,   /**< BIP-CMAC-128 - default management frame cipher suite          */
    WICED_CIPHER_GROUP_DISALLOWED      = 7,   /**< Group address traffic not allowed                             */
    WICED_CIPHER_GCMP_128              = 8,   /**< GCMP-128 - default for 60 GHz STAs                            */
    WICED_CIPHER_GCMP_256              = 9,   /**< GCMP-256 - introduced for Suite B                             */
    WICED_CIPHER_CCMP_256              = 10,  /**< CCMP-256 - introduced for suite B                             */
    WICED_CIPHER_BIP_GMAC_128          = 11,  /**< BIP-GMAC-128 - introduced for suite B                         */
    WICED_CIPHER_BIP_GMAC_256          = 12,  /**< BIP-GMAC-256 - introduced for suite B                         */
    WICED_CIPHER_BIP_CMAC_256          = 13,  /**< BIP-CMAC-256 - introduced for suite B                         */
} wiced_80211_cipher_t;

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    wlan_state_t         state;
    wiced_country_code_t country_code;
    uint32_t             keep_wlan_awake;
} wwd_wlan_status_t;

#define WWD_WLAN_KEEP_AWAKE( )  do { wwd_wlan_status.keep_wlan_awake++; } while (0)
#define WWD_WLAN_LET_SLEEP( )   do { wwd_wlan_status.keep_wlan_awake--; } while (0)



#pragma pack(1)

/* 802.11 Information Element structures */

/* DSS Parameter Set */
typedef struct
{
    tlv8_header_t tlv_header;          /* id, length */
    uint8_t current_channel;
} dsss_parameter_set_ie_t;

#define DSSS_PARAMETER_SET_LENGTH (1)

/* Robust Secure Network */
typedef struct
{
    tlv8_header_t tlv_header;          /* id, length */
    uint16_t version;
    uint32_t group_key_suite;          /* See wiced_80211_cipher_t for values */
    uint16_t pairwise_suite_count;
    uint32_t pairwise_suite_list[1];
} rsn_ie_fixed_portion_t;

#define RSN_IE_MINIMUM_LENGTH (8)

typedef struct
{
    tlv8_header_t tlv_header;          /* id, length */
    uint8_t  oui[4];
} vendor_specific_ie_header_t;

#define VENDOR_SPECIFIC_IE_MINIMUM_LENGTH (4)

/* WPA IE */
typedef struct
{
    vendor_specific_ie_header_t vendor_specific_header;
    uint16_t version;
    uint32_t multicast_suite;
    uint16_t unicast_suite_count;
    uint8_t  unicast_suite_list[1][4];
} wpa_ie_fixed_portion_t;

#define WPA_IE_MINIMUM_LENGTH (12)

typedef struct
{
    uint16_t akm_suite_count;
    uint32_t akm_suite_list[1];
} akm_suite_portion_t;

typedef struct
{
    tlv8_header_t tlv_header;          /* id, length */
    uint16_t ht_capabilities_info;
    uint8_t  ampdu_parameters;
    uint8_t  rx_mcs[10];
    uint16_t rxhighest_supported_data_rate;
    uint8_t  tx_supported_mcs_set;
    uint8_t  tx_mcs_info[3];
    uint16_t ht_extended_capabilities;
    uint32_t transmit_beam_forming_capabilities;
    uint8_t  antenna_selection_capabilities;
} ht_capabilities_ie_t;

#define HT_CAPABILITIES_IE_LENGTH (26)

#define HT_CAPABILITIES_INFO_LDPC_CODING_CAPABILITY        ( 1 <<  0 )
#define HT_CAPABILITIES_INFO_SUPPORTED_CHANNEL_WIDTH_SET   ( 1 <<  1 )
#define HT_CAPABILITIES_INFO_SM_POWER_SAVE_OFFSET          ( 1 <<  2 )
#define HT_CAPABILITIES_INFO_SM_POWER_SAVE_MASK            ( 3 <<  2 )
#define HT_CAPABILITIES_INFO_HT_GREENFIELD                 ( 1 <<  4 )
#define HT_CAPABILITIES_INFO_SHORT_GI_FOR_20MHZ            ( 1 <<  5 )
#define HT_CAPABILITIES_INFO_SHORT_GI_FOR_40MHZ            ( 1 <<  6 )
#define HT_CAPABILITIES_INFO_TX_STBC                       ( 1 <<  7 )
#define HT_CAPABILITIES_INFO_RX_STBC_OFFSET                ( 1 <<  8 )
#define HT_CAPABILITIES_INFO_RX_STBC_MASK                  ( 3 <<  8 )
#define HT_CAPABILITIES_INFO_HT_DELAYED_BLOCK_ACK          ( 1 << 10 )
#define HT_CAPABILITIES_INFO_MAXIMUM_A_MSDU_LENGTH         ( 1 << 11 )
#define HT_CAPABILITIES_INFO_DSSS_CCK_MODE_IN_40MHZ        ( 1 << 12 )
/* bit 13 reserved */
#define HT_CAPABILITIES_INFO_40MHZ_INTOLERANT              ( 1 << 14 )
#define HT_CAPABILITIES_INFO_L_SIG_TXOP_PROTECTION_SUPPORT ( 1 << 15 )

#pragma pack()

/******************************************************
 *             Function declarations
 ******************************************************/

extern wiced_bool_t wwd_wifi_ap_is_up;
extern uint8_t      wwd_tos_map[8];
extern eapol_packet_handler_t wwd_eapol_packet_handler;

/* Device core control functions */
extern wwd_result_t wwd_disable_device_core    ( device_core_t core_id, wlan_core_flag_t core_flag );
extern wwd_result_t wwd_reset_device_core      ( device_core_t core_id, wlan_core_flag_t core_flag );
extern wwd_result_t wwd_wlan_armcore_run       ( device_core_t core_id, wlan_core_flag_t core_flag );
extern wwd_result_t wwd_device_core_is_up      ( device_core_t core_id );
extern wwd_result_t wwd_wifi_set_down          ( wwd_interface_t interface );
extern void         wwd_set_country            ( wiced_country_code_t code );

/******************************************************
 *             Global variables
 ******************************************************/

extern wwd_wlan_status_t wwd_wlan_status;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_INTERNAL_H */
