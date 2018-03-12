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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tls_types.h"
#include "dtls_types.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp_impl.h"
#include "arch/sys_arch.h"
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
#define WICED_MAXIMUM_SEGMENT_SIZE( socket_in )    MAX_TCP_PAYLOAD_SIZE

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
    WICED_SOCKET_LISTEN,
    WICED_SOCKET_ERROR
} wiced_socket_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct netbuf wiced_packet_t;

/******************************************************
 *                    Structures
 ******************************************************/
/* These should be in wiced_tcpip.h but are needed by wiced_tcp_socket_t, which would cause a circular include chain */
typedef struct wiced_tcp_socket_struct wiced_tcp_socket_t;
typedef struct wiced_udp_socket_struct wiced_udp_socket_t;

typedef wiced_result_t (*wiced_tcp_socket_callback_t)( wiced_tcp_socket_t* socket, void* arg );
typedef wiced_result_t (*wiced_udp_socket_callback_t)( wiced_udp_socket_t* socket, void* arg );

struct wiced_tcp_socket_struct
{
    wiced_tcp_socket_type_t     type;
    int                         socket;
    struct netconn*             conn_handler;
    struct netconn*             accept_handler;
    ip_addr_t                   local_ip_addr;
    wiced_bool_t                is_bound;
    int                         interface;
    wiced_tls_context_t*        tls_context;
    wiced_bool_t                context_malloced;
    struct
    {
        wiced_tcp_socket_callback_t disconnect;
        wiced_tcp_socket_callback_t receive;
        wiced_tcp_socket_callback_t connect;
    } callbacks;
    void*                       callback_arg;
    wiced_socket_state_t        socket_state; /* internal LwIP socket states do not seem to work correctly */
};

typedef struct
{
    wiced_tcp_socket_t   listen_socket;
    wiced_tcp_socket_t   accept_socket       [WICED_MAXIMUM_NUMBER_OF_ACCEPT_SOCKETS];
    int                  data_pending_on_socket;
    uint16_t             port;
    wiced_tls_identity_t* tls_identity;
} wiced_tcp_server_t;

struct wiced_udp_socket_struct
{
    int                         socket;
    struct netconn*             conn_handler;
    struct netconn*             accept_handler;
    ip_addr_t                   local_ip_addr;
    wiced_bool_t                is_bound;
    int                         interface;
    wiced_dtls_context_t*       dtls_context;
    wiced_bool_t                context_malloced;
    wiced_udp_socket_callback_t receive_callback;
    void*                       callback_arg;
};

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
extern sys_thread_t    wiced_thread_handle;
extern struct netif*   wiced_ip_handle[3];
extern struct dhcp     wiced_dhcp_handle;

extern wiced_result_t  lwip_to_wiced_result[];
extern wiced_mutex_t   lwip_send_interface_mutex;

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
