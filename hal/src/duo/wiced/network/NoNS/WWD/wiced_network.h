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
#if 0
#include "nx_api.h"
#include "tx_port.h" /* Needed by nx_dhcp.h that follows */
#include "netx_applications/dhcp/nx_dhcp.h"
#endif
#include "tls_types.h"
#include "dtls_types.h"
#include "wiced_result.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define IP_HANDLE(interface)   (wiced_ip_handle[(interface) & 3])

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_MAXIMUM_NUMBER_OF_SOCKETS_WITH_CALLBACKS    (1)
#define WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS            (1)

#define SIZE_OF_ARP_ENTRY           sizeof(1)

#define IP_STACK_SIZE               (2 * 1024)
#define ARP_CACHE_SIZE              (6 * SIZE_OF_ARP_ENTRY)
#define DHCP_STACK_SIZE             (1024)

#define WICED_ANY_PORT              (0)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_SOCKET_ERROR
} wiced_socket_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct
{
    int dummy;
}NOOS_DUMMY;
typedef NOOS_DUMMY    wiced_packet_t;

//typedef NOOS_DUMMY wiced_tls_context_type_t;
//typedef NOOS_DUMMY wiced_tls_context_t;
//typedef NOOS_DUMMY wiced_tls_session_t;
//typedef NOOS_DUMMY wiced_tls_certificate_t;
//typedef NOOS_DUMMY  wiced_tls_endpoint_type_t;
typedef NOOS_DUMMY NOOS_TCP_SOCKET;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_dtls_context_t*       dtls_context;
} wiced_udp_socket_t;

typedef struct
{
    NOOS_TCP_SOCKET             socket;
    wiced_tls_context_t*        tls_context;
    wiced_bool_t                context_malloced;
} wiced_tcp_socket_t;

typedef struct
{
    wiced_tcp_socket_t  socket[WICED_MAXIMUM_NUMBER_OF_SERVER_SOCKETS];
    int                 interface;
    uint16_t            port;
} wiced_tcp_server_t;

typedef NOOS_DUMMY wiced_tcp_socket_callback_t;
typedef NOOS_DUMMY wiced_udp_socket_callback_t;

/******************************************************
 *                 Global Variables
 ******************************************************/
typedef struct
{
    int dummy;
}NOOS_IP;
typedef struct
{
    int dummy;
}NOOS_PACKET_POOL;
/*
 * Note: These objects are for internal use only!
 */
extern NOOS_IP          wiced_ip_handle       [3];
extern NOOS_PACKET_POOL wiced_packet_pools    [2]; /* 0=TX, 1=RX */

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /*extern "C" */
#endif
