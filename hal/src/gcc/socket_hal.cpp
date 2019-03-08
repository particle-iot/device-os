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

// FIXME: Avoid defining sockaddr twice. We should probably update gcc platform to use POSIX sockets
#define HAL_SOCKET_HAL_COMPAT_NO_SOCKADDR (1)
#include "device_globals.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "core_msg.h"
#include <vector>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmissing-braces"
#include <boost/array.hpp>

// conflict of types
#define socklen_t boost_socklen_t
#include <boost/asio.hpp>
#undef socklen_t

#include <boost/system/system_error.hpp>

namespace ip = boost::asio::ip;

const sock_handle_t SOCKET_COUNT = sock_handle_t(8);
const sock_handle_t SOCKET_MAX =  SOCKET_COUNT*2;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

boost::asio::io_service device_io_service;
boost::system::error_code ec;

boost::array<ip::tcp::socket, SOCKET_COUNT> tcp_handles = {
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service),
    ip::tcp::socket(device_io_service)
};

boost::array<ip::udp::socket, SOCKET_COUNT> udp_handles = {
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service),
    ip::udp::socket(device_io_service)
};


ip::tcp::socket invalid_tcp_(device_io_service);
ip::udp::socket invalid_udp_(device_io_service);

ip::tcp::socket& invalid_tcp() {
    return invalid_tcp_;
}

ip::udp::socket& invalid_udp() {
    return invalid_udp_;
}

bool is_tcp_socket(sock_handle_t sd)
{
	return sd<SOCKET_COUNT;
}

bool is_udp_socket(sock_handle_t sd)
{
	return sd>=SOCKET_COUNT && sd<SOCKET_MAX;
}


ip::tcp::socket& tcp_from(sock_handle_t sd)
{
    if (sd>=SOCKET_COUNT)
        return invalid_tcp();
    return tcp_handles[sd];
}

ip::udp::socket& udp_from(sock_handle_t sd)
{
    if (sd<SOCKET_COUNT || sd>=SOCKET_MAX)
        return invalid_udp();
    return udp_handles[sd-SOCKET_COUNT];
}


sock_handle_t next_unused_tcp()
{
    for (sock_handle_t i=0; i<SOCKET_COUNT; i++) {
        if (!tcp_from(i).is_open())
            return i;
    }
    return -1;
}

sock_handle_t next_unused_udp()
{
    for (sock_handle_t i=SOCKET_COUNT; i<SOCKET_MAX; i++) {
        if (!udp_from(i).is_open())
            return i;
    }
    return -1;
}



bool is_valid(ip::tcp::socket& handle) {
    return &handle!=&invalid_tcp();
}

bool is_valid(ip::udp::socket& handle) {
    return &handle!=&invalid_udp();
}



class TCPServer
{
	ip::tcp::acceptor acceptor;

	void accept_handler(const boost::system::error_code& error)
	{
		if (!error)
		{
			start_accept();
		}
	}

	sock_handle_t start_accept()
	{
		sock_handle_t handle = next_unused_tcp();
		if (!socket_handle_valid(handle))
			return handle;

		ip::tcp::socket& sock = tcp_from(handle);
		acceptor.accept(sock);
		return handle;
	}


public:
	TCPServer(uint16_t port)
		: acceptor(device_io_service, ip::tcp::endpoint(ip::tcp::v4(), port))
	{
		acceptor.non_blocking(true);
	}

	~TCPServer()
	{
		acceptor.cancel();
	}

	sock_handle_t accept()
	{
		sock_handle_t handle = next_unused_tcp();
		if (!socket_handle_valid(handle))
			return handle;

		ip::tcp::socket& sock = tcp_from(handle);

		acceptor.accept(sock, ec);
		return !ec ? handle : socket_handle_invalid();
	}


};

class TCPServers
{
	std::vector<TCPServer*> servers;
public:

	bool is_valid(sock_handle_t handle) {
		return handle>=SOCKET_MAX && handle<SOCKET_MAX+servers.size();
	}

	TCPServer* from(sock_handle_t handle)
	{
		if (!is_valid(handle))
			return nullptr;

		return servers[handle-SOCKET_MAX];
	}

	sock_handle_t add(TCPServer* server)
	{
		if (!server)
			return SOCKET_INVALID;
		size_t handle = servers.size();
		servers.push_back(server);
		return SOCKET_MAX + handle;
	}

	void dispose(sock_handle_t handle)
	{
		if (is_valid(handle)) {
			size_t index = handle-SOCKET_MAX;
			TCPServer* server = servers[index];
			servers[index] = nullptr;
			delete server;
		}
	}
};



TCPServers servers;

sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif)
{
	DEBUG("Creating TCP Server on port %d", port);
	TCPServer* server = new TCPServer(port);
	return servers.add(server);
}

sock_result_t socket_accept(sock_handle_t handle)
{
	TCPServer* server = servers.from(handle);
	if (!server)
		return socket_handle_invalid();
	return server->accept();
}


int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    auto& handle = tcp_from(sd);
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
    auto& handle = tcp_from(sd);
    if (!is_valid(handle))
        return -1;
    boost::asio::socket_base::bytes_readable command(true);
    handle.io_control(command);
    std::size_t available = command.get();
    sock_result_t result = 0;
    if (_timeout || available) {
        available = handle.read_some(boost::asio::buffer(buffer, len), ec);
        result = ec.value() ? -abs(ec.value()) : available;
        if (ec.value()) {
            if (ec.value() == boost::system::errc::resource_deadlock_would_occur || // EDEADLK (35)
                ec.value() == boost::system::errc::resource_unavailable_try_again) { // EAGAIN (11)
                result = 0; // No data available
            } else {
                DEBUG("socket receive error: %d %s, read=%d", ec.value(), ec.message().c_str(), available);
            }
        }
    }
    return result;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    auto& socket = tcp_from(sd);
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

