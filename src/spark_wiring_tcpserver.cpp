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

TCPServer::TCPServer(uint16_t port) : _port(port), _sock(MAX_SOCK_NUM)
{

}

void TCPServer::begin()
{
	sockaddr tServerAddr;

	tServerAddr.sa_family = AF_INET;

	tServerAddr.sa_data[0] = (_port & 0xFF00) >> 8;
	tServerAddr.sa_data[1] = (_port & 0x00FF);
	tServerAddr.sa_data[2] = 0;
	tServerAddr.sa_data[3] = 0;
	tServerAddr.sa_data[4] = 0;
	tServerAddr.sa_data[5] = 0;

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0)
	{
		return;
	}

	if (setsockopt(sock, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, SOCK_ON, sizeof(SOCK_ON)) < 0)
	{
		return;
	}


	if (bind(sock, (sockaddr*)&tServerAddr, sizeof(tServerAddr)) < 0)
	{
		return;
	}

	if (listen(sock, 0) < 0)
	{
		return;
	}

	_sock = sock;
}

void TCPServer::acceptClientConnections()
{
	sockaddr tClientAddr;
	socklen_t tAddrLen = sizeof(tClientAddr);

	for (int i = 0; i < MAX_SOCK_NUM; ++i)
	{
		if (!_clients[i].connected())
		{
			int sock = accept(_sock, (sockaddr*)&tClientAddr, &tAddrLen);

			if (sock > -1)
			{
				_clients[i] = TCPClient(sock);
			}
		}
	}
}

TCPClient TCPServer::available()
{
	acceptClientConnections();

	for (int i = 0; i < MAX_SOCK_NUM; ++i)
	{
		if (_clients[i].connected() && _clients[i].available())
		{
			return _clients[i];
		}
	}

	return TCPClient(MAX_SOCK_NUM);
}

size_t TCPServer::write(uint8_t b) 
{
	return write(&b, 1);
}

size_t TCPServer::write(const uint8_t *buffer, size_t size) 
{
	size_t n = 0;
	for (int i = 0; i < MAX_SOCK_NUM; ++i)
	{
		if (_clients[i].connected())
		{
			n += _clients[i].write(buffer, size);
		}
	}

	return n;
}
