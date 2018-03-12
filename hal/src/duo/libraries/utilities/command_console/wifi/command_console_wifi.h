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

#include "wiced_result.h"
#include "wiced_network.h"
#include "wwd_structures.h"
#include "wwd_wifi.h"
#include "wwd_debug.h"
#include "wwd_constants.h"

#include "wwd_wifi.h"
#include "wwd_debug.h"
#include "wwd_constants.h"
#include "internal/wwd_bcmendian.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

#include "platform_mcu_peripheral.h"
#include "wiced_deep_sleep.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

#define WIFI_COMMANDS \
    { (char*) "antenna",                            antenna,                            1, NULL, NULL, (char*) "<0|1|3>",                                    (char*) "Antenna selection. 3 = Auto"},\
    { (char*) "disable_11n",                        disable_11n,                        0, NULL, NULL, (char*) "",                                           (char*) "Disable 11n mode operation"},\
    { (char*) "enable_11n",                         enable_11n,                         0, NULL, NULL, (char*) "",                                           (char*) "Enable 11n mode operation"},\
    { (char*) "get_noise",                          get_noise,                          0, NULL, NULL, (char*) "",                                           (char*) "Get PHY noise after successful TX"},\
    { (char*) "get_ap_info",                        get_ap_info,                        0, NULL, NULL, (char*) "",                                           (char*) "Get AP information"}, \
    { (char*) "get_access_category_parameters_sta", get_access_category_parameters_sta, 0, NULL, NULL, (char*) "",                                           (char*) "Get access category parameters for STA"}, \
    { (char*) "get_associated_sta_list",            get_associated_sta_list,            0, NULL, NULL, (char*) "",                                           (char*) "Get list of associated clients"}, \
    { (char*) "get_counters",                       get_counters,                       0, NULL, NULL, (char*) "[-t seconds][-n][rx][tx][rate]",                               (char*) "Get counters. Options: -t seconds to collect, -n displays counters normalized per second, catagory"}, \
    { (char*) "get_country",                        get_country,                        0, NULL, NULL, (char*) "",                                           (char*) "Get country."},\
    { (char*) "set_country",                        set_country,                        1, NULL, NULL, (char*) "<US/0|KR/4>",                                (char*) "Set country."},\
    { (char*) "get_rate",                           get_rate,                           0, NULL, NULL, (char*) "",                                           (char*) "Get current rate."},\
    { (char*) "get_data_rate",                      get_data_rate,                      0, NULL, NULL, (char*) "",                                           (char*) "Get data rate."},\
    { (char*) "get_mac_addr",                       get_mac_addr,                       0, NULL, NULL, (char*) "",                                           (char*) "Get the device MAC address."}, \
    { (char*) "get_preferred_association_band",     get_preferred_association_band,     0, NULL, NULL, (char*) "",                                           (char*) "Get the preferred radio band for association."}, \
    { (char*) "get_pmk",                            get_pmk,                            1, NULL, NULL, (char*) "<key>",                                      (char*) "Get PMK"}, \
    { (char*) "get_random",                         get_random,                         0, NULL, NULL, (char*) "",                                           (char*) "Get a random number."}, \
    { (char*) "get_rssi",                           get_rssi,                           0, NULL, NULL, (char*) "",                                           (char*) "Get the received signal strength of the AP (client mode only)."}, \
    { (char*) "get_soft_ap_credentials",            get_soft_ap_credentials,            0, NULL, NULL, (char*) "",                                           (char*) "Get SoftAP credentials"},\
    { (char*) "get_tx_power",                       get_tx_power,                       0, NULL, NULL, (char*) "",                                           (char*) "Gets the tx power in dBm."},\
    { (char*) "join",                               join,                               2, NULL, NULL, (char*) "<ssid> <open|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key] [channel] [ip netmask gateway]\n\t--Encapsulate SSID in quotes in order to include spaces", (char*) "Join an AP. DHCP assumed if no IP address provided"}, \
    { (char*) "join_adhoc",                         join_adhoc,                         6, NULL, NULL, (char*) "<ssid> <open|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key] [ip netmask gateway]", (char*) "Join specified IBSS, no DHCP assumed."}, \
    { (char*) "join_ent",                           join_ent,                           0, NULL, NULL, (char*) "<ssid> <eap_tls|peap> [username] [password] <wpa2|wpa2_tkip|wpa|wpa_tkip>",       (char*) "Join an AP using an enterprise EAP method. DHCP assumed."}, \
    { (char*) "join_specific",                      join_specific,                      2, NULL, NULL, (char*) "<ssid> <bssid> <channel> <open|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key] [ip netmask gateway]", (char*) "Join specified AP. DHCP assumed if no IP address provided"}, \
    { (char*) "leave",                              leave,                              0, NULL, NULL, (char*) "",                                           (char*) "Leave an AP."}, \
    { (char*) "phyrate_dump",                       get_phyrate_log,                    1, NULL, NULL, (char*) "<bin size>",                                 (char*) "Dump the phyrate log and bin averages to the console."}, \
    { (char*) "scan",                               scan,                               0, NULL, NULL, (char*) "",                                           (char*) "Scan all enabled channels and print a list of APs found."}, \
    { (char*) "set_data_rate",                      set_data_rate,                      1, NULL, NULL, (char*) "<1|2|5.5|6|9|11|12|18|24|36|48|54>",         (char*) "Set data rate."},\
    { (char*) "set_legacy_rate",                    set_legacy_rate,                    1, NULL, NULL, (char*) "<1|2|5.5|6|9|11|12|18|24|36|48|54>",         (char*) "Set legacy (CCK/OFDM) rate on PHY"},\
    { (char*) "set_mcs_rate",                       set_mcs_rate,                       1, NULL, NULL, (char*) "<MCS Index> <Override MCS only>",            (char*) "Set MCS rate on PHY"},\
    { (char*) "set_listen_interval",                set_listen_interval,                1, NULL, NULL, (char*) "<listen interval> <time unit>",              (char*) "Set listen interval in time unit 0 = Beacon Intervals. 1 = DTIM Intervals"}, \
    { (char*) "set_preferred_association_band",     set_preferred_association_band,     0, NULL, NULL, (char*) "<0 = Auto | 1 = 5GHz | 2 = 2.4GHz>",         (char*) "Set the preferred radio band for association"}, \
    { (char*) "set_tx_power",                       set_tx_power,                       1, NULL, NULL, (char*) "<0-31>",                                     (char*) "Set the tx power in dBm."},\
    { (char*) "start_ap",                           start_ap,                           4, NULL, NULL, (char*) "<ssid> <open|wpa2|wpa2_aes|wep|wep_shared> <key> <channel> <wps>\n\t--Encapsulate SSID in quotes in order to include spaces",(char*) "Start AP mode."}, \
    { (char*) "status",                             status,                             0, NULL, NULL, (char*) "",                                           (char*) "Print status of the Wi-Fi and network interfaces."},\
    { (char*) "stop_ap",                            stop_ap,                            0, NULL, NULL, (char*) "",                                           (char*) "Stop AP mode."}, \
    { (char*) "test_ap",                            test_ap,                            4, NULL, NULL, (char*) "<ssid> <open|wpa2|wpa2_aes> <key> <channel> <wps> <iterations>\n\t--Encapsulate SSID in quotes in order to include spaces",(char*) "Test AP mode."}, \
    { (char*) "test_join",                          test_join,                          2, NULL, NULL, (char*) "<ssid> <open|wep|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key] [ip netmask gateway] <iterations>\n\t--Encapsulate SSID in quotes in order to include spaces", (char*) "Test joining an AP. DHCP assumed if no IP address provided"}, \
    { (char*) "test_join_specific",                 test_join_specific,                 2, NULL, NULL, (char*) "<ssid> <bssid> <channel> <open|wep|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key] [ip netmask gateway] <iterations>", (char*) "Test joining an AP. DHCP assumed if no IP address provided"}, \
    { (char*) "test_cred",                          test_credentials,                   2, NULL, NULL, (char*) "<ssid> <bssid> <channel> <open|wep|wpa_aes|wpa_tkip|wpa2|wpa2_tkip> [key]\n\t--Encapsulate SSID in quotes in order to include spaces", (char*) "Test joining an AP"}, \
    { (char*) "wifi_powersave",                     wifi_powersave,                     1, NULL, NULL, (char*) "<mode> [delay]",                             (char*) "Enable/disable Wi-Fi powersave mode. 0 = disable. 1 = PS Poll. 2 = Wait [delay] ms before entering powersave"}, \
    { (char*) "wifi_resume",                        wifi_resume,                        0, NULL, NULL, (char*) "[<ip> <netmask> <gateway>]",                 (char*) "Resume networking after deep-sleep"}, \
    { (char*) "wlan_chip_log",                      read_wlan_chip_console_log,         0, NULL, NULL, (char*) "",                                           (char*) "Dump WLAN chip console log"}, \
    { (char*) "wlog",                               read_wlan_chip_console_log,         0, NULL, NULL, (char*) "",                                           (char*) "Dump WLAN chip console log"}, \
    { (char*) "peek",                               peek,                               1, NULL, NULL, (char*) " peek [address]",                            (char*) "Dump memory"}, \
    { (char*) "poke",                               poke,                               2, NULL, NULL, (char*) " poke [address] [value]",                    (char*) "Write memory"}, \
    { (char*) "find_ap",                            find_ap,                            1, NULL, NULL, (char*) "[SSID]",                                     (char*) "Find AP"}, \
    { (char*) "rrm",                                set_get_rrm,                        1, NULL, NULL, (char*) "<set|get> [value]",                          (char*) "enable or disable RRM feature"}, \
    { (char*) "rrm_nbr_req",                        rrm_nbr_req,                        1, NULL, NULL, (char*) "[SSID]",                                     (char*) "send 11k neighbor report measurement request (works only when associated)"}, \
    { (char*) "rrm_lm_req",                         rrm_lm_req,                         1, NULL, NULL, (char*) "[da]",                                       (char*) "send 11k link measurement request"}, \
    { (char*) "rrm_bcn_req",                        rrm_bcn_req,                        7, NULL, NULL, (char*) "[bcn mode] [da] [duration] [random int] [channel] [ssid] [repetitions]", (char*) "send 11k beacon measurement request"}, \
    { (char*) "rrm_chload_req",                     rrm_req,                            6, NULL, NULL, (char*) "[regulatory] [da] [duration] [random int] [channel] [repetitions]", (char*) "send 11k channel load measurement request"}, \
    { (char*) "rrm_noise_req",                      rrm_req,                            6, NULL, NULL, (char*) "[regulatory] [da] [duration] [random int] [channel] [repetitions]", (char*) "send 11k noise measurement request"}, \
    { (char*) "rrm_frame_req",                      rrm_frame_req,                      7, NULL, NULL, (char*) "[regulatory] [da] [duration] [random int] [channel] [ta] [repetitions]", (char*) "send 11k frame measurement request"}, \
    { (char*) "rrm_stat_req",                       rrm_stat_req,                       6, NULL, NULL, (char*) "[da] [random int] [duration] [peer] [group id] [repetitions]", (char*) "send 11k stat measurement request"}, \
    { (char*) "rrm_nbr_list",                       rrm_nbr_list,                       0, NULL, NULL, (char*) "",                                           (char*) "get 11k neighbor report list"}, \
    { (char*) "rrm_nbr_del_nbr",                    rrm_nbr_del_nbr,                    1, NULL, NULL, (char*) "[bssid]",                                    (char*) "delete node from 11k neighbor report list"}, \
    { (char*) "rrm_nbr_add_nbr",                    rrm_nbr_add_nbr,                    5, NULL, NULL, (char*) "[bssid] [bssid info] [regulatory] [channel] [phytype]", (char*) "add node to 11k neighbor report list"}, \
    { (char*) "fbtoverds",                          fbt_over_ds,                        0, NULL, NULL, (char*) "[value]", (char*) "Use of FBT(Fast BSS Transition) Over-the-DS(Distribution System) is allowed "}, \
    { (char*) "fbt_cap",                            fbt_caps,                           0, NULL, NULL, (char*) "",                                           (char*) "Driver 4-way handshake & reassoc (WLFBT) "}, \
    { (char*) "mfp_cap",                            mfp_capabilities,                   1, NULL, NULL, (char*) "<set|get> [value]",                          (char*) "Set or Get PMF (Protected Management Frame)"}, \
    { (char*) "reset_wifi_counters",                reset_statistics_counters,          0, NULL, NULL, (char*) "<set> [value]",                              (char*) "Reset persistent WiFi statistics counters"}, \

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

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

