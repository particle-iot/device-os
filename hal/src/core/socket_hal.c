/**
 ******************************************************************************
 * @file    socket_hal.c
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

// Skip conflicting sockaddr declaration
#define HAL_SOCKET_HAL_COMPAT_NO_SOCKADDR (1)
#include "socket_hal.h"
#include "cc3000_spi.h"
#include "evnt_handler.h"
#include "socket.h"

const sock_handle_t SOCKET_MAX = (sock_handle_t)8;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    return connect(sd, (const sockaddr*)addr, addrlen);
}

sock_result_t socket_reset_blocking_call()
{
    //Work around to exit the blocking nature of socket calls
    tSLInformation.usEventOrDataReceived = 1;
    tSLInformation.usRxEventOpcode = 0;
    tSLInformation.usRxDataPending = 0;
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
  timeval timeout;
  _types_fd_set_cc3000 readSet;
  int bytes_received = 0;
  int num_fds_ready = 0;

  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sd, &readSet);

  // tell select to timeout after the minimum 5 milliseconds
  // defined in the SimpleLink API as SELECT_TIMEOUT_MIN_MICRO_SECONDS
  _timeout = _timeout<5 ? 5 : _timeout;
  timeout.tv_sec = _timeout/1000;
  timeout.tv_usec = (_timeout%1000)*1000;

  num_fds_ready = select(sd + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sd, &readSet))
    {
      // recv returns negative numbers on error
      bytes_received = recv(sd, buffer, len, 0);
      DEBUG("bytes_received %d",bytes_received);
    }
  }
  else if (0 > num_fds_ready)
  {
    // error from select
    DEBUG("select Error %d",num_fds_ready);
    return num_fds_ready;
  }
  return bytes_received;
}


sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif)
{
    sock_handle_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle_valid(sock))
    {
        long optval = SOCK_ON;
        sockaddr tServerAddr;

        tServerAddr.sa_family = AF_INET;
        tServerAddr.sa_data[0] = (port & 0xFF00) >> 8;
        tServerAddr.sa_data[1] = (port & 0x00FF);
        tServerAddr.sa_data[2] = 0;
        tServerAddr.sa_data[3] = 0;
        tServerAddr.sa_data[4] = 0;
        tServerAddr.sa_data[5] = 0;

        sock_result_t retVal=setsockopt(sock, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, &optval, sizeof(optval));
        if (retVal>=0) retVal=bind(sock, &tServerAddr, sizeof(tServerAddr));
        if (retVal>=0) retVal=listen(sock,0);
        if (retVal<0) {
            socket_close(sock);
            sock = retVal;
        }
    }
    return sock;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize) {
    _types_fd_set_cc3000 readSet;
    timeval timeout;

    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    int ret = -1;
    if (select(sock + 1, &readSet, NULL, NULL, &timeout) > 0)
    {
        if (FD_ISSET(sock, &readSet))
        {
            ret = recvfrom(sock, buffer, bufLen, 0, (sockaddr*)addr, addrsize);
        }
    }
    return ret;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port)
{
    sockaddr tUDPAddr;
    memset(&tUDPAddr, 0, sizeof(tUDPAddr));
    tUDPAddr.sa_family = AF_INET;
    tUDPAddr.sa_data[0] = (port & 0xFF00) >> 8;
    tUDPAddr.sa_data[1] = (port & 0x00FF);

    return bind(sock, &tUDPAddr, sizeof(tUDPAddr));
}

sock_result_t socket_accept(sock_handle_t sock)
{
    sockaddr tClientAddr;
    socklen_t tAddrLen = sizeof(tClientAddr);
    return accept(sock, &tClientAddr, &tAddrLen);
}

uint8_t socket_active_status(sock_handle_t socket)
{
    return get_socket_active_status(socket);
}

sock_result_t socket_close(sock_handle_t sock)
{
    return closesocket(sock);
}

sock_result_t socket_shutdown(sock_handle_t sd, int how)
{
    // Not supported
    return -1;
}

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    sock_handle_t sock = socket(family, type, protocol);
    if (socket_handle_valid(sock)) {
        if (IPPROTO_UDP==protocol) {
            bool bound = socket_bind(sock, port) >= 0;
            DEBUG("socket=%d bound=%d",sock,bound);
            if(!bound)
            {
                socket_close(sock);
                sock = SOCKET_INVALID;
            }
        }
    }
    return sock;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return send(sd, buffer, len, 0);
}

sock_result_t socket_send_ex(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, system_tick_t timeout, void* reserved)
{
    /* NOTE: non-blocking mode and timeouts are not supported */
    return socket_send(sd, buffer, len);
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    return sendto(sd, buffer, len, flags, (sockaddr*)addr, addr_size);
}

uint8_t socket_handle_valid(sock_handle_t handle)
{
    return handle<SOCKET_MAX;
}

sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
    /* Not supported on Core */
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
    /* Not supported on Core */
    return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}
