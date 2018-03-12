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
 *  Prototypes of functions for controlling the Wi-Fi system
 *
 *  This file provides prototypes for end-user functions which allow
 *  actions such as scanning for Wi-Fi networks, joining Wi-Fi
 *  networks, getting the MAC address, etc
 *
 */

#ifndef INCLUDED_WWD_WIFI_H
#define INCLUDED_WWD_WIFI_H

#include <stdint.h>
#include "wwd_constants.h"                  /* For wwd_result_t */
#include "wwd_structures.h"
#include "chip_constants.h"
#include "RTOS/wwd_rtos_interface.h"        /* For semaphores */
#include "network/wwd_network_interface.h"  /* For interface definitions */
#include "wwd_structures.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @cond !ADDTHIS*/

#ifndef WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS
#define WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS     (5)
#endif

/* Packet Filter Offsets for Ethernet Frames */
#define FILTER_OFFSET_PACKET_START                       0
#define FILTER_OFFSET_ETH_HEADER_DESTINATION_ADDRESS     0
#define FILTER_OFFSET_ETH_HEADER_SOURCE_ADDRESS          6
#define FILTER_OFFSET_ETH_HEADER_ETHERTYPE              12
#define FILTER_OFFSET_ETH_DATA                          14

/* Packet Filter Offsets for ARP Packets */
#define FILTER_OFFSET_ARP_HEADER_START                  14
#define FILTER_OFFSET_ARP_HEADER_HTYPE                  14
#define FILTER_OFFSET_ARP_HEADER_PTYPE                  16
#define FILTER_OFFSET_ARP_HEADER_HLEN                   18
#define FILTER_OFFSET_ARP_HEADER_PLEN                   19
#define FILTER_OFFSET_ARP_HEADER_OPER                   20
#define FILTER_OFFSET_ARP_HEADER_SHA                    22
#define FILTER_OFFSET_ARP_HEADER_SPA                    28
#define FILTER_OFFSET_ARP_HEADER_THA                    30
#define FILTER_OFFSET_ARP_HEADER_TPA                    36

/* Packet Filter Offsets for IPv4 Packets */
#define FILTER_OFFSET_IPV4_HEADER_START                 14
#define FILTER_OFFSET_IPV4_HEADER_VER_IHL               14
#define FILTER_OFFSET_IPV4_HEADER_DSCP_ECN              15
#define FILTER_OFFSET_IPV4_HEADER_TOTAL_LEN             16
#define FILTER_OFFSET_IPV4_HEADER_ID                    18
#define FILTER_OFFSET_IPV4_HEADER_FLAGS_FRAGMENT_OFFSET 20
#define FILTER_OFFSET_IPV4_HEADER_TTL                   22
#define FILTER_OFFSET_IPV4_HEADER_PROTOCOL              23
#define FILTER_OFFSET_IPV4_HEADER_CHECKSUM              24
#define FILTER_OFFSET_IPV4_HEADER_SOURCE_ADDR           26
#define FILTER_OFFSET_IPV4_HEADER_DESTINATION_ADDR      30
#define FILTER_OFFSET_IPV4_HEADER_OPTIONS               34
#define FILTER_OFFSET_IPV4_DATA_START                   38

/* Packet Filter Offsets for IPv4 Packets */
#define FILTER_OFFSET_IPV6_HEADER_START                 14
#define FILTER_OFFSET_IPV6_HEADER_PAYLOAD_LENGTH        18
#define FILTER_OFFSET_IPV6_HEADER_NEXT_HEADER           20
#define FILTER_OFFSET_IPV6_HEADER_HOP_LIMIT             21
#define FILTER_OFFSET_IPV6_HEADER_SOURCE_ADDRESS        22
#define FILTER_OFFSET_IPV6_HEADER_DESTINATION_ADDRESS   38
#define FILTER_OFFSET_IPV6_DATA_START                   54

/* Packet Filter Offsets for ICMP Packets */
#define FILTER_OFFSET_ICMP_HEADER_START                 14

/** @endcond */

#define PM1_POWERSAVE_MODE          ( 1 )
#define PM2_POWERSAVE_MODE          ( 2 )
#define NO_POWERSAVE_MODE           ( 0 )

/* Roaming trigger options */
#define WICED_WIFI_DEFAULT_ROAMING TRIGGER              ( 0 )
#define WICED_WIFI_OPTIMIZE_BANDWIDTH_ROAMING_TRIGGER   ( 1 )
#define WICED_WIFI_OPTIMIZE_DISTANCE_ROAMING_TRIGGER    ( 2 )

/* Phyrate counts */
#define WICED_WIFI_PHYRATE_COUNT       16
#define WICED_WIFI_PHYRATE_LOG_SIZE    WL_PHYRATE_LOG_SIZE
#define WICED_WIFI_PHYRATE_LOG_OFF     0
#define WICED_WIFI_PHYRATE_LOG_TX      1
#define WICED_WIFI_PHYRATE_LOG_RX      2

/* Preferred Network Offload: time between scans
  * Set based on desired reconnect responsiveness after the AP has been off
  * for a long time.  (Reconnect will occur via roam if only off for 10s or less.)
  * Also, larger numbers will generally consume less energy.
  */
#define WICED_WIFI_PNO_SCAN_PERIOD     20

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                   Structures
 ******************************************************/

typedef void (*wwd_wifi_raw_packet_processor_t)( wiced_buffer_t buffer, wwd_interface_t interface );

/******************************************************
 *                 Global Variables
 ******************************************************/

extern wiced_bool_t wwd_wifi_p2p_go_is_up;

/******************************************************
 *             Function declarations
 ******************************************************/

/*@-exportlocal@*/ /* Lint: These are API functions it is ok if they are not all used externally - they will be garbage collected by the linker if needed */

/** Scan result callback function pointer type
 *
 * @param result_ptr  : A pointer to the pointer that indicates where to put the next scan result
 * @param user_data   : User provided data
 * @param status      : Status of scan process
 */
typedef void (*wiced_scan_result_callback_t)( wiced_scan_result_t** result_ptr, void* user_data, wiced_scan_status_t status );

