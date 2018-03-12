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
 *  Defines functions to communicate over the IP network
 */

#pragma once

#include "wiced_utilities.h"
#include "network/wwd_network_interface.h"
#include "wiced_network.h"
#include <limits.h>
#include "wiced_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                Macros
 ******************************************************/

/* For wiced_tcp_bind - if the local port number is irrelevant */
#define WICED_ANY_PORT    (0)

#define INITIALISER_IPV4_ADDRESS( addr_var, addr_val )  addr_var = { WICED_IPV4, { .v4 = (uint32_t)(addr_val) } }
#define SET_IPV4_ADDRESS( addr_var, addr_val )          (((addr_var).version = WICED_IPV4),((addr_var).ip.v4 = (uint32_t)(addr_val)))
#define GET_IPV4_ADDRESS( addr_var )                    ((addr_var).ip.v4)
#define WICED_IP_BROADCAST                              (&wiced_ip_broadcast)
#define MAKE_IPV4_ADDRESS(a, b, c, d)                   ((((uint32_t) a) << 24) | (((uint32_t) b) << 16) | (((uint32_t) c) << 8) | ((uint32_t) d))
#define GET_IPV6_ADDRESS( addr_var )                    ((uint32_t*)((addr_var).ip.v6))
#define SET_IPV6_ADDRESS( addr_var, addr_val )          { \
                                                            uint32_t _value[4] = addr_val; \
                                                            (addr_var).version = WICED_IPV6; \
                                                            (addr_var).ip.v6[0] = _value[0];  \
                                                            (addr_var).ip.v6[1] = _value[1];  \
                                                            (addr_var).ip.v6[2] = _value[2];  \
                                                            (addr_var).ip.v6[3] = _value[3];  \
                                                        }
#define MAKE_IPV6_ADDRESS(a, b, c, d, e, f, g, h)       { \
                                                            (((((uint32_t) (a)) << 16) & (uint32_t)0xFFFF0000UL) | ((uint32_t)(b) & (uint32_t)0x0000FFFFUL)), \
                                                            (((((uint32_t) (c)) << 16) & (uint32_t)0xFFFF0000UL) | ((uint32_t)(d) & (uint32_t)0x0000FFFFUL)), \
                                                            (((((uint32_t) (e)) << 16) & (uint32_t)0xFFFF0000UL) | ((uint32_t)(f) & (uint32_t)0x0000FFFFUL)), \
                                                            (((((uint32_t) (g)) << 16) & (uint32_t)0xFFFF0000UL) | ((uint32_t)(h) & (uint32_t)0x0000FFFFUL))  \
                                                        }


/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*wiced_ip_address_change_callback_t)( void* arg );
typedef wiced_result_t (*wiced_tcp_stream_write_callback_t)( void* tcp_stream, const void* data, uint32_t data_length );

typedef struct wiced_packet_pool_s* wiced_packet_pool_ref;

/******************************************************
 *            Enumerations
 ******************************************************/

/**
 * IP Version
 */
typedef enum
{
    WICED_IPV4 = 4,
    WICED_IPV6 = 6,
    WICED_INVALID_IP = INT_MAX
} wiced_ip_version_t;

/**
 * IPv6 Address Type
 */
typedef enum
{
    IPv6_LINK_LOCAL_ADDRESS,
    IPv6_GLOBAL_ADDRESS,
} wiced_ipv6_address_type_t;

/**
 * TCP Callback Events
 */
typedef enum
{
    WICED_TCP_DISCONNECTED_EVENT = (1 << 0),
    WICED_TCP_RECEIVE_EVENT      = (1 << 1),
    WICED_TCP_CONNECTED_EVENT    = (1 << 2),
} wiced_tcp_event_t;

typedef enum
{
    WICED_TCP_SHUT_RD   = 1,
    WICED_TCP_SHUT_WR   = 2,
    WICED_TCP_SHUT_RDWR = 3,
} wiced_tcp_shutdown_flags_t;

typedef enum
{
    WICED_TCP_SEND_FLAG_NONE     = 0x00,
    WICED_TCP_SEND_FLAG_NONBLOCK = 0x01
} wiced_tcp_send_flags_t;

/**
 * Packet type for network packet allocation requests.
 */
typedef enum
{
    WICED_PACKET_TYPE_RAW,      /* No space reserved             */
    WICED_PACKET_TYPE_IP,       /* Space reserved for IP header  */
    WICED_PACKET_TYPE_TCP,      /* Space reserved for TCP header */
    WICED_PACKET_TYPE_UDP       /* Space reserved for UDP header */
} wiced_packet_type_t;

/******************************************************
 *             Structures
 ******************************************************/

/**
 * TCP Stream Structure
 */
typedef struct
{
    wiced_tcp_socket_t* socket;
    wiced_packet_t*     tx_packet;
    uint8_t*            tx_packet_data;
    uint16_t            tx_packet_data_length;
    uint16_t            tx_packet_space_available;
    wiced_packet_t*     rx_packet;
} wiced_tcp_stream_t;

/**
 * IP Address Structure
 */
typedef struct
{
    wiced_ip_version_t version;

    union
    {
        uint32_t v4;
        uint32_t v6[4];
    } ip;
} wiced_ip_address_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern const wiced_ip_address_t wiced_ip_broadcast;

