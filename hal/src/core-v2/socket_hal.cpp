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

#if USE_WICED_SDK==1

#include "socket_hal.h"
#include "wiced.h"
#include <algorithm>

const sock_handle_t SOCKET_MAX = (sock_handle_t)16;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

/**
 * The info we maintain for each socket.
 */
struct tcp_socket_t : wiced_tcp_socket_t {
    /**
     * Any outstanding packet to retrieve data from.
     */
    wiced_packet_t* packet;
    unsigned offset;
    
    tcp_socket_t() : packet(NULL), offset(0) {}
    
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

tcp_socket_t* handles[SOCKET_MAX];

inline bool is_valid(sock_handle_t handle) {
    return handle<SOCKET_MAX;
}


/**
 * Fetches the tcp_socket_t info from an opaque handle. 
 */
tcp_socket_t* from_handle(sock_handle_t handle) {    
    return is_valid(handle) ? handles[handle] : NULL;
}

sock_handle_t socket_init() {
    for (unsigned i=0; i<SOCKET_MAX; i++) {
        if (handles[i]==NULL) {
            handles[i] = new tcp_socket_t();
            return i;
        }
    }    
    return SOCKET_INVALID;
}

sock_handle_t socket_dispose(sock_handle_t handle) {
    if (is_valid(handle)) {            
        delete handles[handle];
        handles[handle] = NULL;        
    }
    return SOCKET_INVALID;
}


sock_result_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    wiced_tcp_socket_t* socket = from_handle(sd);
    sock_result_t result = SOCKET_INVALID;
    if (socket) {
        result = wiced_tcp_bind(socket, WICED_ANY_PORT);
        if (result==WICED_SUCCESS) {            
            const uint8_t* data = addr->sa_data;
            unsigned port = data[0]<<8 | data[1];
            unsigned timeout = 0;
            wiced_ip_address_t INITIALISER_IPV4_ADDRESS(ip_addr, MAKE_IPV4_ADDRESS(data[2], data[3], data[4], data[5]));
            result = wiced_tcp_connect(socket, &ip_addr, port, timeout);
        }
    }
    return result;
}

sock_result_t socket_reset_blocking_call() 
{
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{  
    uint8_t* data;
    uint16_t available;
    uint16_t total;
    
    sock_result_t bytes_read = 0;
    tcp_socket_t* socket = from_handle(sd);
    if (socket) {        
        if (!socket->packet) {
            wiced_tcp_receive(socket, &socket->packet, _timeout);
        }
        
        bool dispose = true;
        if (wiced_packet_get_data(socket->packet, socket->offset, &data, &available, &total)==WICED_SUCCESS) {
            int read = std::min(uint16_t(len), available);
            socket->offset += read;
            memcpy(buffer, data, read);            
            dispose = (total==read);
            bytes_read = read;
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

uint8_t socket_active_status(sock_handle_t sd) 
{
    tcp_socket_t* socket = from_handle(sd);
    uint8_t result = 0;
    if (socket) {
        result = socket->is_bound;
    }
    return result;
}

sock_result_t socket_close(sock_handle_t sock) 
{
    wiced_tcp_socket_t* socket = from_handle(sock);
    sock_result_t result = WICED_SUCCESS;
    if (socket) {
        wiced_tcp_disconnect(socket);
        result = wiced_tcp_delete_socket(socket);        
        socket_dispose(sock);
    }
    return result;
}

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol) 
{
    sock_handle_t result = socket_init();
    if (is_valid(result)) {
        wiced_tcp_socket_t* socket = from_handle(result);
        if (wiced_tcp_create_socket(socket, WICED_STA_INTERFACE)!=WICED_SUCCESS || family!=AF_INET || 
                !((type==SOCK_DGRAM && protocol==IPPROTO_UDP) && (type==SOCK_STREAM && protocol==IPPROTO_TCP)) ) {
            result = socket_dispose(result);
        }        
    }    
    return result;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len) 
{
    sock_result_t result = WICED_BADVALUE;
    wiced_tcp_socket_t* socket = from_handle(sd);
    if (socket) {
        result = wiced_tcp_send_buffer(socket, buffer, len);
    }
    return result;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size) 
{
    return 0;
}

#else
#include "../template/socket_hal.c"
#endif