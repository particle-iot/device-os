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

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_TCP_PAYLOAD_SIZE    ( WICED_PAYLOAD_MTU - TCP_HLEN - IP_HLEN - WICED_PHYSICAL_HEADER )
#define MAX_UDP_PAYLOAD_SIZE    ( WICED_PAYLOAD_MTU - UDP_HLEN - IP_HLEN - WICED_PHYSICAL_HEADER )
#define MAX_IP_PAYLOAD_SIZE     ( WICED_PAYLOAD_MTU - IP_HLEN - WICED_PHYSICAL_HEADER )


#define IP_STACK_SIZE               (4*1024)
#define DHCP_STACK_SIZE             (1024)

#define WICED_ANY_PORT              (0)

#define wiced_packet_pools          (NULL)


#define IP_HANDLE(interface)   (*wiced_ip_handle[(interface)&3])

#define WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS    (5)
#define WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS            (5)

extern wiced_result_t lwip_to_wiced_err[];
#define LWIP_TO_WICED_ERR( lwip_err )  ((lwip_err >= ERR_ISCONN)? lwip_to_wiced_err[ -lwip_err ] : WICED_UNKNOWN_NETWORK_STACK_ERROR )

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

typedef struct netbuf       wiced_packet_t;
typedef wiced_result_t (*wiced_socket_callback_t)( void* socket );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_tls_context_type_t context_type;
    wiced_tls_context_t      context;
    wiced_tls_session_t      session;
    wiced_packet_t*          temp_packet;
} wiced_tls_simple_context_t;

typedef struct
{
    wiced_tls_context_type_t context_type;
    wiced_tls_context_t      context;
    wiced_tls_session_t      session;
    wiced_packet_t*          temp_packet;
    wiced_tls_certificate_t  certificate;
    wiced_tls_key_t          key;
} wiced_tls_advanced_context_t;

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
    wiced_socket_callback_t     callbacks[3];
} wiced_tcp_socket_t;

typedef struct
{
    wiced_tcp_socket_t   listen_socket;
    wiced_tcp_socket_t   accept_socket [WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS];
    wiced_socket_state_t accept_socket_state [WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS];
    int                  data_pending_on_socket;
    uint16_t             port;
} wiced_tcp_server_t;

typedef wiced_tcp_socket_t  wiced_udp_socket_t;

typedef struct
{
    wiced_tcp_socket_type_t  type;
    int                      socket;
    wiced_tls_context_t      context;
    wiced_tls_session_t      session;
    wiced_tls_certificate_t* certificate;
    wiced_tls_key_t*         key;
} wiced_tls_socket_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Note: These objects are for internal use only! */
extern xTaskHandle     wiced_thread_handle;
extern struct netif*   wiced_ip_handle[3];
extern struct dhcp     wiced_dhcp_handle;

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