/******************************************************
 *               Function Declarations
 * @endcond
 ******************************************************/

/*****************************************************************************/
/** @defgroup ipcoms       IP Communication
 *
 *  WICED IP Communication Functions
 */
/*****************************************************************************/


/*****************************************************************************/
/** @addtogroup tcp       TCP
 *  @ingroup ipcoms
 *
 * Communication functions for TCP (Transmission Control Protocol)
 * Many of these are similar to the BSD-Sockets functions which are standard on POSIX
 *
 *  @{
 */
/*****************************************************************************/

/** Create a new TCP socket
 *
 *  Creates a new TCP socket.
 *  Additional steps required for the socket to become active:
 *
 *  Client socket:
 *   - bind - the socket needs to be bound to a local port ( usually WICED_ANY_PORT )
 *   - connect - connect to a specific remote IP & TCP port
 *
 *  Server socket:
 *   - listen - opens a specific local port and attaches socket to it.
 *   - accept - waits for a remote client to establish a connection
 *
 * @param[out] socket    : A pointer to a UDP socket structure which will receive the created socket handle
 * @param[in]  interface : The interface (AP or STA) for which the socket should be created
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_create_socket( wiced_tcp_socket_t* socket, wiced_interface_t interface );


/** Sets the type of service for the indicated TCP socket
 *
 * @param[in,out] socket : A pointer to a TCP socket handle that has been previously created with @ref wiced_tcp_create_socket
 * @param[in]     tos    : The type of service, where 0x00 or 0xC0 = Best effort, 0x40 or 0x80 = Background, 0x20 or 0xA0 = Video, 0x60 or 0xE0 = Voice
 *
 * @return void
 */
void wiced_tcp_set_type_of_service( wiced_tcp_socket_t* socket, uint32_t tos );


/** Registers a callback function with the indicated TCP socket
 *
 * @param[in,out] socket              : A pointer to a TCP socket handle that has been previously created with @ref wiced_tcp_create_socket
 * @param[in]     connect_callback    : The function that will be called when the TCP socket is connected
 * @param[in]     receive_callback    : The function that will be called when a new packet is received by the TCP socket
 * @param[in]     disconnect_callback : The function that will be called when the TCP socket is disconnected
 * @param[in]     arg                 : The argument that will be passed to the callbacks
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_register_callbacks( wiced_tcp_socket_t* socket, wiced_tcp_socket_callback_t connect_callback, wiced_tcp_socket_callback_t receive_callback, wiced_tcp_socket_callback_t disconnect_callback, void* arg );


/** Un-registers all callback functions associated with the indicated TCP socket
 *
 * @param[in,out] socket              : A pointer to a TCP socket handle that has been previously created with @ref wiced_tcp_create_socket
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_unregister_callbacks( wiced_tcp_socket_t* socket );

/** Binds a TCP socket to a local TCP port
 *
 *  Binds a TCP socket to a local port.
 *
 * @param[in,out] socket : A pointer to a socket handle that has been previously created with @ref wiced_tcp_create_socket
 * @param[in]     port   : The TCP port number on the local device. Can be WICED_ANY_PORT if it is not important.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_bind( wiced_tcp_socket_t* socket, uint16_t port );


/** Connects a client TCP socket to a remote server
 *
 *  Connects an existing client TCP socket to a specific remote server TCP port
 *
 * @param[in,out] socket     : A pointer to a socket handle that has been previously created with @ref wiced_tcp_create_socket
 * @param[in]     address    : The IP address of the remote server to which the connection should be made
 * @param[in]     port       : The TCP port number on the remote server
 * @param[in]     timeout_ms : Timeout period in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_connect( wiced_tcp_socket_t* socket, const wiced_ip_address_t* address, uint16_t port, uint32_t timeout_ms );


/** Opens a specific local port and attaches a socket to listen on it.
 *
 *  Opens a specific local port and attaches a socket to listen on it.
 *
 * @param[in,out] socket : A pointer to a socket handle that has been previously created with @ref wiced_tcp_create_socket
 * @param[in]     port   : The TCP port number on the local device
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_listen( wiced_tcp_socket_t* socket, uint16_t port );


/** Returns the details( ip address and the source port) of the client
 *  which is connected currently to a server
 *
 *  @param[in]  socket : A pointer to a socket handle that has been previously created with @ref wiced_tcp_create_socket
 *  @param[out] address: returned IP address of the connected client
 *  @param[out] port   : returned source port of the connected client
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_peer( wiced_tcp_socket_t* socket, wiced_ip_address_t* address, uint16_t* port );


/** Wait for a remote client and establish TCP connection
 *
 *  Sleeps until a remote client to connects to the given socket.
 *
 * @param[in,out] socket : A pointer to a socket handle that has been previously listened with @ref wiced_tcp_listen
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_accept( wiced_tcp_socket_t* socket );


/** Close a TCP socket
 *
 *  Either fully close() or shutdown() one side of the socket depending on the passed flags.
 *  NOTE: Doesn't invalidate socket handle. wiced_tcp_disconnect() still needs to be called later
 *
 * @param[in,out] socket : The open TCP socket to close/shutdown
 * @param[in]     flags  : one of wiced_tcp_shutdown_flags_t
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_close_shutdown( wiced_tcp_socket_t* socket, wiced_tcp_shutdown_flags_t flags );


/** Disconnect a TCP connection
 *
 *  Disconnects a TCP connection from a remote host
 *
 * @param[in,out] socket : The open TCP socket to disconnect
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_disconnect( wiced_tcp_socket_t* socket );


/** Deletes a TCP socket
 *
 *  Deletes a TCP socket. Socket must be either never opened or disconnected.
 *
 * @param[in,out] socket : The open TCP socket to delete
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_delete_socket( wiced_tcp_socket_t* socket );


/** Enable TLS on a TCP server socket
 *
 * Enable Transport Layer Security (successor to SSL) on a TCP
 * socket with a pre-existing TLS context
 *
 * @note: if socket is not yet connected with @ref wiced_tcp_accept , then a
 *        call to @ref wiced_tcp_accept will cause TLS to start.
 *        Otherwise, if a connection is already established, you will need
 *        to call @ref wiced_tcp_start_tls to begin TLS communication.
 *
 * @param[in,out] socket  : The TCP socket to use for TLS
 * @param[in]     context : The TLS context to use for security.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_enable_tls( wiced_tcp_socket_t* socket, void* context );


/** Start TLS on a TCP Connection
 *
 * Start Transport Layer Security (successor to SSL) on a TCP Connection
 *
 * @param[in,out] socket       : The TCP socket to use for TLS
 * @param[in]     type         : Identifies whether the device will be TLS client or server
 * @param[in]     verification : Indicates whether to verify the certificate chain against a root server.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_start_tls( wiced_tcp_socket_t* socket, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification );


/** Start TLS on a TCP Connection with a particular set of cipher suites
 *
 * Start Transport Layer Security (successor to SSL) on a TCP Connection
 *
 * @param[in,out] tls_context  : The tls context to work with
 * @param[in,out] referee      : Transport reference - e.g. TCP socket or EAP context
 * @param[in]     type         : Identifies whether the device will be TLS client or server
 * @param[in]     verification : Indicates whether to verify the certificate chain against a root server.
 * @param[in]     cipher_list  : a list of cipher suites. Null terminated.
 *                               e.g.
 *                                    static const cipher_suite_t* my_ciphers[] =
 *                                    {
 *                                          &TLS_RSA_WITH_AES_128_CBC_SHA,
 *                                          &TLS_RSA_WITH_AES_256_CBC_SHA,
 *                                          0
 *                                    };
 * @param[in]     transport_protocol : Which type of transport to use - e.g. TCP, UDP, EAP
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_generic_start_tls_with_ciphers( wiced_tls_context_t* tls_context, void* referee, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification, const cipher_suite_t* cipher_list[], tls_transport_protocol_t transport_protocol );


/*****************************************************************************/
/** @addtogroup tcppkt       TCP packet comms
 *  @ingroup tcp
 *
 * Functions for communication over TCP in packet mode
 *
 *  @{
 */
