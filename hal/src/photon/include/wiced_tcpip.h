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

/** @file
 *  Defines functions to communicate over the IP network
 */

#pragma once

#include "wiced_utilities.h"
#include "wwd_network_interface.h"
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
                                                            (((((uint32_t) (a)) << 16) & 0xFFFF0000UL) | ((uint32_t)(b) &0x0000FFFFUL)), \
                                                            (((((uint32_t) (c)) << 16) & 0xFFFF0000UL) | ((uint32_t)(d) &0x0000FFFFUL)), \
                                                            (((((uint32_t) (e)) << 16) & 0xFFFF0000UL) | ((uint32_t)(f) &0x0000FFFFUL)), \
                                                            (((((uint32_t) (g)) << 16) & 0xFFFF0000UL) | ((uint32_t)(h) &0x0000FFFFUL))  \
                                                        }


/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*wiced_ip_address_change_callback_t)( void* arg );
typedef wiced_result_t (*wiced_tcp_stream_write_callback_t)( void* tcp_stream, const void* data, uint32_t data_length );

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
/** @addtogroup tls       TLS Security
 *  @ingroup ipcoms
 *
 * Security initialisation functions for TLS enabled connections (Transport Layer Security - successor to SSL Secure Sockets Layer )
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a simple TLS context handle
 *
 * @param[out] context : A pointer to a wiced_tls_simple_context_t context object that will be initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_simple_context( wiced_tls_simple_context_t* context, const char* peer_cn );


/** Initialises an advanced TLS context handle using a supplied certificate and private key
 *
 * @param[out] context    : A pointer to a wiced_tls_advanced_context_t context object that will be initialised
 * @param[in] certificate : The server x509 certificate in base64 encoded string format
 * @param[in] key         : The server private key in base64 encoded string format
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_advanced_context( wiced_tls_advanced_context_t* context, const char* certificate, const char* key);


/** Initialise the trusted root CA certificates
 *
 *  Initialises the collection of trusted root CA certificates used to verify received certificates
 *
 * @param[in] trusted_ca_certificates : A chain of x509 certificates in base64 encoded string format
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_root_ca_certificates( const char* trusted_ca_certificates );


/** De-initialise the trusted root CA certificates
 *
 *  De-initialises the collection of trusted root CA certificates used to verify received certificates
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_deinit_root_ca_certificates( void );


/** De-initialise a previously inited simple or advanced context
 *
 * @param[in,out] context : a pointer to either a wiced_tls_simple_context_t or wiced_tls_advanced_context_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_deinit_context( wiced_tls_simple_context_t* context );


/** Reset a previously inited simple or advanced context
 *
 * @param[in,out] tls_context : a pointer to either a wiced_tls_simple_context_t or wiced_tls_advanced_context_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_reset_context( wiced_tls_simple_context_t* tls_context );

/** @} */

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
 * @param[in]     context : The TLS context to use for security. This must
 *                          have been initialised with @ref wiced_tls_init_simple_context
 *                          or @ref wiced_tls_init_advanced_context
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
wiced_result_t wiced_generic_start_tls_with_ciphers( wiced_tls_simple_context_t* tls_context, void* referee, wiced_tls_endpoint_type_t type, wiced_tls_certificate_verification_t verification, const cipher_suite_t* cipher_list[], tls_transport_protocol_t transport_protocol );


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
 * @param[in] context      : A pointer to a wiced_tls_advanced_context_t context object that will be initialized
 * @param[in] certificate  : The server x509 certificate in base64 encoded string format
 * @param[in] key          : The server private key in base64 encoded string format
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tcp_server_add_tls( wiced_tcp_server_t* tcp_server, wiced_tls_advanced_context_t* context, const char* server_cert, const char* server_key );

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
 *         Theavailable_space parameter should be used for this.
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