/** Initiates a scan to search for 802.11 networks.
 *
 *  The scan progressively accumulates results over time, and may take between 1 and 10 seconds to complete.
 *  The results of the scan will be individually provided to the callback function.
 *  Note: The callback function will be executed in the context of the WICED thread and so must not perform any
 *  actions that may cause a bus transaction.
 *
 * @param[in]  scan_type     : Specifies whether the scan should be Active, Passive or scan Prohibited channels
 * @param[in]  bss_type      : Specifies whether the scan should search for Infrastructure networks (those using
 *                             an Access Point), Ad-hoc networks, or both types.
 * @param[in]  optional_ssid : If this is non-Null, then the scan will only search for networks using the specified SSID.
 * @param[in]  optional_mac  : If this is non-Null, then the scan will only search for networks where
 *                             the BSSID (MAC address of the Access Point) matches the specified MAC address.
 * @param[in] optional_channel_list    : If this is non-Null, then the scan will only search for networks on the
 *                                       specified channels - array of channel numbers to search, terminated with a zero
 * @param[in] optional_extended_params : If this is non-Null, then the scan will obey the specifications about
 *                                       dwell times and number of probes.
 * @param callback[in]   : the callback function which will receive and process the result data.
 * @param result_ptr[in] : a pointer to a pointer to a result storage structure.
 * @param user_data[in]  : user specific data that will be passed directly to the callback function
 *
 * @note : When scanning specific channels, devices with a strong signal strength on nearby channels may be detected
 * @note : Callback must not use blocking functions, nor use WICED functions, since it is called from the context of the
 *         WWD thread.
 * @note : The callback, result_ptr and user_data variables will be referenced after the function returns.
 *         Those variables must remain valid until the scan is complete.
 *
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_scan( wiced_scan_type_t                              scan_type,
                                   wiced_bss_type_t                               bss_type,
                                   /*@null@*/ const wiced_ssid_t*                 optional_ssid,
                                   /*@null@*/ const wiced_mac_t*                  optional_mac,
                                   /*@null@*/ /*@unique@*/ const uint16_t*        optional_channel_list,
                                   /*@null@*/ const wiced_scan_extended_params_t* optional_extended_params,
                                   wiced_scan_result_callback_t                   callback,
                                   wiced_scan_result_t**                          result_ptr,
                                   /*@null@*/ void*                               user_data,
                                   wwd_interface_t                                interface );

/** Abort a previously issued scan
 *
 * @return    WICED_SUCCESS or WICED_ERROR
 */
extern wwd_result_t wwd_wifi_abort_scan( void );

/** Sets default scan parameters in FW
 *
 * @param[in]  assoc_time    : Specifies dwell time per channel in associated state
 * @param[in]  unassoc_time  : Specifies dwell time per channel in unassociated state
 * @param[in]  passive_time  : Specifies dwell time per channel for passive scanning
 * @param[in]  home_time     : Specifies dwell time for the home channel between channel scans
 * @param[in]  nprobes       : Specifies number of probes per channel
 *
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_set_scan_params( uint32_t assoc_time,
                                              uint32_t unassoc_time,
                                              uint32_t passive_time,
                                              uint32_t home_time,
                                              uint32_t nprobes );

/** Sets default scan parameters in FW
 *
 * @param[out]  assoc_time    : Dwell time per channel in associated state
 * @param[out]  unassoc_time  : Dwell time per channel in unassociated state
 * @param[out]  passive_time  : Dwell time per channel for passive scanning
 * @param[out]  home_time     : Dwell time for the home channel between channel scans
 * @param[out]  nprobes       : Number of probes per channel
 *
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_get_scan_params( uint32_t* assoc_time,
                                              uint32_t* unassoc_time,
                                              uint32_t* passive_time,
                                              uint32_t* home_time,
                                              uint32_t* nprobes );

/** Joins a Wi-Fi network
 *
 * Scans for, associates and authenticates with a Wi-Fi network.
 * On successful return, the system is ready to send data packets.
 *
 * @param[in] ssid       : A null terminated string containing the SSID name of the network to join
 * @param[in] auth_type  : Authentication type:
 *                         - WICED_SECURITY_OPEN           - Open Security
 *                         - WICED_SECURITY_WEP_PSK        - WEP Security with open authentication
 *                         - WICED_SECURITY_WEP_SHARED     - WEP Security with shared authentication
 *                         - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security
 *                         - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher
 *                         - WICED_SECURITY_WPA2_TKIP_PSK  - WPA2 Security using TKIP cipher
 *                         - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers
 * @param[in] security_key : A byte array containing either the cleartext security key for WPA/WPA2
 *                           secured networks, or a pointer to an array of wiced_wep_key_t structures
 *                           for WEP secured networks
 * @param[in] key_length  : The length of the security_key in bytes.
 * @param[in] semaphore   : A user provided semaphore that is flagged when the join is complete
 *
 * @return    WWD_SUCCESS : when the system is joined and ready to send data packets
 *            Error code   : if an error occurred
 */
extern wwd_result_t wwd_wifi_join( const wiced_ssid_t* ssid, wiced_security_t auth_type, const uint8_t* security_key, uint8_t key_length, host_semaphore_type_t* semaphore );


/** Joins a specific Wi-Fi network
 *
 * Associates and authenticates with a specific Wi-Fi access point.
 * On successful return, the system is ready to send data packets.
 *
 * @param[in] ap           : A pointer to a wiced_scan_result_t structure containing AP details
 * @param[in] security_key : A byte array containing either the cleartext security key for WPA/WPA2
 *                           secured networks, or a pointer to an array of wiced_wep_key_t structures
 *                           for WEP secured networks
 * @param[in] key_length   : The length of the security_key in bytes.
 * @param[in] semaphore    : A user provided semaphore that is flagged when the join is complete
 *
 * @return    WWD_SUCCESS : when the system is joined and ready to send data packets
 *            Error code   : if an error occurred
 */
extern wwd_result_t wwd_wifi_join_specific( const wiced_scan_result_t* ap, const uint8_t* security_key, uint8_t key_length, host_semaphore_type_t* semaphore, wwd_interface_t interface );

/** Disassociates from a Wi-Fi network.
 *
 * @return    WWD_SUCCESS : On successful disassociation from the AP
 *            Error code   : If an error occurred
 */
extern wwd_result_t wwd_wifi_leave( wwd_interface_t interface );

/** Deauthenticates a STA which may or may not be associated to SoftAP or Group Owner
 *
 * @param[in] mac       : Pointer to a variable containing the MAC address to which the deauthentication will be sent
 * @param[in] reason    : Deauthentication reason code
 * @param[in] interface : SoftAP interface or P2P interface

 * @return    WWD_SUCCESS : On successful deauthentication of the other STA
 *            WWD_ERROR   : If an error occurred
 */
extern wwd_result_t wwd_wifi_deauth_sta( const wiced_mac_t* mac, wwd_dot11_reason_code_t reason, wwd_interface_t interface );

/** Deauthenticates all client STAs associated to SoftAP or Group Owner
 *
 * @param[in] reason    : Deauthentication reason code
 * @param[in] interface : SoftAP interface or P2P interface

 * @return    WWD_SUCCESS : On successful deauthentication of the other STA
 *            WWD_ERROR   : If an error occurred
 */
extern wwd_result_t wwd_wifi_deauth_all_associated_client_stas( wwd_dot11_reason_code_t reason, wwd_interface_t interface );