/*****************************************************************************/

/** Send a TCP data packet
 *
 *  Sends a TCP packet to the remote host.
 *  Once this function is called, the caller must not use the packet pointer
 *  again, since ownership has been transferred to the IP stack.
 *
 * @param[in,out] socket : A pointer to an open socket handle.
 * @param[in]     packet : A pointer to a packet to be sent.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t* packet );


/** Receives a TCP data packet
 *
 *  Attempts to receive a TCP data packet from the remote host.
 *  If a packet is returned successfully, then ownership of it
 *  has been transferred to the caller, and it must be released
 *  with @ref wiced_packet_delete as soon as it is no longer needed.
 *
 * @param[in,out] socket  : A pointer to an open socket handle.
 * @param[in]     packet  : A pointer to a packet pointer which will be
 *                          filled with the received packet.
 * @param[in]     timeout : Timeout value in milliseconds or WICED_NEVER_TIMEOUT
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_receive( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );

/** @} */
/*****************************************************************************/
/** @addtogroup tcpbfr       TCP buffer comms
 *  @ingroup tcp
 *
 * Functions for communication over TCP with C array buffers
 *
 *  @{
 */
/*****************************************************************************/

/** Send a memory buffer of TCP data
 *
 *  Sends a memory buffer containing TCP data to the remote host.
 *  This is not limited by packet sizes.
 *
 * @param[in,out] socket        : A pointer to an open socket handle.
 * @param[in]     buffer        : The memory buffer to send
 * @param[in]     buffer_length : The number of bytes in the buffer to send
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_send_buffer( wiced_tcp_socket_t* socket, const void* buffer, uint16_t buffer_length );

/** Send a memory buffer of TCP data
 *
 *  Sends a memory buffer containing TCP data to the remote host.
 *  This is not limited by packet sizes.
 *
 * @param[in,out] socket        : A pointer to an open socket handle.
 * @param[in]     buffer        : The memory buffer to send
 * @param[in,out] buffer_length : A pointer to a variable holding the number of bytes in the buffer to send.
 *                                This variable will be set to the actual number of bytes sent.
 * @param[in]     flags         : ORed wiced_tcp_send_flags_t
 * @param[in]     timeout       : Timeout value in milliseconds or WICED_NEVER_TIMEOUT
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_send_buffer_ex( wiced_tcp_socket_t* socket, const void* buffer, uint16_t* buffer_length, wiced_tcp_send_flags_t flags, uint32_t timeout );

/** @} */
/*****************************************************************************/
/** @addtogroup tcpstream       TCP stream comms
 *  @ingroup tcp
 *
 * Functions for communication over TCP in stream mode
 * Users need not worry about splitting data into packets in this mode
 *
 *  @{
 */
