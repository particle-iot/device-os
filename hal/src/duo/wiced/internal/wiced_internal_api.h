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

#include "wiced_management.h"
#include "wiced_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_TO_WWD_INTERFACE( interface )        ( wwd_interface_t )( (interface)&3 )
#define SET_IP_NETWORK_INITED(interface, status)   (ip_networking_inited[(interface)&3] = status)

#ifdef WICED_USE_ETHERNET_INTERFACE
#define LINK_UP_CALLBACKS_LIST( interface )    ( interface == WICED_STA_INTERFACE ? link_up_callbacks_wireless : link_up_callbacks_ethernet )
#define LINK_DOWN_CALLBACKS_LIST( interface )  ( interface == WICED_STA_INTERFACE ? link_down_callbacks_wireless : link_down_callbacks_ethernet )
#else
#define LINK_UP_CALLBACKS_LIST( interface )    ( interface == WICED_STA_INTERFACE ? link_up_callbacks_wireless : link_up_callbacks_wireless )
#define LINK_DOWN_CALLBACKS_LIST( interface )  ( interface == WICED_STA_INTERFACE ? link_down_callbacks_wireless : link_down_callbacks_wireless )
#endif


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

typedef struct
{
    uint16_t          received_byte_count;
    uint8_t           received_cooee_data[512];
    wiced_semaphore_t sniff_complete;
    wiced_mac_t         initiator_mac;
    wiced_mac_t         ap_bssid;
    wiced_bool_t      wiced_cooee_complete;
    uint16_t          size_of_zero_data_packet;
    uint32_t          received_segment_bitmap[32];
    wiced_bool_t      scan_complete;
    uint8_t*          user_processed_data;
} wiced_cooee_workspace_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern wiced_bool_t                  ip_networking_inited[];
extern wiced_mutex_t                 link_subscribe_mutex;

extern wiced_network_link_callback_t link_up_callbacks_wireless[WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS];
extern wiced_network_link_callback_t link_down_callbacks_wireless[WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS];
#ifdef WICED_USE_ETHERNET_INTERFACE
extern wiced_network_link_callback_t link_up_callbacks_ethernet[WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS];
extern wiced_network_link_callback_t link_down_callbacks_ethernet[WICED_MAXIMUM_LINK_CALLBACK_SUBSCRIPTIONS];
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/

/* WICED <-> Platform API */
extern wiced_result_t wiced_platform_init( void );

/* WICED <-> Network API */
extern wiced_result_t wiced_join_ap       ( void );
extern wiced_result_t wiced_leave_ap      ( wiced_interface_t interface );
extern wiced_result_t wiced_join_ap_specific( wiced_ap_info_t* details, uint8_t security_key_length, const char security_key[ 64 ] );
extern wiced_result_t wiced_start_ap      ( wiced_ssid_t* ssid, wiced_security_t security, const char* key, uint8_t channel);
extern wiced_result_t wiced_stop_ap       ( void );
extern wiced_result_t wiced_ip_up         ( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings );
extern wiced_result_t wiced_ip_up_cancellable  ( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings, volatile uint8_t* cancel_flag );

extern volatile uint8_t wiced_network_up_cancel;

extern wiced_result_t wiced_ip_down       ( wiced_interface_t interface );
extern wiced_bool_t IP_NETWORK_IS_INITED( wiced_interface_t interface );

/* WICED <-> Network Link Management API */
/* NOTE:
 * The notify link functions below are called from within the context of the WICED thread
 * The link handler functions below are called from within the context of the Network Worker thread */
extern void           wiced_network_notify_link_up     ( wiced_interface_t interface );
extern void           wiced_network_notify_link_down   ( wiced_interface_t interface );
extern wiced_result_t wiced_wireless_link_down_handler ( void* arg );
extern wiced_result_t wiced_wireless_link_up_handler   ( void* arg );
extern wiced_result_t wiced_wireless_link_renew_handler( void* arg );
#ifdef WICED_USE_ETHERNET_INTERFACE
extern wiced_result_t wiced_ethernet_link_down_handler ( void );
extern wiced_result_t wiced_ethernet_link_up_handler   ( void );
#endif

/* Wiced Cooee API*/
extern wiced_result_t wiced_wifi_cooee( wiced_cooee_workspace_t* workspace );

/* TLS helper function to do TCP without involving TLS context */
wiced_result_t network_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t*  packet );
wiced_result_t network_tcp_send_buffer( wiced_tcp_socket_t* socket, const void* buffer, uint16_t* length, wiced_tcp_send_flags_t flags, uint32_t timeout );
wiced_result_t network_tcp_receive( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );
wiced_result_t network_udp_receive( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );

void system_monitor_thread_main( wiced_thread_arg_t arg );

wiced_result_t internal_defer_tcp_callback_to_wiced_network_thread( wiced_tcp_socket_t* socket, wiced_tcp_socket_callback_t callback  );
wiced_result_t internal_defer_udp_callback_to_wiced_network_thread( wiced_udp_socket_t* socket );

#ifdef __cplusplus
} /*extern "C" */
#endif