/** Retrieves the current Media Access Control (MAC) address
 *  (or Ethernet hardware address) of the 802.11 device
 *
 * @param mac Pointer to a variable that the current MAC address will be written to
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_get_mac_address( wiced_mac_t* mac, wwd_interface_t interface );

/** Retrieves the current Media Access Control (MAC) address
 *  (or Ethernet hardware address) of the 802.11 device
 *  and store it to local cache, so subsequent
 *  wwd_wifi_get_mac_address() be faster.
 *
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_get_and_cache_mac_address( wwd_interface_t interface );

/** ----------------------------------------------------------------------
 *  WARNING : This function is for internal use only!
 *  ----------------------------------------------------------------------
 *  This function sets the current Media Access Control (MAC) address of the
 *  802.11 device.  To override the MAC address in the Wi-Fi OTP or NVRAM add
 *  a global define in the application makefile as shown below. With this define
 *  in place, the MAC address stored in the DCT is used instead of the MAC in the
 *  OTP or NVRAM.
 *
 *  In <WICED-SDK>/App/my_app/my_app.mk add the following global define
 *    GLOBAL_DEFINES := MAC_ADDRESS_SET_BY_HOST
 *  Further information about MAC addresses is available in the following
 *  automatically generated file AFTER building your first application
 *  <WICED-SDK>/generated_mac_address.txt
 *
 *  NOTE:
 *     Ensure Wi-Fi core and network is down before invoking this function.
 *     Refer wiced_wifi_down() API for details.
 *
 * @param[in] mac Wi-Fi MAC address
 * @return    WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_set_mac_address( wiced_mac_t mac );

/** Starts an infrastructure WiFi network
 *
 * @warning If a STA interface is active when this function is called, the softAP will\n
 *          start on the same channel as the STA. It will NOT use the channel provided!
 *
 * @param[in] ssid       : A null terminated string containing the SSID name of the network to join
 * @param[in] auth_type  : Authentication type: \n
 *                         - WICED_SECURITY_OPEN           - Open Security \n
 *                         - WICED_SECURITY_WPA_TKIP_PSK   - WPA Security \n
 *                         - WICED_SECURITY_WPA2_AES_PSK   - WPA2 Security using AES cipher \n
 *                         - WICED_SECURITY_WPA2_MIXED_PSK - WPA2 Security using AES and/or TKIP ciphers \n
 *                         - WEP security is NOT IMPLEMENTED. It is NOT SECURE! \n
 * @param[in] security_key : A byte array containing the cleartext security key for the network
 * @param[in] key_length   : The length of the security_key in bytes.
 * @param[in] channel      : 802.11 channel number
 *
 * @return    WWD_SUCCESS : if successfully creates an AP
 *            Error code   : if an error occurred
 */
extern wwd_result_t wwd_wifi_start_ap( wiced_ssid_t* ssid, wiced_security_t auth_type, /*@unique@*/ const uint8_t* security_key, uint8_t key_length, uint8_t channel );

/** Stops an existing infrastructure WiFi network
 *
 * @return    WWD_SUCCESS : if the AP is successfully stopped
 *            Error code   : if an error occurred
 */
extern wwd_result_t wwd_wifi_stop_ap( void );

/** Determines if a particular interface is ready to transceive ethernet packets
 *
 * @param     Radio interface to check, options are WICED_STA_INTERFACE, WICED_AP_INTERFACE
 * @return    WWD_SUCCESS  : if the interface is ready to transceive ethernet packets
 * @return    WICED_NOTFOUND : no AP with a matching SSID was found
 * @return    WICED_NOT_AUTHENTICATED: a matching AP was found but it won't let you authenticate.
 *                                     This can occur if this device is in the block list on the AP.
 * @return    WICED_NOT_KEYED: the device has authenticated and associated but has not completed the
 *                             key exchange. This can occur if the passphrase is incorrect.
 * @return    Error code    : if the interface is not ready to transceive ethernet packets
 */
extern wwd_result_t wwd_wifi_is_ready_to_transceive( wwd_interface_t interface );

/** Enables powersave mode without regard for throughput reduction
 *
 *  This function enables (legacy) 802.11 PS-Poll mode and should be used
 *  to achieve the lowest power consumption possible when the Wi-Fi device
 *  is primarily passively listening to the network
 *
 * @return @ref wwd_result_t
 */
extern wwd_result_t wwd_wifi_enable_powersave( void );

/* Enables powersave mode while attempting to maximise throughput
 *
 * Network traffic is typically bursty. Reception of a packet often means that another
 * packet will be received shortly afterwards (and vice versa for transmit).
 *
 * In high throughput powersave mode, rather then entering powersave mode immediately
 * after receiving or sending a packet, the WLAN chip waits for a timeout period before
 * returning to sleep.
 *
 * @return  WWD_SUCCESS : if power save mode was successfully enabled
 *          Error code   : if power save mode was not successfully enabled
 *
 * @param[in] return_to_sleep_delay : The variable to set return to sleep delay.*
 *
 * return to sleep delay must be set to a multiple of 10 and not equal to zero.
 */
extern wwd_result_t wwd_wifi_enable_powersave_with_throughput( uint16_t return_to_sleep_delay );


/** Disables 802.11 power save mode
 *
 * @return  WWD_SUCCESS : if power save mode was successfully disabled
 *          Error code   : if power save mode was not successfully disabled
 */
extern wwd_result_t wwd_wifi_disable_powersave( void );

/** Gets the tx power in dBm units
 *
 * @param dbm : The variable to receive the tx power in dbm.
 *
 * @return  WWD_SUCCESS : if successful
 *          Error code   : if not successful
 */
extern wwd_result_t wwd_wifi_get_tx_power( uint8_t* dbm );

/** Sets the tx power in dBm units
 *
 * @param dbm : The desired tx power in dbm. If set to -1 (0xFF) the default value is restored.
 *
 * @return  WWD_SUCCESS : if tx power was successfully set
 *          Error code   : if tx power was not successfully set
 */
extern wwd_result_t wwd_wifi_set_tx_power( uint8_t dbm );

/** Sets the 802.11 powersave listen interval for a Wi-Fi client, and communicates
 *  the listen interval to the Access Point. The listen interval will be set to
 *  (listen_interval x time_unit) seconds.
 *
 *  The default value for the listen interval is 0. With the default value set,
 *  the Wi-Fi device wakes to listen for AP beacons every DTIM period.
 *
 *  If the DTIM listen interval is non-zero, the DTIM listen interval will over ride
 *  the beacon listen interval value.
 *
 *  If it is necessary to set the listen interval sent to the AP to a value other
 *  than the value set by this function, use the additional association listen
 *  interval API : wwd_wifi_set_listen_interval_assoc()
 *
 *  NOTE: This function applies to 802.11 powersave operation. Please read the
 *  WICED powersave application note for further information about the
 *  operation of the 802.11 listen interval.
 *
 * @param listen_interval : The desired beacon listen interval
 * @param time_unit       : The listen interval time unit; options are beacon period or DTIM period.
 *
 * @return  WWD_SUCCESS : If the listen interval was successfully set.
 *          Error code   : If the listen interval was not successfully set.
 */