/* Console commands */
int antenna                           ( int argc, char* argv[] );
int disable_11n                       ( int argc, char* argv[] );
int enable_11n                        ( int argc, char* argv[] );
int get_noise                         ( int argc, char* argv[] );
int get_access_category_parameters_sta( int argc, char* argv[] );
int get_ap_info                       ( int argc, char* argv[] );
int get_associated_sta_list           ( int argc, char* argv[] );
int get_counters                      ( int argc, char* argv[] );
int get_country                       ( int argc, char* argv[] );
int set_country                       ( int argc, char* argv[] );
int get_rate                          ( int argc, char* argv[] );
int get_data_rate                     ( int argc, char* argv[] );
int get_mac_addr                      ( int argc, char* argv[] );
int get_pmk                           ( int argc, char* argv[] );
int get_preferred_association_band    ( int argc, char* argv[] );
int get_random                        ( int argc, char* argv[] );
int get_rssi                          ( int argc, char* argv[] );
int get_soft_ap_credentials           ( int argc, char* argv[] );
int get_tx_power                      ( int argc, char* argv[] );
int join                              ( int argc, char* argv[] );
int join_adhoc                        ( int argc, char* argv[] );
int join_ent                          ( int argc, char* argv[] );
int join_specific                     ( int argc, char* argv[] );
int leave                             ( int argc, char* argv[] );
int read_wlan_chip_console_log        ( int argc, char* argv[] );
int get_phyrate_log                   ( int argc, char* argv[] );
int scan                              ( int argc, char* argv[] );
int set_data_rate                     ( int argc, char* argv[] );
int set_legacy_rate                   ( int argc, char* argv[] );
int set_mcs_rate                      ( int argc, char* argv[] );
int set_listen_interval               ( int argc, char* argv[] );
int set_preferred_association_band    ( int argc, char* argv[] );
int set_tx_power                      ( int argc, char* argv[] );
int start_ap                          ( int argc, char* argv[] );
int status                            ( int argc, char* argv[] );
int stop_ap                           ( int argc, char* argv[] );
int test_ap                           ( int argc, char* argv[] );
int test_join                         ( int argc, char* argv[] );
int test_credentials                  ( int argc, char* argv[] );
int test_join_specific                ( int argc, char* argv[] );
int wifi_powersave                    ( int argc, char* argv[] );
int wifi_resume                       ( int argc, char* argv[] );
int peek                              ( int argc, char* argv[] );
int poke                              ( int argc, char* argv[] );
int find_ap                           ( int argc, char* argv[] );
int set_get_rrm                       ( int argc, char* argv[] );
int rrm_nbr_req                       ( int argc, char* argv[] );
int rrm_lm_req                        ( int argc, char* argv[] );
int rrm_bcn_req                       ( int argc, char* argv[] );
int rrm_req                           ( int argc, char* argv[] );
int rrm_frame_req                     ( int argc, char* argv[] );
int rrm_stat_req                      ( int argc, char* argv[] );
int rrm_nbr_list                      ( int argc, char* argv[] );
int rrm_nbr_del_nbr                   ( int argc, char* argv[] );
int rrm_nbr_add_nbr                   ( int argc, char* argv[] );
int fbt_over_ds                       ( int argc, char* argv[] );
int fbt_caps                          ( int argc, char* argv[] );
int mfp_capabilities                  ( int argc, char* argv[] );
int reset_statistics_counters         ( int argc, char* argv[] );
int print_rrm_caps                    (radio_resource_management_capability_ie_t *rrm_cap);



/* Functions potentially used by other modules */
int              wifi_join                        ( char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* key, uint16_t key_length, char* ip, char* netmask, char* gateway );
wiced_security_t str_to_authtype                  ( char* arg );
wiced_security_t str_to_enterprise_authtype       ( char* arg );
void             str_to_mac                       ( char* arg, wiced_mac_t* mac );
void             network_print_status             ( wiced_interface_t interface );

#ifdef __cplusplus
} /* extern "C" */
#endif
