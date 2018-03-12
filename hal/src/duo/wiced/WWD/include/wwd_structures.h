/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines common structures used with WWD
 *
 */

#ifndef INCLUDED_WWD_STRUCTURES_H
#define INCLUDED_WWD_STRUCTURES_H

#include "wwd_constants.h"
#include "wwd_wlioctl.h"


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define SSID_NAME_SIZE        (32)
#define HOSTNAME_SIZE         (32)

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/** @cond !ADDTHIS*/

typedef volatile void*    host_semaphore_pointer_t;
typedef volatile void*    host_mutex_pointer_t;
typedef volatile void*    host_thread_pointer_t;

typedef uint32_t          wwd_time_t;          /* Time value in milliseconds */
typedef struct wl_bss_info_struct     wiced_bss_info_t;

typedef struct edcf_acparam    wiced_edcf_ac_param_t;
typedef struct wl_action_frame wiced_action_frame_t;

typedef struct wiced_event_header_struct wwd_event_header_t;
/** @endcond */

/******************************************************
 *                    Structures
 ******************************************************/


/*
    The received packet formats are different when EXT_STA is enabled. In case
    of EXT_STA the received packets are in 802.11 format, where as in other
    case the received packets have Ethernet II format

    1. 802.11 frames
    ----------------------------------------------------------------------------
    | FC (2) | DID (2) |A1 (6) |A2 (6)|A3 (6) |SID (2) |SNAP (6) |type (2) |data (46 - 1500) |
    ----------------------------------------------------------------------------

    2. Ethernet II frames
    -------------------------------------------------
    | DA (6) | SA (6) | type (2) | data (46 - 1500) |
    -------------------------------------------------
    */


/**
 * Structure describing a packet filter list item
*/
typedef struct
{
    uint32_t                       id;            /**< Unique identifier for a packet filter item                             */
    wiced_packet_filter_rule_t     rule;          /**< Filter matches are either POSITIVE or NEGATIVE matching */
    uint16_t                       offset;        /**< Offset in bytes to start filtering (referenced to the start of the ethernet packet) */
    uint16_t                       mask_size;     /**< Size of the mask in bytes */
    uint8_t*                       mask;          /**< Pattern mask bytes to be ANDed with the pattern eg. "\xff00" (must be in network byte order) */
    uint8_t*                       pattern;       /**< Pattern bytes used to filter eg. "\x0800"  (must be in network byte order) */
    wiced_bool_t                   enabled_status; /**< When returned from wwd_wifi_get_packet_filters, indicates if the filter is enabled */
} wiced_packet_filter_t;

/** @cond */
typedef struct wl_pkt_filter_stats wiced_packet_filter_stats_t;
/** @endcond */

/**
 * Structure describing a packet filter list item
*/
typedef struct
{
    uint8_t     keep_alive_id;  /**< Unique identifier for the keep alive packet */
    uint32_t    period_msec;    /**< Repeat interval in milliseconds             */
    uint16_t    packet_length;  /**< Length of the keep alive packet             */
    uint8_t*    packet;         /**< Pointer to the keep alive packet            */
} wiced_keep_alive_packet_t;

/**
 * Structure for storing a Service Set Identifier (i.e. Name of Access Point)
 */
typedef struct
{
    uint8_t length;                  /**< SSID length */
    uint8_t value[ SSID_NAME_SIZE ]; /**< SSID name (AP name)  */
} wiced_ssid_t;

/**
 * Structure for storing a MAC address (Wi-Fi Media Access Control address).
 */
typedef struct
{
    uint8_t octet[6]; /**< Unique 6-byte MAC address */
} wiced_mac_t;

/**
 * Structure for storing a null terminated network hostname
 */
typedef struct
{
    char value[ HOSTNAME_SIZE + 1 ];
} wiced_hostname_t;

/**
 * Structure for storing extended scan parameters
 */
