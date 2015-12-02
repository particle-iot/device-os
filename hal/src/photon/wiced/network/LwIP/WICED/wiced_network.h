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

#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tls_types.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "wiced_result.h"
#include "wiced_wifi.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define LWIP_TO_WICED_ERR( lwip_err )  ((lwip_err >= ERR_ISCONN)? lwip_to_wiced_result[ -lwip_err ] : WICED_UNKNOWN_NETWORK_STACK_ERROR )

#define IP_HANDLE(interface)   (*wiced_ip_handle[(interface)&3])

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_TCP_PAYLOAD_SIZE    ( WICED_PAYLOAD_MTU - TCP_HLEN - IP_HLEN - WICED_PHYSICAL_HEADER )
#define MAX_UDP_PAYLOAD_SIZE    ( WICED_PAYLOAD_MTU - UDP_HLEN - IP_HLEN - WICED_PHYSICAL_HEADER )
#define MAX_IP_PAYLOAD_SIZE     ( WICED_PAYLOAD_MTU - IP_HLEN - WICED_PHYSICAL_HEADER )


#define IP_STACK_SIZE               (4*1024)
#define DHCP_STACK_SIZE             (1280)

#define WICED_ANY_PORT              (0)

#define wiced_packet_pools          (NULL)

#define WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS    (5)
#define WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS            (MEMP_NUM_NETCONN)

#define WICED_LINK_CHECK( interface )      do { if ( netif_is_up( &IP_HANDLE(interface) ) != 1){ return WICED_NOTUP; }} while(0)
#define WICED_LINK_CHECK_TCP_SOCKET( socket_in )   WICED_LINK_CHECK((socket_in)->interface)
#define WICED_LINK_CHECK_UDP_SOCKET( socket_in )   WICED_LINK_CHECK((socket_in)->interface)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_TCP_STANDARD_SOCKET,
    WICED_TCP_SECURE_SOCKET,
} wiced_tcp_socket_type_t;

typedef enum
{
    WICED_SOCKET_CLOSED,
    WICED_SOCKET_CLOSING,
    WICED_SOCKET_CONNECTING,
    WICED_SOCKET_CONNECTED,
    WICED_SOCKET_DATA_PENDING,
    WICED_SOCKET_ERROR
} wiced_socket_state_t;

typedef enum
{
    WICED_TCP_DISCONNECT_CALLBACK_INDEX = 0,
    WICED_TCP_RECEIVE_CALLBACK_INDEX    = 1,
    WICED_TCP_CONNECT_CALLBACK_INDEX    = 2,
} wiced_tcp_callback_index_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct netbuf wiced_packet_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_tcp_socket_type_t     type;
    int                         socket;
    struct netconn*             conn_handler;
    struct netconn*             accept_handler;
    ip_addr_t                   local_ip_addr;
    wiced_bool_t                is_bound;
    int                         interface;
    wiced_tls_simple_context_t* tls_context;
    wiced_bool_t                context_malloced;
    uint32_t                    callbacks[3];
    void*                       arg;
} wiced_tcp_socket_t;

typedef struct
{
    wiced_tcp_socket_t   listen_socket;
    wiced_tcp_socket_t   accept_socket       [WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS];
    wiced_socket_state_t accept_socket_state [WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS];
    int                  data_pending_on_socket;
    uint16_t             port;
} wiced_tcp_server_t;

typedef struct
{
    int             socket;
    struct netconn* conn_handler;
    struct netconn* accept_handler;
    ip_addr_t       local_ip_addr;
    wiced_bool_t    is_bound;
    int             interface;
    uint32_t        receive_callback;
    void*           arg;
} wiced_udp_socket_t;

typedef struct
{
    wiced_tcp_socket_type_t  type;
    int                      socket;
    wiced_tls_context_t      context;
    wiced_tls_session_t      session;
    wiced_tls_certificate_t* certificate;
    wiced_tls_key_t*         key;
} wiced_tls_socket_t;

typedef wiced_result_t (*wiced_tcp_socket_callback_t)( wiced_tcp_socket_t* socket, void* arg );
typedef wiced_result_t (*wiced_udp_socket_callback_t)( wiced_udp_socket_t* socket, void* arg );

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Note: These objects are for internal use only! */
extern xTaskHandle     wiced_thread_handle;
extern struct netif*   wiced_ip_handle[3];
extern struct dhcp     wiced_dhcp_handle;

extern wiced_result_t lwip_to_wiced_result[];

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
