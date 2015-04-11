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
 * 
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
            offset = 0;
        }
    }
};

struct udp_socket_t : wiced_udp_socket_t 
{
    ~udp_socket_t() {}
};

struct socket_t
{
    enum socket_type_t {
        NONE, TCP, UDP        
    };
    
    uint8_t type;
    union both {
        tcp_socket_t tcp;
        udp_socket_t udp;
        
        both() {}
        ~both() {}
    } s;    
    
    socket_t() {
        type = 0;
    }
    
   ~socket_t() {
       if (type)
           s.udp.~udp_socket_t();
       else
           s.tcp.~tcp_socket_t();
   }
};

inline bool is_udp(socket_t* socket) { return socket && socket->type==socket_t::UDP; }
inline bool is_tcp(socket_t* socket) { return socket && socket->type==socket_t::TCP; }
inline tcp_socket_t* tcp(socket_t* socket) { return is_tcp(socket) ? &socket->s.tcp : NULL; }
inline udp_socket_t* udp(socket_t* socket) { return is_udp(socket) ? &socket->s.udp : NULL; }


/**
 * Store for all the socket handles tracked.
 */
socket_t* handles[SOCKET_MAX];

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

uint8_t socket_handle_valid(sock_handle_t handle) {
    return is_valid(handle);    
}

/**
 * Fetches the socket_t info from an opaque handle. 
 * @return The socket_t pointer, or NULL if no socket is available for the
 * given handle.
 */
socket_t* from_handle(sock_handle_t handle) {    
    return is_valid(handle) ? handles[handle] : NULL;
}

/**
 * Initializes a new socket in an empty slot. 
 * @return The socket handle for the new socket, or {@link #SOCKET_INVALID}.
 */
