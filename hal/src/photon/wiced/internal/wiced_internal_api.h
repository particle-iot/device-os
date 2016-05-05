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
#pragma once

#include "wiced_management.h"
#include "wiced_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WICED_TO_WWD_INTERFACE( interface )        ((interface)&3)
#define IP_NETWORK_IS_INITED(interface)            (ip_networking_inited[(interface)&3] == WICED_TRUE)
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

/* WICED <-> RTOS API */
extern wiced_result_t wiced_rtos_init  ( void );
extern wiced_result_t wiced_rtos_deinit( void );

/* WICED <-> Network API */
extern wiced_result_t wiced_network_init  ( void );
extern wiced_result_t wiced_network_deinit( void );
extern wiced_result_t wiced_join_ap       ( void );
extern wiced_result_t wiced_leave_ap      ( wiced_interface_t interface );
extern wiced_result_t wiced_join_ap_specific( wiced_ap_info_t* details, uint8_t security_key_length, const char security_key[ 64 ] );
extern wiced_result_t wiced_start_ap      ( wiced_ssid_t* ssid, wiced_security_t security, const char* key, uint8_t channel);
extern wiced_result_t wiced_stop_ap       ( void );
extern wiced_result_t wiced_ip_up         ( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings );
extern wiced_result_t wiced_ip_up_cancellable  ( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings, volatile uint8_t* cancel_flag );

extern volatile uint8_t wiced_network_up_cancel;

extern wiced_result_t wiced_ip_down       ( wiced_interface_t interface );

/* WICED <-> Network Link Management API */
/* NOTE:
 * The notify link functions below are called from within the context of the WICED thread
 * The link handler functions below are called from within the context of the Network Worker thread */
extern void           wiced_network_notify_link_up    ( wiced_interface_t interface );
extern void           wiced_network_notify_link_down  ( wiced_interface_t interface );
extern wiced_result_t wiced_wireless_link_down_handler ( void* arg );
extern wiced_result_t wiced_wireless_link_up_handler   ( void* arg );
extern wiced_result_t wiced_wireless_link_renew_handler( void );
#ifdef WICED_USE_ETHERNET_INTERFACE
extern wiced_result_t wiced_ethernet_link_down_handler ( void );
extern wiced_result_t wiced_ethernet_link_up_handler   ( void );
#endif

/* Wiced Cooee API*/
extern wiced_result_t wiced_wifi_cooee( wiced_cooee_workspace_t* workspace );

/* Entry point for user Application */
extern void application_start          ( void );

/* TLS helper function to do TCP without involving TLS context */
wiced_result_t network_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t*  packet );
wiced_result_t network_tcp_receive( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );

#ifdef __cplusplus
} /*extern "C" */
#endif