extern wwd_result_t wwd_wifi_set_listen_interval( uint8_t listen_interval, wiced_listen_interval_time_unit_t time_unit );

/** Sets the 802.11 powersave beacon listen interval communicated to Wi-Fi Access Points
 *
 *  This function is used by Wi-Fi clients to set the value of the beacon
 *  listen interval sent to the AP (in the association request frame) during
 *  the association process.
 *
 *  To set the client listen interval as well, use the wwd_wifi_set_listen_interval() API
 *
 *  This function applies to 802.11 powersave operation. Please read the
 *  WICED powersave application note for further information about the
 *  operation of the 802.11 listen interval.
 *
 * @param listen_interval : The beacon listen interval sent to the AP during association.
 *                          The time unit is specified in multiples of beacon periods.
 *
 * @return  WWD_SUCCESS : if listen interval was successfully set
 *          Error code   : if listen interval was not successfully set
 */
extern wwd_result_t wwd_wifi_set_listen_interval_assoc( uint16_t listen_interval );

/** Gets the current value of all beacon listen interval variables
 *
 * @param listen_interval_beacon : The current value of the listen interval set as a multiple of the beacon period
 * @param listen_interval_dtim   : The current value of the listen interval set as a multiple of the DTIM period
 * @param listen_interval_assoc  : The current value of the listen interval sent to access points in an association request frame
 *
 * @return  WWD_SUCCESS : If all listen interval values are read successfully
 *          Error code   : If at least one of the listen interval values are NOT read successfully
 */
extern wwd_result_t wwd_wifi_get_listen_interval( wiced_listen_interval_t* li );

/** Registers interest in a multicast address
 * Once a multicast address has been registered, all packets detected on the
 * medium destined for that address are forwarded to the host.
 * Otherwise they are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  WWD_SUCCESS : if the address was registered successfully
 *          Error code   : if the address was not registered
 */
extern wwd_result_t wwd_wifi_register_multicast_address( const wiced_mac_t* mac );

/** Registers interest in a multicast address
 * Similar to wwd_wifi_register_multicast_address but able to define interface
 *
 * @param mac      : Ethernet MAC address
 * @param interface: Wireless interface
 *
 * @return  WWD_SUCCESS : if the address was registered successfully
 *          Error code   : if the address was not registered
 */
extern wwd_result_t wwd_wifi_register_multicast_address_for_interface( const wiced_mac_t* mac, wwd_interface_t interface );

/** Unregisters interest in a multicast address
 * Once a multicast address has been unregistered, all packets detected on the
 * medium destined for that address are ignored.
 *
 * @param mac: Ethernet MAC address
 *
 * @return  WWD_SUCCESS : if the address was unregistered successfully
 *          Error code  : if the address was not unregistered
 */
extern wwd_result_t wwd_wifi_unregister_multicast_address( const wiced_mac_t* mac );

/** Unregisters interest in a multicast address
 * Similar to wwd_wifi_unregister_multicast_address but able to define interface.
 *
 * @param mac      : Ethernet MAC address
 * @param interface: Wireless interface
 *
 * @return  WWD_SUCCESS : if the address was unregistered successfully
 *          Error code   : if the address was not unregistered
 */
extern wwd_result_t wwd_wifi_unregister_multicast_address_for_interface( const wiced_mac_t* mac, wwd_interface_t interface );

/** Retrieve the latest RSSI value
 *
 * @param rssi: The location where the RSSI value will be stored
 *
 * @return  WWD_SUCCESS : if the RSSI was succesfully retrieved
 *          Error code  : if the RSSI was not retrieved
 */
extern wwd_result_t wwd_wifi_get_rssi( int32_t* rssi );

/** Retrieve the latest RSSI value of the AP client
 *
 * @param rssi: The location where the RSSI value will be stored
 * @param client_mac_addr: Mac address of the AP client
 *                         Please note that you can get the full list of AP clients
 *                         currently connected to Wiced AP by calling a function
 *                         wwd_wifi_get_associated_client_list
 *
 * @return  WWD_SUCCESS : if the RSSI was succesfully retrieved
 *          Error code   : if the RSSI was not retrieved
 */
extern wwd_result_t wwd_wifi_get_ap_client_rssi( int32_t* rssi, const wiced_mac_t* client_mac_addr );

/** Select the Wi-Fi antenna
 *    antenna = 0 -> select antenna 0
 *    antenna = 1 -> select antenna 1
 *    antenna = 3 -> enable auto antenna selection ie. automatic diversity
 *
 * @param antenna: The antenna configuration to use
 *
 * @return  WWD_SUCCESS : if the antenna selection was successfully set
 *          Error code   : if the antenna selection was not set
 */
extern wwd_result_t wwd_wifi_select_antenna( wiced_antenna_t antenna );

/** Bring down the Wi-Fi core
 *
 *  WARNING / NOTE:
 *     This brings down the Wi-Fi core and existing network connections will be lost.
 *     Re-establish the network by calling wiced_wifi_up() and wiced_network_up().
 *     Refer those APIs for more details.
 *
 * @return  WWD_SUCCESS : if success
 *          Error code  : if fails
 */
extern wwd_result_t wwd_wifi_set_down( void );

/** Brings up the Wi-Fi core
 *
 * @return  WWD_SUCCESS : if success
 *          Error code  : if fails
 */
extern wwd_result_t wwd_wifi_set_up( void );

/** Manage the addition and removal of custom IEs
 *
 * @param interface    : interface on which the operation to be performed
 * @param action       : the action to take (add or remove IE)
 * @param oui          : the oui of the custom IE
 * @param subtype      : the IE sub-type
 * @param data         : a pointer to the buffer that hold the custom IE
 * @param length       : the length of the buffer pointed to by 'data'
 * @param which_packets: a mask of which packets this IE should be included in. See wiced_ie_packet_flag_t
 *
 * @return WWD_SUCCESS : if the custom IE action was successful
 *         Error code  : if the custom IE action failed
 */
extern wwd_result_t wwd_wifi_manage_custom_ie( wwd_interface_t interface, wiced_custom_ie_action_t action, /*@unique@*/ const uint8_t* oui, uint8_t subtype, const void* data, uint16_t length, uint16_t which_packets );

/** Set roam trigger level
 *
 * @param trigger_level : Trigger level in dBm. The Wi-Fi device will search for a new AP to connect to once the \n
 *                        signal from the AP (it is currently associated with) drops below the roam trigger level
 *
 * @return  WWD_SUCCESS : if the roam trigger was successfully set
 *          Error code  : if the roam trigger was not successfully set
 */
extern wwd_result_t wwd_wifi_set_roam_trigger( int32_t trigger_level );

