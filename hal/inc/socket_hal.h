/**
 ******************************************************************************
 * @file    socket_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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


#ifndef _SOCKET_HAL_H
#define	_SOCKET_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "debug.h"
#include <stdint.h>
#include "system_tick_hal.h"
#include "inet_hal.h"

typedef struct _sockaddr_t
{
    uint16_t   sa_family;
    uint8_t    sa_data[14];
} sockaddr_t;

typedef uint32_t sock_handle_t;
typedef uint32_t socklen_t;
typedef int32_t sock_result_t;

static const uint8_t SOCKET_STATUS_INACTIVE = 1;
static const uint8_t SOCKET_STATUS_ACTIVE = 0;

uint8_t socket_active_status(sock_handle_t socket);

uint8_t socket_handle_valid(sock_handle_t handle);

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif);

sock_result_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen);

/**
 * Reads from a socket to a buffer, upto a given number of bytes, with timeout.
 * Returns the number of bytes read, which can be less than the number requested.
 * Returns <0 on error or if the stream is closed.
 */
sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout);

sock_result_t socket_receivefrom(sock_handle_t sd, void* buffer, socklen_t len, uint32_t flags, sockaddr_t* address, socklen_t* addr_size);

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len);

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size);

sock_result_t socket_close(sock_handle_t sd);

sock_result_t socket_reset_blocking_call();

sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif);
sock_result_t socket_accept(sock_handle_t sd);

/**
 * Retrieves the handle of an invalid socket.
 */
sock_handle_t socket_handle_invalid();



//--------- Address Families --------

#define  AF_INET                2
#define  AF_INET6               23

//------------ Socket Types ------------

#define  SOCK_STREAM            1
#define  SOCK_DGRAM             2
#define  SOCK_RAW               3           // Raw sockets allow new IPv4 protocols to be implemented in user space. A raw socket receives or sends the raw datagram not including link level headers
#define  SOCK_RDM               4
#define  SOCK_SEQPACKET         5

//----------- Socket Protocol ----------

#define IPPROTO_IP              0           // dummy for IP
#define IPPROTO_ICMP            1           // control message protocol
#define IPPROTO_IPV4            IPPROTO_IP  // IP inside IP
#define IPPROTO_TCP             6           // tcp
#define IPPROTO_UDP             17          // user datagram protocol
#define IPPROTO_IPV6            41          // IPv6 in IPv6
#define IPPROTO_NONE            59          // No next header
#define IPPROTO_RAW             255         // raw IP packet
#define IPPROTO_MAX             256


#ifdef	__cplusplus
}
#endif

#endif	/* SOCKET_H */