typedef struct
{
    int32_t number_of_probes_per_channel;                     /**< Number of probes to send on each channel                                               */
    int32_t scan_active_dwell_time_per_channel_ms;            /**< Period of time to wait on each channel when active scanning                            */
    int32_t scan_passive_dwell_time_per_channel_ms;           /**< Period of time to wait on each channel when passive scanning                           */
    int32_t scan_home_channel_dwell_time_between_channels_ms; /**< Period of time to wait on the home channel when scanning. Only relevant if associated. */
} wiced_scan_extended_params_t;

/**
 * Structure for storing radio band list information
 */
typedef struct
{
    int32_t number_of_bands; /**< Number of bands supported, currently 1 or 2      */
    int32_t current_band;    /**< Current band type : WLC_BAND_2G or WLC_BAND_5G   */
    int32_t other_band;      /**< If number of bands is 2 then the other band type */
} wiced_band_list_t;

/**
 * Structure for storing AP information
 */
#pragma pack(1)
typedef struct wiced_ap_info
{
    wiced_ssid_t              SSID;             /**< Service Set Identification (i.e. Name of Access Point)                    */
    wiced_mac_t               BSSID;            /**< Basic Service Set Identification (i.e. MAC address of Access Point)       */
    int16_t                   signal_strength;  /**< Receive Signal Strength Indication in dBm. <-90=Very poor, >-30=Excellent */
    uint32_t                  max_data_rate;    /**< Maximum data rate in kilobits/s                                           */
    wiced_bss_type_t          bss_type;         /**< Network type                                                              */
    wiced_security_t          security;         /**< Security type                                                             */
    uint8_t                   channel;          /**< Radio channel that the AP beacon was received on                          */
    wiced_802_11_band_t       band;             /**< Radio band                                                                */
    struct wiced_ap_info*     next;             /**< Pointer to the next scan result                                           */
} wiced_ap_info_t;
#pragma pack()

/**
 * Structure for storing scan results
 */
#pragma pack(1)
typedef struct wiced_scan_result
{
    wiced_ssid_t              SSID;             /**< Service Set Identification (i.e. Name of Access Point)                    */
    wiced_mac_t               BSSID;            /**< Basic Service Set Identification (i.e. MAC address of Access Point)       */
    int16_t                   signal_strength;  /**< Receive Signal Strength Indication in dBm. <-90=Very poor, >-30=Excellent */
    uint32_t                  max_data_rate;    /**< Maximum data rate in kilobits/s                                           */
    wiced_bss_type_t          bss_type;         /**< Network type                                                              */
    wiced_security_t          security;         /**< Security type                                                             */
    uint8_t                   channel;          /**< Radio channel that the AP beacon was received on                          */
    wiced_802_11_band_t       band;             /**< Radio band                                                                */
    wiced_bool_t              on_channel;       /**< True if scan result was recorded on the channel advertised in the packet  */
    struct wiced_scan_result* next;             /**< Pointer to the next scan result                                           */
} wiced_scan_result_t;
#pragma pack()

/**
 * Structure for storing a WEP key
 */
typedef struct
{
    uint8_t index;    /**< WEP key index [0/1/2/3]                                             */
    uint8_t length;   /**< WEP key length. Either 5 bytes (40-bits) or 13-bytes (104-bits) */
    uint8_t data[32]; /**< WEP key as values NOT chars                                     */
} wiced_wep_key_t;


/** Structure for storing 802.11 powersave listen interval values \n
 * See @ref wiced_wifi_get_listen_interval for more information
 */
typedef struct
{
    uint8_t  beacon;  /**< Listen interval in beacon periods */
    uint8_t  dtim;    /**< Listen interval in DTIM periods   */
    uint16_t assoc;   /**< Listen interval as sent to APs    */
} wiced_listen_interval_t;


#pragma pack(1)

/**
 * Structure describing a list of associated softAP clients
 */
typedef struct
{
    uint32_t    count;         /**< Number of MAC addresses in the list    */
    wiced_mac_t   mac_list[1];   /**< Variable length array of MAC addresses */
} wiced_maclist_t;