/** Get roam trigger level
 *
 * @param trigger_level  : Trigger level in dBm. Pointer to store current roam trigger level value
 * @return  WWD_SUCCESS  : if the roam trigger was successfully get
 *          Error code   : if the roam trigger was not successfully get
 */
extern wwd_result_t wwd_wifi_get_roam_trigger( int32_t* trigger_level );

/** Set roam trigger delta value
 *
 * @param trigger_delta : Trigger delta is in dBm. After a roaming is triggered - The successful roam will happen \n
 *                        when a target AP with RSSI better than the current serving AP by at least trigger_delta (in dB)
 *
 * @return  WWD_SUCCESS : if the roam trigger delta was successfully set
 *          Error code  : if the roam trigger delta was not successfully set
 */
extern wwd_result_t wwd_wifi_set_roam_delta( int32_t trigger_delta );

/** Get roam trigger delta value
 *
 * @param trigger_delta : Trigger delta is in dBm. Pointer to store the current roam trigger delta value
 * @return  WWD_SUCCESS : if the roam trigger delta was successfully get
 *          Error code  : if the roam trigger delta was not successfully get
 */
extern wwd_result_t wwd_wifi_get_roam_delta( int32_t* trigger_delta );

/** Set roam scan period
 *
 * @param roam_scan_period : Roam scan period is in secs. Updates the partial scan period - for partial scan - Only for STA
 * @return  WWD_SUCCESS    : if the roam scan period was successfully set
 *          Error code     : if the roam scan period was not successfully set
 */
extern wwd_result_t wwd_wifi_set_roam_scan_period( uint32_t roam_scan_period );

/** Get roam scan period
 *
 * @param roam_scan_period : Roam scan period is in secs. Pointer to store the current partial scan period
 * @return  WWD_SUCCESS    : if the roam scan period was successfully get
 *          Error code     : if the roam scan period was not successfully get
 */
extern wwd_result_t wwd_wifi_get_roam_scan_period( uint32_t* roam_scan_period );

/** Turn off roaming
 *
 * @param disable         : Boolean value which if TRUE will turn roaming off and if FALSE will turn roaming on
 *
 * @return  WICED_SUCCESS : if the roaming was successfully turned off
 *          WICED_ERROR   : if the roaming was not successfully turned off
 */
extern wwd_result_t wwd_wifi_turn_off_roam( wiced_bool_t disable );

/** Send a pre-prepared action frame
 *
 * @param action_frame   : A pointer to a pre-prepared action frame structure
 * @param interface      : The interface that is sending the action frame (WWD_STA_INTERFACE, WWD_AP_INTERFACE or WWD_P2P_INTERFACE)
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t wwd_wifi_send_action_frame( const wiced_action_frame_t* action_frame, wwd_interface_t interface );

/** Retrieve the latest STA EDCF AC parameters
 *
 * Retrieve the latest Station (STA) interface enahance distributed
 * coordination function Access Category parameters
 *
 * @param acp: The location where the array of AC parameters will be stored
 *
 * @return  WWD_SUCCESS : if the AC Parameters were successfully retrieved
 *          Error code   : if the AC Parameters were not retrieved
 */
extern wwd_result_t wwd_wifi_get_acparams_sta( wiced_edcf_ac_param_t *acp );

/** Prioritize access category parameters as a function of min and max contention windows and backoff slots
 *
 * @param acp:       Pointer to an array of AC parameters
 * @param priority:  Pointer to a matching array of priority values
 *
 * @return
 */
extern void wwd_wifi_prioritize_acparams( const wiced_edcf_ac_param_t *acp, int *priority );

/** For each traffic priority (0..7) look up the 802.11 Access Category that is mapped to this type
 *  of service and update the TOS map with the priority that the AP actually allows
 *
 * @return  WICED_SUCCESS : if the
 *          WICED_ERROR   : if the AC Parameters were not retrieved
 */
extern wwd_result_t wwd_wifi_update_tos_map( void );

/** Print access category parameters with their priority (1-4, where 4 is highest priority)
 *
 * @param acp:       Pointer to an array of AC parameters
 * @param priority:  Pointer to a matching array of priority values
 *
 * @return
 */
extern void wwd_wifi_edcf_ac_params_print( const wiced_edcf_ac_param_t *acp, const int *priority );


/** Get the current channel on the WLAN radio
 *
 * NOTE: on most WLAN devices this will get the channel for both AP *AND* STA
 *       (since there is only one radio - it cannot be on two channels simulaneously)
 *
 * @param interface : The interface to set
 * @param channel   : pointer which receives the current channel
 *
 * @return  WWD_SUCCESS : if the channel was successfully retrieved
 *          Error code   : if the channel was not successfully retrieved
 */
extern wwd_result_t wwd_wifi_get_channel( wwd_interface_t interface, uint32_t* channel );

/** Set the current channel on the WLAN radio
 *
 * NOTE: on most WLAN devices this will set the channel for both AP *AND* STA
 *       (since there is only one radio - it cannot be on two channels simulaneously)
 *
 * @param interface : The interface to set
 * @param channel   : The desired channel
 *
 * @return  WWD_SUCCESS : if the channel was successfully set
 *          Error code   : if the channel was not successfully set
 */
extern wwd_result_t wwd_wifi_set_channel( wwd_interface_t interface, uint32_t channel );

/** Get the counters for the provided interface
 *
 * @param interface  : The interface from which the counters are requested
 *        counters   : A pointer to the structure where the counter data will be written
 *
 * @return  WWD_SUCCESS : if the counters were successfully read
 *          Error code   : if the counters were not successfully read
 */
extern wwd_result_t wwd_wifi_get_counters( wwd_interface_t interface, wiced_counters_t* counters );

/** Get the maximum number of associations supported by all interfaces (STA and Soft AP)
 *
 * @param max_assoc  : The maximum number of associations supported by the STA and Soft AP interfaces. For example
 *                     if the STA interface is associated then the Soft AP can support (max_assoc - 1)
 *                     associated clients.
 *
 * @return  WICED_SUCCESS : if the maximum number of associated clients was successfully read
 *          WICED_ERROR   : if the maximum number of associated clients was not successfully read
 */
extern wwd_result_t wwd_wifi_get_max_associations( uint32_t* max_assoc );


/** Get the current data rate for the provided interface
 *
 * @param interface  : The interface from which the rate is requested
 *        rate   : A pointer to the uint32_t where the value will be returned in 500Kbits/s units, 0 for auto
 *
 * @return  WWD_SUCCESS : if the rate was successfully read
 *          Error code   : if the rate was not successfully read
 */
extern wwd_result_t wwd_wifi_get_rate( wwd_interface_t interface, uint32_t* rate);

