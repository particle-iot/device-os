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

#include "nx_api.h"
#include "tx_port.h" /* Needed by nx_dhcp.h that follows */
#include "netx_applications/dhcp/nx_dhcp.h"
#include "tls_types.h"
#include "wiced_result.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define IP_HANDLE(interface)   (*wiced_ip_handle[(interface)&3])

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS    (NX_MAX_LISTEN_REQUESTS)
#define WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS            (WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS)

#define SIZE_OF_ARP_ENTRY           sizeof(NX_ARP)

#define IP_STACK_SIZE               (2*1024)
#define ARP_CACHE_SIZE              (6 * SIZE_OF_ARP_ENTRY)
#define DHCP_STACK_SIZE             (1024)

#define WICED_ANY_PORT              (0)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_TCP_DISCONNECT_CALLBACK_INDEX = 0,
    WICED_TCP_RECEIVE_CALLBACK_INDEX    = 1,
    WICED_TCP_CONNECT_CALLBACK_INDEX    = 2,
} wiced_tcp_callback_index_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef NX_UDP_SOCKET    wiced_udp_socket_t;
typedef NX_PACKET        wiced_packet_t;
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
    NX_TCP_SOCKET               socket;
    wiced_tls_simple_context_t* tls_context;
    wiced_bool_t                context_malloced;
    wiced_socket_callback_t     callbacks[3];

} wiced_tcp_socket_t;

typedef struct
{
    wiced_tcp_socket_t  socket[WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS];
    int                 interface;
    uint16_t            port;
} wiced_tcp_server_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/*
 * Note: These objects are for internal use only!
 */
extern NX_IP*         wiced_ip_handle   [3];
extern NX_PACKET_POOL wiced_packet_pools[2]; /* 0=TX, 1=RX */

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /*extern "C" */
#endif
