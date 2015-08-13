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

#include "nx_api.h"
#include "tx_port.h" /* Needed by nx_dhcp.h that follows */
#include "netx_applications/dhcp/nx_dhcp.h"
#include "netx_applications/auto_ip/nx_auto_ip.h"
#include "wiced_result.h"
#include "tls_types.h"
#include "linked_list.h"
#include "wwd_network_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define IP_HANDLE(interface)     (*wiced_ip_handle[( interface )&3])
#define WICED_LINK_CHECK( interface )              { if ( !wiced_network_interface_is_up( &IP_HANDLE(interface) ) ){ return WICED_NOTUP; }}
#define WICED_LINK_CHECK_TCP_SOCKET( socket_in )   { if ( (socket_in)->socket.nx_tcp_socket_ip_ptr->nx_ip_driver_link_up == 0 ){ return WICED_NOTUP; }}
#define WICED_LINK_CHECK_UDP_SOCKET( socket_in )   { if ( (socket_in)->socket.nx_udp_socket_ip_ptr->nx_ip_driver_link_up == 0 ){ return WICED_NOTUP; }}

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS    (NX_MAX_LISTEN_REQUESTS)
#define WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS            (WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS)

#define SIZE_OF_ARP_ENTRY           sizeof(NX_ARP)

#ifdef DEBUG
#define IP_STACK_SIZE               (3*1024)
#else
#define IP_STACK_SIZE               (2*1024)
#endif
#define ARP_CACHE_SIZE              (6 * SIZE_OF_ARP_ENTRY)
#define DHCP_STACK_SIZE             (1280)

#define WICED_ANY_PORT              (0)
#define WICED_NETWORK_MTU_SIZE      (WICED_LINK_MTU)
#define WICED_SOCKET_MAGIC_NUMBER   (0xfeedbead)

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

typedef NX_PACKET wiced_packet_t;

/******************************************************
 *                    Structures
 ******************************************************/

/* NOTE: Don't change the order or the fields within this wiced_tcp_socket_t and wiced_udp_socket_t.
 * Socket must always be the first field.
 * WICED TCP/IP layer uses socket magic number to differentiate between a native NX socket or a WICED socket.
 * This allows access to WICED socket object from a NX callback without having to store its pointer globally.
 */
typedef struct
{
    NX_TCP_SOCKET               socket;
    uint32_t                    socket_magic_number;
    wiced_tls_simple_context_t* tls_context;
    wiced_bool_t                context_malloced;
    uint32_t                    callbacks[3];
    void*                       arg;
} wiced_tcp_socket_t;

typedef struct
{
    NX_UDP_SOCKET socket;
    uint32_t      socket_magic_number;
    uint32_t      receive_callback;
    void*         arg;
} wiced_udp_socket_t;

typedef struct
{
    linked_list_t socket_list;
    int           interface;
    uint16_t      port;
} wiced_tcp_server_t;

typedef struct
{
    linked_list_node_t socket_node;
    wiced_tcp_socket_t socket;
} wiced_tcp_server_socket_t;

typedef wiced_result_t (*wiced_tcp_socket_callback_t)( wiced_tcp_socket_t* socket, void* arg );
typedef wiced_result_t (*wiced_udp_socket_callback_t)( wiced_udp_socket_t* socket, void* arg );

/******************************************************
 *                 Global Variables
 ******************************************************/

/*
 * Note: These objects are for internal use only!
 */
extern NX_IP*         wiced_ip_handle   [4];
extern NX_PACKET_POOL wiced_packet_pools[2]; /* 0=TX, 1=RX */

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wiced_bool_t wiced_network_interface_is_up( NX_IP* ip_handle );

#ifdef __cplusplus
} /*extern "C" */
#endif