/*****************************************************************************/

/** Creates a stream for a TCP connection
 *
 *  Creates a stream for a TCP connection.
 *  The stream allows the user to write successive small
 *  amounts data into the stream without worrying about packet boundaries
 *
 * @param[out]    tcp_stream : A pointer to a stream handle that will be initialised
 * @param[in,out] socket     : A pointer to an open socket handle.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_init( wiced_tcp_stream_t* tcp_stream, wiced_tcp_socket_t* socket );


/** Deletes a TCP stream
 *
 *  Deletes a stream for a TCP connection.
 *
 * @param[in,out] tcp_stream : A pointer to a stream handle that will be de-initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_deinit( wiced_tcp_stream_t* tcp_stream );


/** Write data into a TCP stream
 *
 *  Write data into an open TCP stream.
 *  Data will only be sent if it causes the current internal packet
 *  to become full, or if @ref wiced_tcp_stream_flush is called.
 *
 * @param[in,out] tcp_stream  : A pointer to a stream handle where data will be written
 * @param[in]     data        : The memory buffer to send
 * @param[in]     data_length : The number of bytes in the buffer to send
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_write( wiced_tcp_stream_t* tcp_stream, const void* data, uint32_t data_length );


/** Write data from a resource object into a TCP stream
 *
 *  Write resource object data into an open TCP stream.
 *  Data will only be sent if it causes the current internal packet
 *  to become full, or if @ref wiced_tcp_stream_flush is called.
 *
 * @param tcp_stream  : A pointer to a stream handle that will be initialised
 * @param res_id      : The resource to send
  *
 * @return    WICED_SUCCESS : on success.
 *            WICED_ERROR   : if an error occurred
 */
wiced_result_t wiced_tcp_stream_write_resource( wiced_tcp_stream_t* tcp_stream, const resource_hnd_t* res_id );


/** Read data from a TCP stream
 *
 * @param[in,out] tcp_stream    : A pointer to a stream handle where data will be written
 * @param[out]    buffer        : The memory buffer to write data into
 * @param[in]     buffer_length : The number of bytes to read into the buffer
 * @param[in]     timeout       : Timeout value in milliseconds or WICED_NEVER_TIMEOUT
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_read( wiced_tcp_stream_t* tcp_stream, void* buffer, uint16_t buffer_length, uint32_t timeout );


/** Read data from a TCP stream and returns actual number of bytes read
 *
 * @param[in,out] tcp_stream    : A pointer to a stream handle where data will be written
 * @param[out]    buffer        : The memory buffer to write data into
 * @param[in]     buffer_length : The number of bytes to read into the buffer
 * @param[in]     timeout       : Timeout value in milliseconds or WICED_NEVER_TIMEOUT
 * @param[out]    read_count    : A pointer to an integer to store the actual number of bytes read
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_read_with_count( wiced_tcp_stream_t* tcp_stream, void* buffer, uint16_t buffer_length, uint32_t timeout, uint32_t* read_count );


/** Flush pending TCP stream data out to remote host
 *
 *  Flushes any pending data in the TCP stream out to the remote host
 *
 * @param[in,out] tcp_stream  : A pointer to a stream handle whose pending data will be flushed
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_stream_flush( wiced_tcp_stream_t* tcp_stream );


/* Enable TCP keepalive mechanism on a specified socket
 *
 * @param[in]     socket    : Pointer to a tcp_socket
 * @param[in]     interval  : The interval between subsequent keep-alive probes
 * @param[in]     probes    : The number of unacknowledged probes to send before considering \n
 *                            the connection dead and notifying the application layer
 * @param[in]     time      : The interval between the last data packet sent (simple ACKs are not \n
                             considered data) and the first keep-alive probe
*/
wiced_result_t wiced_tcp_enable_keepalive(wiced_tcp_socket_t* socket, uint16_t interval, uint16_t probes, uint16_t _time );

/** @} */

/*****************************************************************************/
/** @addtogroup tcpserver       TCP server comms
 *  @ingroup tcp
 *
 * Functions for communication over TCP as a server
 *
 *  @{
 */
/*****************************************************************************/

