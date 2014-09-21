/**
 ******************************************************************************
 * @file    spark_wiring_tcpserver.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Nov-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

#include "spark_wiring_tcpclient.h"
#include "spark_wiring_tcpserver.h"

using namespace spark;

TCPServer::TCPServer(uint16_t port) : _port(port), _sock(SOCKET_INVALID), _client(SOCKET_INVALID)
{

}

void TCPServer::begin()
{
	if(!WiFi.ready())
	{
		return;
	}

	int sock = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0)
	{
		return;
	}

	_sock = socket_create_nonblocking_server(sock, _port);
}

TCPClient TCPServer::available()
{
	if(_sock == SOCKET_INVALID)
	{
		begin();
	}

	if((!WiFi.ready()) || (_sock == SOCKET_INVALID))
	{
		_sock = SOCKET_INVALID;
		_client = TCPClient(SOCKET_INVALID);
		return _client;
	}

	int sock = socket_accept(_sock);

	if (sock < 0)
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