typedef struct
{
    char     abbrev[3];
    uint8_t  rev;
    uint8_t  data[64];
} wiced_country_info_t;

#pragma pack()

/* DSS Parameter Set */
typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t current_channel;
} dsss_parameter_set_ie_t;

/* In dongle firmware this is defined as ht_add_ie_t in 802.11.h. It has similar structure but different field names due to history.
 * Wireshark reports this element as the HT information element. */
typedef struct
{
    uint8_t       type;
    uint8_t       length;
    uint8_t       primary_channel;
    uint8_t       ht_operation_subset_1;
    uint16_t      ht_operation_subset_2;
    uint16_t      ht_operation_subset_3;
    uint8_t       rx_supported_mcs_set[16];
} ht_operation_ie_t;


/* 11k Radio resource capabilities bit information */
typedef struct radio_resource_management_capability_debug_msg
{
    uint32_t value;
    const char* string;
} radio_resource_management_capability_debug_msg_t;

/* 11k Radio resource capabilities */
typedef struct
{
    uint8_t radio_resource_management[RRM_CAPABILITIES_LEN];
} radio_resource_management_capability_ie_t;

/* 11k Radio resource beacon request */
typedef struct radio_resource_management_beacon_req
{
    uint8_t       bcn_mode;
    int           duration;
    int           channel;
    wiced_mac_t   da;
    uint16_t      random_int;
    wlc_ssid_t    ssid;
    uint16_t      repetitions;
} radio_resource_management_beacon_req_t;

/* 11k Radio resource management request */
typedef struct radio_resource_management_req
{
    wiced_mac_t   da;
    uint8_t       regulatory;
    uint8_t       channel;
    uint16_t      random_int;
    uint16_t      duration;
    uint16_t      repetitions;
} radio_resource_management_req_t;

/* 11k Radio resource management frame request */
typedef struct radio_resource_management_framereq
{
    wiced_mac_t   da;
    uint8_t       regulatory;
    uint8_t       channel;
    uint16_t      random_int;
    uint16_t      duration;
    wiced_mac_t   ta;
    uint16_t      repetitions;
} radio_resource_management_framereq_t;

/* 11k Radio resource management statistics request */
typedef struct radio_resource_management_statreq
{
    wiced_mac_t   da;
    wiced_mac_t   peer;
    uint16_t      random_int;
    uint16_t      duration;
    uint8_t       group_id;
    uint16_t      repetitions;
} radio_resource_management_statreq_t;

/* 11k Radio resource statistics report */
typedef struct
{
    uint16_t      version;
    wiced_mac_t   sta_address;
    uint32_t      timestamp;
    uint16_t      flag;
    uint16_t      length_of_payload;
    unsigned char data[WL_RRM_RPT_MAX_PAYLOAD];
} radio_resource_management_statrpt_t;

/* 11k Neighbor Report element */
typedef struct rrm_nbr_element
{
    uint8_t       id;
    uint8_t       length;
    wiced_mac_t   bssid;
    uint32_t      bssid_info;
    uint8_t       regulatory;
    uint8_t       channel;
    uint8_t       phytype;
    uint8_t       pad;
} radio_resource_management_nbr_element_t;

typedef struct
{
    uint8_t mode;       /* 1 == target must not tx between recieving CSA and actually switching */
                        /* 0 == target may tx between recieving CSA and actually switching */
    uint8_t count;      /* count number of beacons before switching */
    uint16_t chspec;    /* target chanspec */
} wiced_chan_switch_t;

typedef struct radio_resource_management_neight_report
{
  struct radio_resource_management_neight_report *link;
  radio_resource_management_nbr_element_t nbr_elm;
} radio_resource_management_neighbor_report_t;


typedef struct wwd_rrm_report
{
  wwd_rrm_report_type_t      type;
  uint32_t                   report_len;
  uint8_t                   *report;
} wwd_rrm_report_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_STRUCTURES_H */
