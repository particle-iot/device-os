/**
 ******************************************************************************
 * @file    particle_wiring_tcpserver.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Nov-2013
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

#include "particle_wiring_tcpclient.h"
#include "particle_wiring_tcpserver.h"
#include "particle_wiring_network.h"

using namespace particle;

TCPServer::TCPServer(uint16_t port, network_interface_t nif) : _port(port), _nif(nif), _sock(socket_handle_invalid()), _client(socket_handle_invalid())
{

}

bool TCPServer::begin()
{
    stop();
    if(!Network.from(_nif).ready())
    {
        return false;
    }

    if (socket_handle_valid(_sock)) {
        return true;
    }
    sock_result_t result = socket_create_tcp_server(_port, _nif);
    if (socket_handle_valid(result)) {
        _sock = result;
        return true;
    }
    stop();
    return false;
}

void TCPServer::stop()
{
    socket_close(_sock);
    _sock = socket_handle_invalid();
}

TCPClient TCPServer::available()
{
    sock_handle_t SOCKET_INVALID = socket_handle_invalid();

    if(_sock == SOCKET_INVALID)
    {
        begin();
    }

    if((!Network.from(_nif).ready()) || (_sock == SOCKET_INVALID))
    {
        _sock = SOCKET_INVALID;
        _client = TCPClient(SOCKET_INVALID);
        return _client;
    }

    int sock = socket_accept(_sock);

    if (!socket_handle_valid(sock))
    {
        _client = TCPClient(SOCKET_INVALID);
    }
    else
    {
        _client = TCPClient(sock);
    }

    return _client;
}

size_t TCPServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t TCPServer::write(const uint8_t *buffer, size_t size)
{
    return _client.write(buffer, size);
}