/** Set the legacy (CCK/OFDM) transmit data rate for the provided interface
 *
 * @param interface  : The interface for which the rate is going to be set
 *        rate       : uint32_t where the rate value is given in 500Kbits/s units, 0 for auto
 *
 * @return  WWD_SUCCESS : if the rate was successfully set
 *          Error code   : if the rate was not successfully set
 */
extern wwd_result_t wwd_wifi_set_legacy_rate( wwd_interface_t interface, int32_t rate);

/** Set the MCS index transmit data rate for the provided interface
 *
 * @param interface  : The interface for which the rate is going to be set
 *        mcs        : int32_t where the mcs index is given, -1 for auto
 *        mcsonly    : indicate that only the mcs index should be changed
 *
 * @return  WWD_SUCCESS : if the rate was successfully set
 *          Error code   : if the rate was not successfully set
 */
extern wwd_result_t wwd_wifi_set_mcs_rate( wwd_interface_t interface, int32_t mcs, wiced_bool_t mcsonly);

/** Enable or disable 11n support (support only for pre-11n modes)
 *
 *  NOTE:
 *     Ensure Wi-Fi core and network is down before invoking this function.
 *     Refer to wiced_wifi_down() for more details.
 *
 * @param interface       : The interface for which 11n mode is being controlled. Currently only STA supported
 *        disable         : Boolean value which if TRUE will turn 11n off and if FALSE will turn 11n on
 *
 * @return  WICED_SUCCESS : if the 11n was successfully turned off
 *          WICED_ERROR   : if the 11n was not successfully turned off
 */
extern wwd_result_t wwd_wifi_set_11n_support( wwd_interface_t interface, wiced_11n_support_t value );

/** Set the AMPDU parameters for both Soft AP and STA
 *
 * Sets various AMPDU parameters for Soft AP and STA to ensure that the number of buffers dedicated to AMPDUs does
 * not exceed the resources of the chip. Both Soft AP and STA interfaces must be down.
 *
 * @return  WICED_SUCCESS : if the AMPDU parameters were successfully set
 *          WICED_ERROR   : if the AMPDU parameters were not successfully set
 */
extern wwd_result_t wwd_wifi_set_ampdu_parameters( void );

/** Set the AMPDU Block Ack window size for both Soft AP and STA
 *
 * Sets the AMPDU Block Ack window size for Soft AP and STA. Soft AP and STA interfaces may be up.
 *
 * @param interface  : STA or Soft AP interface.
 *
 * @return  WICED_SUCCESS : if the Block Ack window size was successfully set
 *          WICED_ERROR   : if the Block Ack window size was not successfully set
 */
extern wwd_result_t wwd_wifi_set_block_ack_window_size( wwd_interface_t interface );

/** Get the average PHY noise detected on the antenna. This is valid only after TX.
 *  Defined only on STA interface
 *
 * @param noise : reports average noise
 *
 * @return  WWD_SUCCESS : if success
 *          Error code   : if Link quality was not enabled or not successful
 */
extern wwd_result_t wwd_wifi_get_noise( int32_t *noise );

/** Get the bands supported by the radio chip
 *
 * @param band_list : pointer to a structure that will hold the band list information
 *
 * @return  WWD_SUCCESS : if success
 *          Error code  : if not successful
 */
extern wwd_result_t wwd_wifi_get_supported_band_list( wiced_band_list_t* band_list );

/** Set the preferred band for association by the radio chip
 *  Defined only on STA interface
 *
 * @param band : preferred band (auto, 2.4 GHz or 5 GHz)
 *
 * @return  WWD_SUCCESS : if success
 *          Error code   : if setting the preferred band was not successful
 */
wwd_result_t wwd_wifi_set_preferred_association_band( int32_t band );

/** Get the preferred band for association by the radio chip
 *
 * @param band : pointer to a variable that will hold the band information (auto, 2.4 GHz or 5 GHz)
 *
 * @return  WWD_SUCCESS : if success
 *          Error code  : if not successful
 */
wwd_result_t wwd_wifi_get_preferred_association_band( int32_t* band );

/** Sets HT mode for the given interface
 *
 *  NOTE:
 *     Ensure WiFi core and network is down before invoking this function.
 *     Refer wiced_wifi_down() API for more details.
 *
 * @param interface       : the interface for which HT mode to be changed.
 *        ht_mode         : enumeration value which indicates the HT mode
 *
 * @return  WICED_SUCCESS : if success
 *          Error code    : error code to indicate the type of error, if HT mode could not be successfully set
 */
extern wwd_result_t wwd_wifi_set_ht_mode( wwd_interface_t interface, wiced_ht_mode_t ht_mode );

/** Gets the current HT mode of the given interface
 *
 * @param interface       : the interface for which current HT mode to be identified
 *        ht_mode         : pointers to store the results (i.e., currently configured HT mode)
 *
 * @return  WICED_SUCCESS : if success
 *          Error code    : error code to indicate the type of error, if HT mode could not be successfully get
 */
extern wwd_result_t wwd_wifi_get_ht_mode( wwd_interface_t interface, wiced_ht_mode_t* ht_mode );

/** Gets the BSS index that the given interface is mapped to in Wiced
 *
 * @param interface       : the interface for which to get the BSS index
 *
 * @return  BSS index
 */
extern uint32_t     wwd_get_bss_index( wwd_interface_t interface );

/** Gets the current EAPOL key timeout for the given interface
 *
 * @param interface         : the interface for which we want the EAPOL key timeout
 *        eapol_key_timeout : pointer to store the EAPOL key timeout value
 *
 * @return  WICED_SUCCESS : if success
 *          Error code    : error code to indicate the type of error
 */
extern wwd_result_t wwd_wifi_get_supplicant_eapol_key_timeout( wwd_interface_t interface, int32_t* eapol_key_timeout );

/** Sets the current EAPOL key timeout for the given interface
 *
 * @param interface         : the interface for which we want to set the EAPOL key timeout
 *        eapol_key_timeout : EAPOL key timeout value
 *
 * @return  WICED_SUCCESS : if success
 *          Error code    : error code to indicate the type of error
 */
extern wwd_result_t wwd_wifi_set_supplicant_eapol_key_timeout( wwd_interface_t interface, int32_t eapol_key_timeout );

/*@+exportlocal@*/
/** @} */

/* AP & STA info API */
extern wwd_result_t wwd_wifi_get_associated_client_list( void* client_list_buffer, uint16_t buffer_length );
extern wwd_result_t wwd_wifi_get_ap_info( wiced_bss_info_t* ap_info, wiced_security_t* security );
extern wwd_result_t wwd_wifi_get_bssid( wiced_mac_t* bssid );

/* Monitor Mode API */
extern wwd_result_t wwd_wifi_enable_monitor_mode     ( void );
extern wwd_result_t wwd_wifi_disable_monitor_mode    ( void );
extern wiced_bool_t wwd_wifi_monitor_mode_is_enabled ( void );
extern wwd_result_t wwd_wifi_set_raw_packet_processor( wwd_wifi_raw_packet_processor_t function );

