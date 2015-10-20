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
#include "device_globals.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "core_msg.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmissing-braces"
#include <boost/array.hpp>

// conflict of types
#define socklen_t boost_socklen_t
#include <boost/asio.hpp>
#undef socklen_t

namespace ip = boost::asio::ip;

const sock_handle_t SOCKET_MAX = (sock_handle_t)8;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

boost::asio::io_service device_io_service;
boost::system::error_code ec;

boost::array<ip::tcp::socket, 8> handles = {
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service)
};

ip::tcp::socket invalid_(device_io_service);

ip::tcp::socket& invalid() {
    return invalid_;
}

ip::tcp::socket& from(sock_handle_t sd)
{
    if (sd>=SOCKET_MAX)
        return invalid();
    return handles[sd];
}

sock_handle_t next_unused()
{
    for (sock_handle_t i=0; i<SOCKET_MAX; i++) {
        if (!from(i).is_open())
            return i;
    }
    return -1;
}

bool is_valid(ip::tcp::socket& handle) {
    return &handle!=&invalid();
}

int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    auto& handle = from(sd);
    if (!is_valid(handle))
        return -1;

    unsigned port = addr->sa_data[0] << 8 | addr->sa_data[1];
    // 2-5 are IP address in network byte order
    const uint8_t* dest = addr->sa_data+2;

    ip::address_v4::bytes_type address = {{ dest[0], dest[1], dest[2], dest[3] }};
    ip::tcp::endpoint endpoint(boost::asio::ip::address_v4(address),port);

    handle.connect(endpoint, ec);
    bool open = handle.is_open();
    return ec.value();
}

sock_result_t socket_reset_blocking_call()
{
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
    auto& handle = from(sd);
    if (!is_valid(handle))
        return -1;
    boost::asio::socket_base::bytes_readable command(true);
    handle.io_control(command);
    std::size_t available = command.get();
    sock_result_t result = 0;
    available = handle.read_some(boost::asio::buffer(buffer, len), ec);
    result = ec.value() ? -abs(ec.value()) : available;
    if (ec.value()) {
        DEBUG("socket receive error: %d %s", ec.value(), ec.message().c_str());
    }
    return result;
}


sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    auto& socket = from(sd);
    if (!is_valid(socket))
        return -1;
    try
    {
        sock_result_t result = write(socket, boost::asio::buffer(buffer, len));
        return result;
    }
    catch (boost::system::system_error e)
    {
        return -1;
    }
}


sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    NOT_IMPLEMENTED("create nonblocking server");
    return 0;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    NOT_IMPLEMENTED("socket_receivefrom");
    return 0;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port)
{
    NOT_IMPLEMENTED("socket_bind");
    return 0;
}

sock_result_t socket_accept(sock_handle_t sock)
{
    NOT_IMPLEMENTED("socket_accept");
    return 0;
}

uint8_t socket_active_status(sock_handle_t socket)
{
    bool open = from(socket).is_open();
    return open ? SOCKET_STATUS_ACTIVE : SOCKET_STATUS_INACTIVE;
}

sock_result_t socket_close(sock_handle_t sock)
{
    auto& handle = from(sock);
    handle.close();
    return 0;
}

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    sock_handle_t handle = next_unused();
    if (handle==SOCKET_INVALID)
        return -1;

    auto& socket = from(handle);
    socket.open(::ip::tcp::v4(), ec);
    sock_handle_t result = ec.value();
    return result ? result : handle;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    //NOT_IMPLEMENTED("socket_sendto");
    return 0;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    return is_valid(from(handle));
}


sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
{
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
{
	return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}