/** Initializes the TCP server, and creates and begins listening on specified port
 *
 * @param[in] tcp_server         : pointer to TCP server structure
 * @param[in] interface          : The interface (AP or STA) for which the socket should be created
 * @param[in] port               : TCP server listening port
 * @param[in] max_sockets        : Specify maximum number of sockets server should support. Unused parameter in FreeRTOS-LwIP
 * @param[in] connect_callback   : listening socket connect callback
 * @param[in] receive_callback   : listening socket receive callback
 * @param[in] disconnect_callback: listening socket disconnect callback
 * @param[in] arg                : argument that will be passed to the callbacks
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_start( wiced_tcp_server_t* tcp_server, wiced_interface_t interface, uint16_t port, uint16_t max_sockets, wiced_tcp_socket_callback_t connect_callback, wiced_tcp_socket_callback_t receive_callback, wiced_tcp_socket_callback_t disconnect_callback, void* arg );

/** Server accepts incoming connection on specified socket
 *
 * @param[in] tcp_server      : pointer to TCP server structure
 * @param[in] socket          : TCP socket structure
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_accept( wiced_tcp_server_t* tcp_server, wiced_tcp_socket_t* socket );

/** Add TLS security to a TCP server ( all server sockets )
 *
 * @param[in] tcp_server   : pointer to TCP server structure
 * @param[in] tls_identity : A pointer to a wiced_tls_identity_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_enable_tls( wiced_tcp_server_t* tcp_server, wiced_tls_identity_t* tls_identity );

/** Stop and tear down TCP server
 *
 * @param[in] tcp_server   : pointer to TCP server structure
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_stop( wiced_tcp_server_t* server );

/** Disconnect server socket
 *
 * @param[in] tcp_server      : pointer to TCP server structure
 * @param[in] socket          : TCP socket structure
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_disconnect_socket( wiced_tcp_server_t* tcp_server, wiced_tcp_socket_t* socket);

/** Get socket state
 *
 * @param[in] socket      : pointer to tcp socket to retrieve socket state from
 * @param[in] state       : socket state is returned here

 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_get_socket_state( wiced_tcp_socket_t* socket, wiced_socket_state_t* socket_state );


/** @} */

/*****************************************************************************/
/** @addtogroup udp       UDP
 *  @ingroup ipcoms
 *
 * Communication functions for UDP (User Datagram Protocol)
 *
 *  @{
 */
/*****************************************************************************/

/** Create a new UDP socket
 *
 *  Creates a new UDP socket.
 *  If successful, the socket is immediately ready to communicate
 *
 * @param[out] socket    : A pointer to a UDP socket structure which will receive the created socket handle
 * @param[in]  port      : The UDP port number on the local device to use. (must not be in use already)
 * @param[in]  interface : The interface (AP or STA) for which the socket should be created
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_create_socket( wiced_udp_socket_t* socket, uint16_t port, wiced_interface_t interface );


/** Update the backlog on an existing UDP socket
 *
 *  Update the backlog on an existing UDP socket
 *  If successful, the socket backlog is updated
 *
 * @param[out] socket    : A pointer to a UDP socket
 * @param[in]  backlog   : Number of UDP packets the socket should be able to queue up
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_update_socket_backlog( wiced_udp_socket_t* socket, uint32_t backlog );


/** Send a UDP data packet
 *
 *  Sends a UDP packet to a remote host.
 *  Once this function is called, the caller must not use the packet pointer
 *  again, since ownership has been transferred to the IP stack.
 *
 * @param[in,out] socket  : A pointer to an open UDP socket handle.
 * @param[in]     address : The IP address of the remote host
 * @param[in]     port    : The UDP port number on the remote host
 * @param[in]     packet  : A pointer to the packet to be sent.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_send( wiced_udp_socket_t* socket, const wiced_ip_address_t* address, uint16_t port, wiced_packet_t* packet );


/** Receives a UDP data packet
 *
 *  Attempts to receive a UDP data packet from the remote host.
 *  If a packet is returned successfully, then ownership of it
 *  has been transferred to the caller, and it must be released
 *  with @ref wiced_packet_delete as soon as it is no longer needed.
 *
 * @param[in,out] socket  : A pointer to an open UDP socket handle.
 * @param[in]     packet  : A pointer to a packet pointer which will be
 *                          filled with the received packet.
 * @param[in]     timeout : Timeout value in milliseconds or WICED_NEVER_TIMEOUT
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_receive( wiced_udp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout );


/** Replies to a UDP received data packet
 *
 *  Sends a UDP packet to the host IP address and UDP
 *  port from which a previous packet was received.
 *  Ownership of the received packet does not change.
 *  Ownership of the packet being sent is transferred to the IP stack.
 *
 * @param[in,out] socket     : A pointer to an open UDP socket handle.
 * @param[in]     in_packet  : Pointer to a packet previously received with @ref wiced_udp_receive
 * @param[in]     out_packet : A packet pointer for the UDP packet to be sent
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_reply( wiced_udp_socket_t* socket, wiced_packet_t* in_packet, wiced_packet_t* out_packet );


/** Deletes a UDP socket
 *
 *  Deletes a UDP socket that has been created with @ref wiced_udp_create_socket
 *
 * @param[in,out] socket : A pointer to an open UDP socket handle.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_delete_socket( wiced_udp_socket_t* socket );


/** Get the remote IP address and UDP port of a received packet
 *
 * Get the IP address and UDP port number details of the remote
 * host for a received packet
 *
 * @param[in]  packet  : the packet handle
 * @param[out] address : a pointer to an address structure that will receive the remote IP address
 * @param[out] port    : a pointer to a variable that will receive the remote UDP port number
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_packet_get_info( wiced_packet_t* packet, wiced_ip_address_t* address, uint16_t* port );

/** Registers a callback function with the indicated UDP socket
 *
 * @param[in,out] socket           : A pointer to a TCP socket handle that has been previously created with @ref wiced_udp_create_socket
 * @param[in]     receive_callback : The callback function that will be called when a UDP packet is received
 * @param[in]     arg              : The argument that will be passed to the callback
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_register_callbacks( wiced_udp_socket_t* socket, wiced_udp_socket_callback_t receive_callback, void* arg );

/** Add DTLS security to a UDP socket
 *
 * @param[in] UDP_SOCKET   : pointer to UDP socket.
 * @param[in] dtls_identity : A pointer to a wiced_dtls_identity_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_enable_dtls( wiced_udp_socket_t* socket, void* context );

/** Start DTLS on a UDP Connection
 *
 * Start Datagram Transport Layer Security on a UDP Connection
 *
 * @param[in,out] socket       : The UDP socket to use for DTLS
 * @param[in]     type         : Identifies whether the device will be DTLS client or server
 * @param[in]     verification : Indicates whether to verify the certificate chain against a root server.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_start_dtls( wiced_udp_socket_t* socket, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_dtls_certificate_verification_t verification );

/** Start DTLS on a UDP Connection with a particular set of cipher suites
 *
 * Start Datagram Transport Layer Security on a UDP Connection
 *
 * @param[in,out] dtls_context  : The tls context to work with
 * @param[in,out] referee       : Transport reference - e.g. UDP socket
 * @param[in]     type          : Identifies whether the device will be DTLS client or server
 * @param[in]     verification  : Indicates whether to verify the certificate chain against a root server.
 * @param[in]     cipher_list   : a list of cipher suites. Null terminated.
 *                               e.g.
 *                                    static const cipher_suite_t* my_ciphers[] =
 *                                    {
 *                                          &TLS_RSA_WITH_AES_128_CBC_SHA,
 *                                          &TLS_RSA_WITH_AES_256_CBC_SHA,
 *                                          0
 *                                    };
 * @param[in]     transport_protocol : Which type of transport to use - e.g. TCP, UDP, EAP
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_generic_start_dtls_with_ciphers( wiced_dtls_context_t* dtls_context, void* referee, wiced_ip_address_t ip, wiced_dtls_endpoint_type_t type, wiced_dtls_certificate_verification_t verification, const cipher_suite_t* cipher_list[], dtls_transport_protocol_t transport_protocol );


/** Un-registers all callback functions associated with the indicated UDP socket
 *
 * @param[in,out] socket              : A pointer to a UDP socket handle that has been previously created with @ref wiced_udp_create_socket
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_udp_unregister_callbacks( wiced_udp_socket_t* socket );

/** Sets the type of service for the indicated UDP socket
 *
 * @param[in,out] socket : A pointer to a UDP socket handle that has been previously created with @ref wiced_udp_create_socket
 * @param[in]     tos    : The type of service, where 0x00 or 0xC0 = Best effort, 0x40 or 0x80 = Background, 0x20 or 0xA0 = Video, 0x60 or 0xE0 = Voice
 *
 * @return void
 */
