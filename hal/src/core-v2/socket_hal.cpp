/**
 ******************************************************************************
 * @file    socket_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    09-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "socket_hal.h"
#include "wiced.h"
#include <algorithm>
#include "service_debug.h"

/**
 * The largest socket handle.  Used for looping
 */
const sock_handle_t SOCKET_MAX = (sock_handle_t)16;

/**
 * The handle value returned when a socket cannot be created. 
 */
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

/**
 * The info we maintain for each socket. It wraps a WICED socket. 
 */
struct tcp_socket_t : wiced_tcp_socket_t {
    /**
     * Any outstanding packet to retrieve data from.
     */
    wiced_packet_t* packet;
    
    /**
     * The current offset of data already read from the packet.
     */    
    unsigned offset;
    
    tcp_socket_t() {}
    
    ~tcp_socket_t() {
        dispose_packet();
    }
    
    void dispose_packet() {
        if (packet) {
            wiced_packet_delete(packet);
            packet = NULL;
        }
    }
};

/**
 * Store for all the socket handles tracked.
 */
tcp_socket_t* handles[SOCKET_MAX];

/**
 * Determines if the given socket handle is valid.
 * @param handle    The handle to test
 * @return {@code true} if the socket handle is valid, {@code false} otherwise.
 * Note that this doesn't guarantee the socket can be used, only that the handle
 * is within a valid range. To determine if a handle has an associated socket,
 * use {@link #from_handle}
 */
inline bool is_valid(sock_handle_t handle) {
    return handle<SOCKET_MAX;
}


/**
 * Fetches the tcp_socket_t info from an opaque handle. 
 * @return The tcp_socket_t pointer, or NULL if no socket is available for the
 * given handle.
 */
tcp_socket_t* from_handle(sock_handle_t handle) {    
    return is_valid(handle) ? handles[handle] : NULL;
}

/**
 * Initializes a new socket in an empty slot. 
 * @return The socket handle for the new socket, or {@link #SOCKET_INVALID}.
 */
sock_handle_t socket_init() {
    for (unsigned i=0; i<SOCKET_MAX; i++) {
        if (handles[i]==NULL) {
            tcp_socket_t* sock = new tcp_socket_t();
            if (sock) {
                memset(sock, 0, sizeof(*sock));
                handles[i] = sock;
                return i;
            }
            else
                break;
        }
    }    
    return SOCKET_INVALID;
}

/**
 * Discards a previously allocated socket. If the socket is already invalid, returns silently.
 * @param handle    The handle to discard.
 * @return SOCKET_INVALID always.
 */
sock_handle_t socket_dispose(sock_handle_t handle) {
    if (is_valid(handle)) {            
        delete handles[handle];
        handles[handle] = NULL;        
    }
    return  SOCKET_INVALID;
}

/**
 * Connects the given socket to the address.
 * @param sd        The socket handle to connect
 * @param addr      The address to connect to
 * @param addrlen   The length of the address details.
 * @return 0 on success.
 */
sock_result_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    sock_result_t result = SOCKET_INVALID;
    wiced_tcp_socket_t* socket = from_handle(sd);
    if (socket) {
        result = wiced_tcp_bind(socket, WICED_ANY_PORT);
        if (result==WICED_SUCCESS) {            
            const uint8_t* data = addr->sa_data;
            unsigned port = data[0]<<8 | data[1];
            unsigned timeout = 300;
            wiced_ip_address_t INITIALISER_IPV4_ADDRESS(ip_addr, MAKE_IPV4_ADDRESS(data[2], data[3], data[4], data[5]));
            result = wiced_tcp_connect(socket, &ip_addr, port, timeout);
        }
    }
    return -result;
}

/**
 * Is there any way to unblock a blocking call on WICED? Perhaps shutdown the networking layer?
 * @return 
 */
sock_result_t socket_reset_blocking_call() 
{
    return 0;
}