/* Duty cycle control API */
extern wwd_result_t wwd_wifi_set_ofdm_dutycycle( uint8_t  duty_cycle_val );
extern wwd_result_t wwd_wifi_set_cck_dutycycle ( uint8_t  duty_cycle_val );
extern wwd_result_t wwd_wifi_get_ofdm_dutycycle( uint8_t* duty_cycle_val );
extern wwd_result_t wwd_wifi_get_cck_dutycycle ( uint8_t* duty_cycle_val );

/* PMK retrieval API */
extern wwd_result_t wwd_wifi_get_pmk( const char* psk, uint8_t psk_length, char* pmk );

/* Packet filter API */
extern wwd_result_t wwd_wifi_add_packet_filter                 ( const wiced_packet_filter_t* filter_settings );
extern wwd_result_t wwd_wifi_set_packet_filter_mode            ( wiced_packet_filter_mode_t mode );
extern wwd_result_t wwd_wifi_remove_packet_filter              ( uint8_t filter_id );
extern wwd_result_t wwd_wifi_enable_packet_filter              ( uint8_t filter_id );
extern wwd_result_t wwd_wifi_disable_packet_filter             ( uint8_t filter_id );
extern wwd_result_t wwd_wifi_get_packet_filter_stats           ( uint8_t filter_id, wiced_packet_filter_stats_t* stats );
extern wwd_result_t wwd_wifi_clear_packet_filter_stats         ( uint32_t filter_id );
extern wwd_result_t wwd_wifi_get_packet_filters                ( uint32_t max_count, uint32_t offset, wiced_packet_filter_t* list,  uint32_t* count_out );
extern wwd_result_t wwd_wifi_get_packet_filter_mask_and_pattern( uint32_t filter_id, uint32_t max_size, uint8_t* mask, uint8_t* pattern, uint32_t* size_out );

/* These functions are not exposed to the external WICED API */
extern wwd_result_t wwd_wifi_toggle_packet_filter( uint8_t filter_id, wiced_bool_t enable );

/* Network Keep Alive API */
extern wwd_result_t wwd_wifi_add_keep_alive    ( const wiced_keep_alive_packet_t* keep_alive_packet_info );
extern wwd_result_t wwd_wifi_get_keep_alive    ( wiced_keep_alive_packet_t* keep_alive_packet_info );
extern wwd_result_t wwd_wifi_disable_keep_alive( uint8_t id );
/** @endcond */

/** Retrieves the WLAN firmware version
 *
 * @param[out] Pointer to a buffer that version information will be written to
 * @param[in]  Length of the buffer
 * @return     @ref wwd_result_t
 */
extern wwd_result_t wwd_wifi_get_wifi_version( char* version, uint8_t length );

extern wwd_result_t wwd_wifi_enable_minimum_power_consumption( void );

/*
 * APIs to write a bit/word to OTP at the specified bit/word offset. (An OTP word is 16 bits)
 * These APIs work only with the MFG WLAN FW and are not supported on the Production WLAN FW.
 */
extern wwd_result_t wwd_wifi_otp_write_bit( uint16_t bit_offset, uint16_t write_bit );
extern wwd_result_t wwd_wifi_otp_write_word( uint16_t word_offset, uint16_t write_word );

extern wwd_result_t wwd_wifi_test_credentials( wiced_scan_result_t* ap, const uint8_t* security_key, uint8_t key_length );


/* These functions are used for link down event handling while in power save mode */
extern uint8_t  wiced_wifi_get_powersave_mode( void );
extern uint16_t wiced_wifi_get_return_to_sleep_delay( void );


extern wwd_result_t wwd_wifi_read_wlan_log( char* buffer, uint32_t buffer_size );

extern wwd_result_t wwd_wifi_set_passphrase( const uint8_t* security_key, uint8_t key_length, wwd_interface_t interface );

/* Set and get IOVAR parameters */
extern wwd_result_t wwd_wifi_set_iovar_value( const char* iovar, uint32_t  value, wwd_interface_t interface );
extern wwd_result_t wwd_wifi_get_iovar_value( const char* iovar, uint32_t* value, wwd_interface_t interface );
extern wwd_result_t wwd_wifi_set_ioctl_value( uint32_t ioctl, uint32_t  value, wwd_interface_t interface );
extern wwd_result_t wwd_wifi_get_ioctl_value( uint32_t ioctl, uint32_t* value, wwd_interface_t interface );
extern wwd_result_t wwd_wifi_get_revision_info( wwd_interface_t interface, wlc_rev_info_t *buf, uint16_t buflen );