sock_handle_t socket_init(socket_t::socket_type_t type) {
    for (unsigned i=0; i<SOCKET_MAX; i++) {
        if (handles[i]==NULL) {
            socket_t* sock = new socket_t();
            if (sock) {
                memset(sock, 0, sizeof(*sock));
                sock->type = type;
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
    return SOCKET_INVALID;
}

void socket_close_all()
{    
    for (sock_handle_t i=0; i<SOCKET_MAX; i++) {
        socket_close(i);
    }
}

#define SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr) \
    const uint8_t* addr_data = addr->sa_data; \
    unsigned port = addr_data[0]<<8 | addr_data[1]; \
    wiced_ip_address_t INITIALISER_IPV4_ADDRESS(ip_addr, MAKE_IPV4_ADDRESS(addr_data[2], addr_data[3], addr_data[4], addr_data[5]));

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
    socket_t* socket = from_handle(sd);
    if (is_tcp(socket)) {
        result = wiced_tcp_bind(tcp(socket), WICED_ANY_PORT);
        if (result==WICED_SUCCESS) {
            SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr);
            unsigned timeout = 300*1000;
            result = wiced_tcp_connect(tcp(socket), &ip_addr, port, timeout);
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

wiced_result_t read_packet(wiced_packet_t* packet, uint8_t* target, uint16_t target_len, uint16_t* read_len)
{
    uint16_t read = 0;
    wiced_result_t result = WICED_SUCCESS;
    uint16_t fragment;
    uint16_t available;
    uint8_t* data;
    while (target_len!=0 && (result = wiced_packet_get_data(packet, read, &data, &fragment, &available))==WICED_SUCCESS && available!=0) {
        uint16_t to_read = std::min(fragment, target_len);
        memcpy(target+read, data, to_read);
        read += to_read;
        target_len -= to_read;
    }
    if (read_len!=NULL)
        *read_len = read;
    return result;
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
    socket_t* socket = from_handle(sd);
    if (is_tcp(socket)) {
        tcp_socket_t* tcp_socket = tcp(socket);
        bytes_read = 0;
        if (!tcp_socket->packet) {
            tcp_socket->offset = 0;
            wiced_result_t result = wiced_tcp_receive(tcp_socket, &tcp_socket->packet, _timeout);
            if (result!=WICED_SUCCESS && result!=WICED_TIMEOUT) {
                DEBUG("Socket %d receive fail %d", (int)sd, int(result));
                return -result;
            }
        }        
        uint8_t* data;
        uint16_t available;
        uint16_t total;    
        bool dispose = true;
        if (tcp_socket->packet && (wiced_packet_get_data(tcp_socket->packet, tcp_socket->offset, &data, &available, &total)==WICED_SUCCESS)) {
            int read = std::min(uint16_t(len), available);
            tcp_socket->offset += read;
            memcpy(buffer, data, read);            
            dispose = (total==read);
            bytes_read = read;
            DEBUG("Socket %d receive bytes %d of %d", (int)sd, int(bytes_read), int(available));
        }        
        if (dispose) {            
            tcp_socket->dispose_packet();
        }
    }
    return bytes_read;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port) 
{
    return 0;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port) 
{           
    // not needed since socket_create binds the port
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
    socket_t* socket = from_handle(sd);
    uint8_t result = 0;
    if (socket) {
        // todo - register disconnect callback and use this to update is_bound flag
#ifdef LWIP_TO_WICED_ERR        
        result = socket->is_bound;
#else
        result = 1;
#endif
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
    socket_t* socket = from_handle(sock);
    if (socket) {
        if (socket->type==socket_t::UDP)
            wiced_udp_delete_socket(udp(socket));
        else {
            wiced_tcp_disconnect(tcp(socket));
            result = wiced_tcp_delete_socket(tcp(socket));
        }
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
sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port) 
{
    if (family!=AF_INET || !((type==SOCK_DGRAM && protocol==IPPROTO_UDP) || (type==SOCK_STREAM && protocol==IPPROTO_TCP)))
        return SOCKET_INVALID;
    
    sock_handle_t result = socket_init(protocol==IPPROTO_UDP ? socket_t::UDP : socket_t::TCP);
    wiced_result_t wiced_result;
    if (is_valid(result)) {
        socket_t* socket = from_handle(result);
        if (is_tcp(socket)) {
            wiced_result = wiced_tcp_create_socket(tcp(socket), WICED_STA_INTERFACE);            
        }
        else {
            wiced_result = wiced_udp_create_socket(udp(socket), port, WICED_STA_INTERFACE);            
        }
        if (wiced_result) {
            socket_dispose(result);
            result = SOCKET_INVALID;
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
    socket_t* socket = from_handle(sd);
    if (is_tcp(socket)) {
        result = wiced_tcp_send_buffer(tcp(socket), buffer, uint16_t(len)) ? -1 : len;
        DEBUG("Write %d bytes to socket %d", (int)len, (int)sd);
    }
    return result;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, 
        uint32_t flags, sockaddr_t* addr, socklen_t addr_size) 
{
    socket_t* socket = from_handle(sd);
    sock_result_t result = -1;
    if (is_udp(socket)) {
        SOCKADDR_TO_PORT_AND_IPADDR(addr, addr_data, port, ip_addr);
        uint16_t available = 0;
        wiced_packet_t* packet = NULL;
        uint8_t* data;
        if ((result=wiced_packet_create_udp(udp(socket), len, &packet, &data, &available))==WICED_SUCCESS) {
            size_t size = std::min(available, uint16_t(len));
            memcpy(data, buffer, size);
            /* Set the end of the data portion */
            wiced_packet_set_data_end(packet, (uint8_t*) data + size);
            result = wiced_udp_send(udp(socket), &ip_addr, port, packet);
        }                       
    }
    return result;
}

sock_result_t socket_receivefrom(sock_handle_t sd, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize) 
{
    socket_t* socket = from_handle(sd);
    sock_result_t result = -1;
    uint16_t read_len = 0;
    if (is_udp(socket)) {        
        wiced_packet_t* packet = NULL;
        // UDP receive timeout changed to 0 sec so as not to block
        if ((result=wiced_udp_receive(udp(socket), &packet, WICED_NO_WAIT))==WICED_SUCCESS) {
            if ((result=read_packet(packet, (uint8_t*)buffer, bufLen, &read_len))==WICED_SUCCESS) {
                wiced_ip_address_t wiced_ip_addr;
                uint16_t port;              
                if ((result=wiced_udp_packet_get_info(packet, &wiced_ip_addr, &port)==WICED_SUCCESS)) {
                    uint32_t ipv4 = GET_IPV4_ADDRESS(wiced_ip_addr);
                    addr->sa_data[0] = (port>>8) & 0xFF;
                    addr->sa_data[1] = port & 0xFF;
                    addr->sa_data[2] = (ipv4 >> 24) & 0xFF;
                    addr->sa_data[3] = (ipv4 >> 16) & 0xFF;
                    addr->sa_data[4] = (ipv4 >> 8) & 0xFF;
                    addr->sa_data[5] = ipv4 & 0xFF;
                }
            }
            wiced_packet_delete(packet);
        }
    }   
    //Return the number of bytes received, or -1 if an error
    return (read_len > 0)?read_len:result;
}