void wiced_udp_set_type_of_service( wiced_udp_socket_t* socket, uint32_t tos );

/** @} */

/*****************************************************************************/
/** @addtogroup icmp       ICMP ping
 *  @ingroup ipcoms
 *
 * Functions for ICMP echo requests (Internet Control Message Protocol)
 * This is commonly known as ping
 *
 *  @{
 */
/*****************************************************************************/

/** Sends a ping (ICMP echo request)
 *
 *  Sends a ICMP echo request (a ping) and waits for the response.
 *  Supports both IPv4 and IPv6
 *
 * @param[in]  interface  : The interface (AP or STA) on which to send the ping
 * @param[in]  address    : The IP address to which the ping should be sent
 * @param[in]  timeout_ms : Timeout value in milliseconds
 * @param[out] elapsed_ms : Pointer to a uint32_t which will receive the elapsed response time in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ping( wiced_interface_t interface, const wiced_ip_address_t* address, uint32_t timeout_ms, uint32_t* elapsed_ms );

/** @} */

/*****************************************************************************/
/** @addtogroup dns       DNS lookup
 *  @ingroup ipcoms
 *
 * Functions for DNS (Domain Name System) lookups
 *
 *  @{
 */
/*****************************************************************************/

/** Looks up a hostname via DNS
 *
 *  Sends a DNS query to find an IP address for a given hostname string.
 *
 *  @note :  hostname is permitted to be in dotted quad form
 *  @note :  The returned IP may be IPv4 or IPv6
 *
 * @param[in]  hostname   : A null-terminated string containing the hostname to be looked-up
 * @param[out] address    : A pointer to an IP address that will receive the resolved address
 * @param[in]  timeout_ms : Timeout value in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_hostname_lookup( const char* hostname, wiced_ip_address_t* address, uint32_t timeout_ms );

/** @} */

/*****************************************************************************/
/** @addtogroup igmp       IGMP multicast
 *  @ingroup ipcoms
 *
 * Functions for joining/leaving IGMP (Internet Group Management Protocol) groups
 *
 *  @{
 */
/*****************************************************************************/

