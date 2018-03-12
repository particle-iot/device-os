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

#include "nx_api.h"
#include "tx_port.h" /* Needed by nx_dhcp.h that follows */
#include "netx_applications/dhcp/nx_dhcp.h"
#include "netx_applications/auto_ip/nx_auto_ip.h"
#include "wiced_result.h"
#include "tls_types.h"
#include "dtls_types.h"
#include "linked_list.h"
#include "wwd_network_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define IP_HANDLE(interface)                       (*wiced_ip_handle[( interface )&3])
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
#define IP_STACK_SIZE                          (3*1024)
#else
#define IP_STACK_SIZE                          (2*1024)
#endif
#define ARP_CACHE_SIZE                         (6 * SIZE_OF_ARP_ENTRY)
#define DHCP_STACK_SIZE                        (1280)

#define WICED_ANY_PORT                         (0)
#define WICED_NETWORK_MTU_SIZE                 (WICED_LINK_MTU)
#define WICED_SOCKET_MAGIC_NUMBER              (0xfeedbead)
#define WICED_MAXIMUM_SEGMENT_SIZE( socket )   MIN(socket->socket.nx_tcp_socket_mss, socket->socket.nx_tcp_socket_connect_mss)


/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef NX_PACKET wiced_packet_t;

typedef enum
{
    WICED_SOCKET_CLOSED,
    WICED_SOCKET_CLOSING,
    WICED_SOCKET_CONNECTING,
    WICED_SOCKET_CONNECTED,
    WICED_SOCKET_DATA_PENDING,
    WICED_SOCKET_LISTEN,
    WICED_SOCKET_ERROR
} wiced_socket_state_t;

typedef struct wiced_packet_pool_s
{
    NX_PACKET_POOL pool;
} wiced_packet_pool_t;

/******************************************************
 *                    Structures
 ******************************************************/
/* These should be in wiced_tcpip.h but are needed by wiced_tcp_socket_t, which would cause a circular include chain */
typedef struct wiced_tcp_socket_struct wiced_tcp_socket_t;
typedef struct wiced_udp_socket_struct wiced_udp_socket_t;

typedef wiced_result_t (*wiced_tcp_socket_callback_t)( wiced_tcp_socket_t* socket, void* arg );
typedef wiced_result_t (*wiced_udp_socket_callback_t)( wiced_udp_socket_t* socket, void* arg );


/* NOTE: Don't change the order or the fields within this wiced_tcp_socket_t and wiced_udp_socket_t.
 * Socket must always be the first field.
 * WICED TCP/IP layer uses socket magic number to differentiate between a native NX socket or a WICED socket.
 * This allows access to WICED socket object from a NX callback without having to store its pointer globally.
 */

struct wiced_tcp_socket_struct
{
    NX_TCP_SOCKET        socket;
    uint32_t             socket_magic_number;
    wiced_tls_context_t* tls_context;
    wiced_bool_t         context_malloced;
    struct
    {
        wiced_tcp_socket_callback_t disconnect;
        wiced_tcp_socket_callback_t receive;
        wiced_tcp_socket_callback_t connect;
    } callbacks;
    void*                callback_arg;
};

struct wiced_udp_socket_struct
{
    NX_UDP_SOCKET               socket;
    uint32_t                    socket_magic_number;
    wiced_dtls_context_t*       dtls_context;
    wiced_bool_t                context_malloced;
    wiced_udp_socket_callback_t receive_callback;
    void*                       callback_arg;
};

typedef struct
{
    linked_list_t         socket_list;
    int                   interface;
    uint16_t              port;
    wiced_tls_identity_t* tls_identity;
} wiced_tcp_server_t;

typedef struct
{
    linked_list_node_t socket_node;
    wiced_tcp_socket_t socket;
} wiced_tcp_server_socket_t;

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