/**
 * Receives data from a socket. 
 * @param sd
 * @param buffer
 * @param len
 * @param _timeout
 * @return The number of bytes read. -1 if the end of the stream is reached.
 */
sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{      
    sock_result_t bytes_read = -1;
    tcp_socket_t* socket = from_handle(sd);
    if (socket) {   
        bytes_read = 0;
        if (!socket->packet) {
            wiced_result_t result = wiced_tcp_receive(socket, &socket->packet, _timeout);
            if (result!=WICED_SUCCESS && result!=WICED_TIMEOUT) {
                DEBUG("Socket %d receive fail %d", (int)sd, int(result));
                return -result;
            }
        }        
        uint8_t* data;
        uint16_t available;
        uint16_t total;    
        bool dispose = true;
        if (socket->packet && (wiced_packet_get_data(socket->packet, socket->offset, &data, &available, &total)==WICED_SUCCESS)) {
            int read = std::min(uint16_t(len), available);
            socket->offset += read;
            memcpy(buffer, data, read);            
            dispose = (total==read);
            bytes_read = read;
            DEBUG("Socket %d receive bytes %d of %d", (int)sd, int(bytes_read), int(available));
        }        
        if (dispose)
            socket->dispose_packet();
    }
    return bytes_read;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port) 
{
    return 0;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize) 
{
    return 0;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port) 
{           
    return 0;
}

sock_result_t socket_accept(sock_handle_t sock) 
{    
    return 0;
}

/**
 * Determines if a given socket is bound.
 * @param sd    The socket handle to test
 * @return non-zero if bound, 0 otherwise.
 */
uint8_t socket_active_status(sock_handle_t sd) 
{
    tcp_socket_t* socket = from_handle(sd);
    uint8_t result = 0;
    if (socket) {
        result = socket->is_bound;
    }
    return result ? SOCKET_STATUS_ACTIVE : SOCKET_STATUS_INACTIVE;
}

/**
 * Closes the socket handle.
 * @param sock
 * @return 
 */
sock_result_t socket_close(sock_handle_t sock) 
{
    sock_result_t result = WICED_SUCCESS;
    wiced_tcp_socket_t* socket = from_handle(sock);
    if (socket) {
        wiced_tcp_disconnect(socket);
        result = wiced_tcp_delete_socket(socket);        
        socket_dispose(sock);
        DEBUG("socket closed %d", int(sock));
    }
    return result;
}

/**
 * Create a new socket handle.
 * @param family    Must be {@code AF_INET}
 * @param type      Either SOCK_DGRAM or SOCK_STREAM
 * @param protocol  Either IPPROTO_UDP or IPPROTO_TCP
 * @return 
 */
sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol) 
{
    if (family!=AF_INET || !((type==SOCK_DGRAM && protocol==IPPROTO_UDP) || (type==SOCK_STREAM && protocol==IPPROTO_TCP)))
        return SOCKET_INVALID;
    
    sock_handle_t result = socket_init();
    if (is_valid(result)) {
        wiced_tcp_socket_t* socket = from_handle(result);        
        if (wiced_tcp_create_socket(socket, WICED_STA_INTERFACE)!=WICED_SUCCESS) {
            result = socket_dispose(result);
        }        
    }        
    return result;
}

/**
 * Send data to a socket.
 * @param sd    The socket handle to send data to.
 * @param buffer    The data to send
 * @param len       The number of bytes to send
 * @return 
 */
sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len) 
{
    sock_result_t result = -1;
    wiced_tcp_socket_t* socket = from_handle(sd);
    if (socket) {
//        char buf[30];
//        sprintf(buf, "%d", (int)len);
//        for (unsigned i=0; i<len; i++) {
//            ((uint8_t*)buffer)[i] = i%32+'A';
//        }
//        strcpy((char*)buffer, buf);
        result = wiced_tcp_send_buffer(socket, buffer, uint16_t(len)) ? -1 : len;
        DEBUG("Write %d bytes to socket %d", (int)len, (int)sd);
    }
    return result;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size) 
{
    return 0;
}