sock_result_t socket_send_ex(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, system_tick_t timeout, void* reserved)
{
    /* NOTE: non-blocking mode and timeouts are not supported */
    return socket_send(sd, buffer, len);
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    NOT_IMPLEMENTED("create nonblocking server");
    return 0;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
	ip::udp::endpoint endpoint;
	auto& socket = udp_from(sock);

	int count = socket.receive_from(boost::asio::buffer(buffer, bufLen), endpoint, 0, ec);
	if (addr && addrsize && *addrsize>=6u) {
		uint16_t port = endpoint.port();
		addr->sa_data[0] = port >> 8;
		addr->sa_data[1] = port & 0xFF;
		uint32_t ip = endpoint.address().to_v4().to_ulong();
		addr->sa_data[2] = (ip >> 24) & 0xFF;
		addr->sa_data[3] = (ip >> 16) & 0xFF;
		addr->sa_data[4] = (ip >> 8) & 0xFF;
		addr->sa_data[5] = (ip >> 0) & 0xFF;
	}

	sock_handle_t result = ec.value();

    if (result == boost::asio::error::would_block)
        return 0;
    if (result==boost::asio::error::try_again)
	   return 0;
	if (!result)
		DEBUG("count: %d", count);
	else
		DEBUG("result: %d %s", ec.value(), ec.message().c_str());

	return result ? result : count;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    unsigned port = addr->sa_data[0] << 8 | addr->sa_data[1];
    // 2-5 are IP address in network byte order
    const uint8_t* dest = addr->sa_data+2;

    ip::address_v4::bytes_type address = {{ dest[0], dest[1], dest[2], dest[3] }};
    ip::udp::endpoint endpoint(boost::asio::ip::address_v4(address),port);

	auto& socket = udp_from(sd);
	int count = socket.send_to(boost::asio::buffer(buffer, len), endpoint, 0, ec);

	sock_handle_t result = ec.value();
    if (result == boost::asio::error::would_block)
        return 0;

    return result ? result : count;
}


sock_result_t socket_bind(sock_handle_t sock, uint16_t port)
{
    NOT_IMPLEMENTED("socket_bind");
    return 0;
}

uint8_t socket_active_status(sock_handle_t socket)
{
    bool open;
    if (socket>=SOCKET_COUNT)
    		open = udp_from(socket).is_open();
    else
    		open = tcp_from(socket).is_open();
    return open ? SOCKET_STATUS_ACTIVE : SOCKET_STATUS_INACTIVE;
}

sock_result_t socket_close(sock_handle_t socket)
{
	if (servers.is_valid(socket))
	{
		servers.dispose(socket);
	}
	else if (socket>=SOCKET_COUNT)
    {
    		auto& s = udp_from(socket);
    		s.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);
    		udp_from(socket).close();
    }
    else
    {
    		auto& s = tcp_from(socket);
		s.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    		s.close();
    }
    return 0;
}

sock_result_t socket_shutdown(sock_handle_t socket, int how)
{
    if (servers.is_valid(socket) || socket >= SOCKET_COUNT) {
        return -1;
    }
    else
    {
        auto& s = tcp_from(socket);
        auto shflags = boost::asio::ip::tcp::socket::shutdown_both;
        if (how == SHUT_WR) {
            shflags = boost::asio::ip::tcp::socket::shutdown_send;
        } else if (how == SHUT_RD) {
            shflags = boost::asio::ip::tcp::socket::shutdown_receive;
        }
        s.shutdown(shflags, ec);
        return (sock_result_t)ec.value();
    }

    return -1;
}



sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
	bool udp = protocol==IPPROTO_UDP;
    sock_handle_t handle = udp ? next_unused_udp() : next_unused_tcp();
    if (handle==SOCKET_INVALID)
        return -1;

    if (udp) {
        auto& socket = udp_from(handle);
        socket.open(ip::udp::v4(), ec);
        sock_handle_t result = ec.value();
        if (result)				// error
            return result;

        boost::asio::ip::udp::endpoint listen_endpoint(ip::udp::v4(), port);
        socket.open(listen_endpoint.protocol(), ec);

        socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

        socket.bind(listen_endpoint, ec);
        result = ec.value();
        if (result) {
            DEBUG("%d %s", port, ec.message().c_str());
            return result;
        }

        socket.non_blocking(true, ec);
    }
    else {
        auto& socket = tcp_from(handle);
        socket.open(ip::tcp::v4(), ec);
        sock_handle_t result = ec.value();
        if (result)
            return result;

        socket.non_blocking(true, ec);
    }

    sock_handle_t result = ec.value();
    return result ? result : handle;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    if (handle==SOCKET_INVALID)
    		return false;
	if (handle>=SOCKET_MAX)
    		return servers.is_valid(handle);
    return handle<SOCKET_COUNT ? is_valid(tcp_from(handle)) : is_valid(udp_from(handle));
}


sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* info)
{
	if (info) {
		sock_handle_t socket = info->sock_handle;
		if (socket>=SOCKET_COUNT)
		{
			auto& s = udp_from(socket);
			ip::address_v4 address(addr->ipv4);
			DEBUG("join multicast %s", address.to_string().c_str());
			s.set_option(ip::multicast::enable_loopback(true));
			boost::asio::ip::multicast::join_group option(address);
			s.set_option(option);
			return 0;
		}
	}
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
	return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}