/** Joins an IGMP group
 *
 *  Joins an IGMP multicast group, allowing reception of packets being sent to
 *  the group.
 *
 * @param[in] interface : The interface (AP or STA) which should be used to join the group
 * @param[in] address   : The IP address of the multicast group which should be joined.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_multicast_join( wiced_interface_t interface, const wiced_ip_address_t* address );


/** Leaves an IGMP group
 *
 *  Leaves an IGMP multicast group, stopping reception of packets being sent to
 *  the group.
 *
 * @param[in] interface : The interface (AP or STA) which should was used to join the group
 * @param[in] address   : The IP address of the multicast group which should be left.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_multicast_leave( wiced_interface_t interface, const wiced_ip_address_t* address );

/** @} */

/*****************************************************************************/
/** @addtogroup pktmgmt       Packet management
 *  @ingroup ipcoms
 *
 * Functions for allocating/releasing/processing packets from the WICED packet pool
 *
 *  @{
 */
/*****************************************************************************/

/** Allocates a TCP packet from the pool
 *
 *  Allocates a TCP packet from the main packet pool.
 *
 *  @note: Packets are fixed size. and applications must be very careful
 *         to avoid writing past the end of the packet buffer.
 *         The available_space parameter should be used for this.
 *
 * @param[in,out] socket          : An open TCP socket for which the packet should be created
 * @param[in]     content_length  : the intended length of TCP content if known.
 *                                  (This can be adjusted at a later point with @ref wiced_packet_set_data_end if not known)
 * @param[out]    packet          : Pointer to a packet handle which will receive the allocated packet
 * @param[out]    data            : Pointer pointer which will receive the data pointer for the packet. This is where
 *                                  TCP data should be written
 * @param[out]    available_space : pointer to a variable which will receive the space
 *                                  available for TCP data in the packet in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_create_tcp( wiced_tcp_socket_t* socket, uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space );


/** Allocates a UDP packet from the pool
 *
 *  Allocates a UDP packet from the main packet pool.
 *
 *  @note: Packets are fixed size. and applications must be very careful
 *         to avoid writing past the end of the packet buffer.
 *         The available_space parameter should be used for this.
 *
 * @param[in,out] socket          : An open UDP socket for which the packet should be created
 * @param[in]     content_length  : the intended length of UDP content if known.
 *                                  (This can be adjusted at a later point with @ref wiced_packet_set_data_end if not known)
 * @param[out]    packet          : Pointer to a packet handle which will receive the allocated packet
 * @param[out]    data            : Pointer pointer which will receive the data pointer for the packet. This is where
 *                                  UDP data should be written
 * @param[out]    available_space : pointer to a variable which will receive the space
 *                                  available for UDP data in the packet in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_create_udp( wiced_udp_socket_t* socket, uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space );


/** Allocates a general packet from the pool
 *
 *  Allocates a general packet from the main packet pool.
 *  Packet will not be usable for TCP/UDP as it will not
 *  have the required headers.
 *
 *  @note: Packets are fixed size. and applications must be very careful
 *         to avoid writing past the end of the packet buffer.
 *         The available_space parameter should be used for this.
 *
 * @param[in]  content_length   : the intended length of content if known.
 *                                (This can be adjusted at a later point with @ref wiced_packet_set_data_end if not known)
 * @param[out] packet           : Pointer to a packet handle which will receive the allocated packet
 * @param[out] data             : Pointer pointer which will receive the data pointer for the packet. This is where
 *                                data should be written
 * @param[out] available_space  : pointer to a variable which will receive the space
 *                                available for data in the packet in bytes
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_create( uint16_t content_length, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space );


/** Releases a packet back to the pool
 *
 *  Releases a packet that is in use, back to the main packet pool,
 *  allowing re-use.
 *
 * @param[in,out] packet : the packet to be released
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_delete( wiced_packet_t* packet );


/** Gets a data buffer pointer for a packet
 *
 * Retrieves a data buffer pointer for a given packet handle at a particular offset.
 * For fragmented packets, the offset input is used to traverse through the packet chain.
 *
 * @param[in,out] packet                          : the packet handle for which to get a data pointer
 * @param[in]     offset                          : the offset from the starting address.
 * @param[out]    data                            : a pointer which will receive the data pointer
 * @param[out]    fragment_available_data_length  : receives the length of data in the current fragment after the specified offset
 * @param[out]    total_available_data_length     : receives the total length of data in the all fragments after the specified offset
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_get_data( wiced_packet_t* packet, uint16_t offset, uint8_t** data, uint16_t* fragment_available_data_length, uint16_t *total_available_data_length );


/** Set the size of data in a packet
 *
 * If data has been added to a packet, this function should be
 * called to ensure the packet length is updated
 *
 * @param[in,out]  packet   : the packet handle
 * @param[in]      data_end : a pointer to the address immediately after the
 *                            last data byte in the packet buffer
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_set_data_end( wiced_packet_t* packet, uint8_t* data_end );


/** Set the size of data in a packet
 *
 * If data has been processed in this packet, this function should be
 * called to ensure calls to wiced_packet_get_data() skip the processed
 * data.
 *
 * @param[in,out] packet     : the packet handle
 * @param[in]     data_start : a pointer to the address immediately after the
 *                             last processed byte in the packet buffer
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_set_data_start( wiced_packet_t* packet, uint8_t* data_start );


/** Get the next fragment from a packet chain
 *
 * Retrieves the next fragment from a given packet handle
 *
 * @param[in]  packet               : the packet handle
 * @param[out] next_packet_fragment : the packet handle of the next fragment
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_get_next_fragment( wiced_packet_t* packet, wiced_packet_t** next_packet_fragment);


/** Creates a network packet pool from a chunk of memory
 *
 * @param[out] packet_pool    : handle to a packet pool instance which will be initialized
 * @param[in]  memory_pointer : pointer to a chunk of memory
 * @param[in]  memory_size    : size of the memory chunk
 * @param[in]  pool_name      : packet pool name string
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_pool_init( wiced_packet_pool_ref packet_pool, uint8_t* memory_pointer, uint32_t memory_size, char *pool_name );


/** Destroy a network packet pool
 *
 * @param[in,out] packet_pool : A pointer to a packet pool handle that will be de-initialized
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_pool_deinit( wiced_packet_pool_ref packet_pool );

/** Allocates a general packet from the specified packet pool
 *
 *  Allocates the desired packet type from the packet pool.
 *  Care must be taken to allocate the correct packet type to make sure that
 *  the packet has the proper headers for use by the network layer.
 *
 *  @note: Packets are fixed size. and applications must be very careful
 *         to avoid writing past the end of the packet buffer.
 *         The available_space parameter should be used for this.
 *
 * @param[in]  packet_pool      : handle to the packet pool
 * @param[in]  packet_type      : type of packet to allocate
 * @param[out] packet           : Pointer to a packet handle which will receive the allocated packet
 * @param[out] data             : Pointer pointer which will receive the data pointer for the packet. This is where
 *                                data should be written
 * @param[out] available_space  : pointer to a variable which will receive the space
 *                                available for data in the packet in bytes
 * @param[in]  timeout          : timeout value in milliseconds or WICED_NEVER_TIMEOUT
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_packet_pool_allocate_packet( wiced_packet_pool_ref packet_pool, wiced_packet_type_t packet_type, wiced_packet_t** packet, uint8_t** data, uint16_t* available_space, uint32_t timeout );

/** @} */