/* 802.11K (Radio Measurement) APIs */
/*----------------------------------*/
/*
 *
 *  This function gets Radio Resource Management Capabilities and parses them and
 *  then passes them to user application to format the data.
 *
 * @param interface                                     : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_capability_ie_t     : The data structure get the different Radio Resource capabilities.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_get_radio_resource_management_capabilities( wwd_interface_t interface, radio_resource_management_capability_ie_t* rrm_cap );

/*
 *
 *  This function sets Radio Resource Management Capabilities in the WLAN firmware.
 *
 * @param interface                                     : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_capability_ie_t     : The data structure to set the different Radio Resource capabilities.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_set_radio_resource_management_capabilities( wwd_interface_t interface, radio_resource_management_capability_ie_t* rrm_cap );


/*
 *
 *  This function send 11k neighbor report measurement request for the particular SSID in the WLAN firmware.
 *
 * @param interface       : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param wiced_ssid_t    : The data structure of the SSID.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_neighbor_req( wwd_interface_t interface, wiced_ssid_t* ssid );

/*
 *
 *  This function sets 11k link measurement request for the particular BSSID in the WLAN firmware.
 *
 * @param interface      : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param wiced_mac_t    : MAC Address of the destination
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_link_management_req( wwd_interface_t interface, wiced_mac_t* ea );

/*
 *
 *  This function sets 11k beacon measurement request in the WLAN firmware.
 *
 * @param interface                                : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_beacon_req_t   : pointer to data structure of rrm_bcn_req_t
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_beacon_req( wwd_interface_t interface, radio_resource_management_beacon_req_t* rrm_bcn_req );

/*
 *
 *  This function sets 11k channel load measurement request in the WLAN firmware.
 *
 * @param interface                       : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_req_t : pointer to data structure of rrm_chload_req
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_channel_load_req( wwd_interface_t interface, radio_resource_management_req_t* rrm_chload_req );

/*
 *
 *  This function sets 11k noise measurement request in the WLAN firmware.
 *
 * @param interface                            : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_req_t      : pointer to data structure of rrm_noise_req
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_noise_req( wwd_interface_t interface, radio_resource_management_req_t* rrm_noise_req );

/*
 *
 *  This function sets 11k frame measurement request in the WLAN firmware.
 *
 * @param interface                            : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_framereq_t : pointer to data structure of rrm_framereq
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_frame_req( wwd_interface_t interface, radio_resource_management_framereq_t* rrm_framereq );

/*
 *
 *  This function sets 11k stat measurement request in the WLAN firmware.
 *
 * @param interface                            : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param radio_resource_management_statreq_t  : pointer to data structure of rrm_statreq
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_stat_req( wwd_interface_t interface, radio_resource_management_statreq_t* rrm_statreq );

/*
 *
 *  This function gets 11k neighbor report list works from the WLAN firmware.
 *
 * @param interface      : WWD_AP_INTERFACE (works only in AP mode)
 * @param uint8_t        : buffer pointer to data structure
 * @param uint16_t       : buffer length
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_neighbor_list( wwd_interface_t interface, uint8_t* buffer, uint16_t buflen );

/*
 *
 *  This function deletes node from 11k neighbor report list
 *
 * @param interface      : WWD_AP_INTERFACE (works only in AP mode)
 * @param wiced_mac_t    : BSSID of the node to be deleted from neighbor report list
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_neighbor_del_neighbor( wwd_interface_t interface, wiced_mac_t* bssid);

/*
 *
 *  This function adds a node to  Neighbor list
 *
 * @param interface           : WWD_AP_INTERFACE (works only in AP mode)
 * @param rrm_nbr_element_t   : pointer to the neighbor element data structure.
 * @param buflen              : buffer length of the neighbor element data.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_radio_resource_management_neighbor_add_neighbor( wwd_interface_t interface, radio_resource_management_nbr_element_t* nbr_elt, uint16_t buflen );


/* 802.11R(Fast BSS Transition) APIs */
/*--------------------------------------*/
/*
 *
 *  This function sets/resets the value of FBT(Fast BSS Transition) Over-the-DS(Distribution System)
 *
 * @param interface       : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param set             : If the value 1 then FBT over the DS is allowed
 *                        : if the value is 0 then FBT over the DS is not allowed (over the air is the only option)
 * @param value           : value of the data.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_fast_bss_transition_over_distribution_system( wwd_interface_t interface, wiced_bool_t set, int* value);

/*
 *
 *  This function  returns the value of WLFBT (1 if Driver 4-way handshake & reassoc (WLFBT)  is enabled 1 and 0 if disabled)
 *
 * @param interface           : WWD_STA_INTERFACE or WWD_AP_INTERFACE
 * @param value               : gets value of the FBT capabilities.
 *
 *
 * @returns : status WWD_SUCCESS or failure
 */
extern wwd_result_t wwd_wifi_fast_bss_transition_capabilities( wwd_interface_t interface, wiced_bool_t* enable );

/** Set a custom WLAN country code
 *
 * @param[in] country_code: Country code information
 * @return     @ref wwd_result_t
 */
extern wwd_result_t wwd_wifi_set_custom_country_code( const wiced_country_info_t* country_code );

/*
 * This function will send a channel switch announcement and switch to the specificed channel at the specified time.
 * @param[in] wiced_chan_switch_t: pointer to channel switch information
 * @param interface              : WWD_AP_INTERFACE (works only in AP mode)
 * @return     @ref wwd_result_t
 */
extern wwd_result_t wwd_wifi_send_csa( const wiced_chan_switch_t* csa, wwd_interface_t interface );

/** RRM report callback function pointer type
 *
 * @param result_ptr  : A pointer to the pointer that indicates where to put the next RRM report
 *
 */
typedef void (*wiced_rrm_report_callback_t)( wwd_rrm_report_t** result_ptr );


/*****************************************************************************/
/** @addtogroup utility       Utility functions
 *  @ingroup wifi
 *  WICED Utility Wi-Fi functions
 *
 *  @{
 */
/*****************************************************************************/

/** Map channel to its band, comparing channel to max 2g channel
 *
 * @param channel     : The channel to map to a band
 *
 * @return                  : WL_CHANSPEC_BAND_2G or WL_CHANSPEC_BAND_5G
 */
extern wl_chanspec_t wwd_channel_to_wl_band( uint32_t channel );

/**
 ******************************************************************************
 * Prints partial details of a scan result on a single line
 *
 * @param[in] record  A pointer to the wiced_scan_result_t record
 *
 */
extern void print_scan_result( wiced_scan_result_t* record );

/** @} */

/**
 ******************************************************************************
 * Resets WiFi driver statistic counters
 */

wwd_result_t wwd_reset_statistics_counters( void );

/**
 ******************************************************************************
 * Starts or stops the WiFi driver Phyrate logging facility.
 * @param[in] a mode selector where 0 = stop, 1 = start TX, 2= start RX
 */

wwd_result_t wwd_phyrate_log( unsigned int mode );

/**
 ******************************************************************************
 * Returns the WiFi driver phyrate statistics sinc the last reset.
 * @param[in] a pointer to the phyrate counts
 * @param[in] size of the phyrate counts buffer
 */

wwd_result_t wwd_get_phyrate_statistics_counters( wiced_phyrate_counters_t *counts_buffer, unsigned int size);

/**
 ******************************************************************************
 * Returns the WiFi driver phyrate log size since the last reset
 * @param[out] size of the phyrate counts buffer
 */

wwd_result_t wwd_get_phyrate_log_size( unsigned int *size);

/**
 ******************************************************************************
 * Returns the WiFi driver phyrate log since the last reset.
 * @param[in] a pointer to the phyrate counts buffer to fill
 */
wwd_result_t wwd_get_phyrate_log( wiced_phyrate_log_t *data);

/**
 ******************************************************************************
 * Returns the WiFi driver statistics counters since the last reset.
 * @param[in] a pointer to the counter staticstics buffer to fill
 */

wwd_result_t wwd_get_counters( wiced_counters_t *data);

/**
 ******************************************************************************
 * Preferred Network Offload functions (pno)
 */

/**
 * Add another preferred network to be searched for in the background.
 * Adds are cumulative and can be called one after another.
 * @param[in] ssid of the network
 * @param[in] security settings for the preferred network
 */

wwd_result_t wwd_wifi_pno_add_network( wiced_ssid_t *ssid, wiced_security_t security );

/**
 * clear added networks and disable pno scanning
 */

wwd_result_t wwd_wifi_pno_clear( void );

/**
 * enable pno scan process now; use previously added networks
 */

wwd_result_t wwd_wifi_pno_start( void );

/**
 * disable pno scan process now; do not clear previously added networks
 */

wwd_result_t wwd_wifi_pno_stop( void );

/**
* Print out an event's information for debugging help
*/
void wwd_log_event( const wwd_event_header_t* event_header, const uint8_t* event_data );

void wwd_wifi_join_cancel(wiced_bool_t called_from_isr);

/*@+exportlocal@*/

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* ifndef INCLUDED_WWD_WIFI_H */


