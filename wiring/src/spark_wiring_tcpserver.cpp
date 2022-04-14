/**
 ******************************************************************************
 * @file    spark_wiring_tcpserver.cpp
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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_COMPAT

#include "spark_wiring_tcpclient.h"
#include "spark_wiring_tcpserver.h"
#include "spark_wiring_network.h"
#include "spark_wiring_thread.h"

using namespace spark;

static TCPClient* s_invalid_client = NULL;

class TCPServerClient : public TCPClient
{

public:

    TCPServerClient(sock_handle_t sock) : TCPClient(sock) {}

    virtual IPAddress remoteIP() override
    {
        sock_peer_t peer;
        memset(&peer, 0, sizeof(peer));
        peer.size = sizeof(peer);
        socket_peer(sock_handle(), &peer, NULL);
        return peer.address;
    }
};

TCPServer::TCPServer(uint16_t port, network_interface_t nif) : _port(port), _nif(nif), _sock(socket_handle_invalid()), _client(socket_handle_invalid())
{
    SINGLE_THREADED_BLOCK() {
        if (!s_invalid_client) {
            s_invalid_client = new TCPClient(socket_handle_invalid());
        }
    }
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
    _client.stop();
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
        stop();
        _client = *s_invalid_client;
        return _client;
    }

    int sock = socket_accept(_sock);

    if (!socket_handle_valid(sock))
    {
        _client = *s_invalid_client;
    }
    else
    {
        TCPServerClient client = TCPServerClient(sock);
        client.d_->remoteIP = client.remoteIP();      // fetch the peer IP ready for the copy operator
        _client = client;

    }

    return _client;
}

size_t TCPServer::write(uint8_t b, system_tick_t timeout)
{
    return write(&b, sizeof(b), timeout);
}

size_t TCPServer::write(const uint8_t *buf, size_t size, system_tick_t timeout)
{
    _client.clearWriteError();
    size_t ret = _client.write(buf, size, timeout);
    setWriteError(_client.getWriteError());
    return ret;
}

size_t TCPServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t TCPServer::write(const uint8_t *buffer, size_t size)
{
    return write(buffer, size, SPARK_WIRING_TCPCLIENT_DEFAULT_SEND_TIMEOUT);
}

#endif // HAL_USE_SOCKET_HAL_COMPAT