/*****************************************************************************/
/** @addtogroup rawip       Raw IP
 *  @ingroup ipcoms
 *
 * Functions to access IP information from network interfaces
 *
 *  @{
 */
/*****************************************************************************/

/** Retrieves the IPv4 address for an interface
 *
 * Retrieves the IPv4 address for an interface (AP or STA) if it
 * exists.
 *
 * @param[in]  interface    : the interface (AP or STA)
 * @param[out] ipv4_address : the address structure to be filled
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_get_ipv4_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address );


/** Retrieves the IPv6 address for an interface
 *
 * Retrieves the IPv6 address for an interface (AP or STA) if it
 * exists.
 *
 * @param[in]  interface    : the interface (AP or STA)
 * @param[out] ipv6_address : the address structure to be filled
 * @param[in]  address_type : the address type
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_get_ipv6_address( wiced_interface_t interface, wiced_ip_address_t* ipv6_address, wiced_ipv6_address_type_t address_type );


/** Retrieves the IPv4 gateway address for an interface
 *
 * Retrieves the gateway IPv4 address for an interface (AP or STA) if it
 * exists.
 *
 * @param[in]   interface    : the interface (AP or STA)
 * @param[out]  ipv4_address : the address structure to be filled with the
 *                             gateway IP
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_get_gateway_address( wiced_interface_t interface, wiced_ip_address_t* ipv4_address );


/** Retrieves the IPv4 netmask for an interface
 *
 * Retrieves the gateway IPv4 netmask for an interface (AP or STA) if it
 * exists.
 *
 * @param[in]  interface    : the interface (AP or STA)
 * @param[out] ipv4_address : the address structure to be filled with the
 *                            netmask
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_get_netmask( wiced_interface_t interface, wiced_ip_address_t* ipv4_address );


/** Registers a callback function that gets called when the IP address has changed
 *
 * Registers a callback function that gets called when the IP address has changed
 *
 * @param[in] callback : callback function to register
 * @param[in] arg      : pointer to the argument to pass to the callback
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_register_address_change_callback( wiced_ip_address_change_callback_t callback, void* arg );


/** De-registers a callback function that gets called when the IP address has changed
 *
 * De-registers a callback function that gets called when the IP address has changed
 *
 * @param[in] callback : callback function to de-register
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_ip_deregister_address_change_callback( wiced_ip_address_change_callback_t callback );

/** Check whether any packets are pending inside IP stack
 *
 * @param interface: IP instance
 *
 * @return WICED_TRUE if any packets pending, otherwise WICED_FALSE
 */
wiced_bool_t wiced_ip_is_any_pending_packets( wiced_interface_t interface );

/** @} */

/*
 ******************************************************************************
 * Convert an ipv4 string to a uint32_t.
 *
 * @param     arg  The string containing the value.
 * @param     arg  The structure which will receive the IP address
 *
 * @return    0 if read successfully
 */
int str_to_ip( const char* arg, wiced_ip_address_t* address );

#ifdef __cplusplus
} /*extern "C" */
#endif
