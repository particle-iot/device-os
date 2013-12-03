/**
 ******************************************************************************
 * @file    spark_wiring_tcpclient.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Nov-2013
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

uint16_t TCPClient::_srcport = 1024;

TCPClient::TCPClient() : _sock(MAX_SOCK_NUM)
{

}

TCPClient::TCPClient(uint8_t sock) : _sock(sock) 
{

}

int TCPClient::connect(const char* host, uint16_t port) 
{
	uint32_t ip_addr = 0;

	if(gethostbyname((char*)host, strlen(host), &ip_addr) > 0)
	{
		IPAddress remote_addr(BYTE_N(ip_addr, 3), BYTE_N(ip_addr, 2), BYTE_N(ip_addr, 1), BYTE_N(ip_addr, 0));

		return connect(remote_addr, port);
	}

	return 0;
}

int TCPClient::connect(IPAddress ip, uint16_t port) 
{
	sockaddr tSocketAddr;

	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_sock < 0)
	{
		return 0;
	}

	tSocketAddr.sa_family = AF_INET;

	tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
	tSocketAddr.sa_data[1] = (port & 0x00FF);

	tSocketAddr.sa_data[2] = ip._address[0];
	tSocketAddr.sa_data[3] = ip._address[1];
	tSocketAddr.sa_data[4] = ip._address[2];
	tSocketAddr.sa_data[5] = ip._address[3];

	if(socket_connect(_sock, &tSocketAddr, sizeof(tSocketAddr)) < 0)
	{
		_sock = MAX_SOCK_NUM;

		return 0;
	}

	return 1;
}

size_t TCPClient::write(uint8_t b) 
{
	return write(&b, 1);
}

size_t TCPClient::write(const uint8_t *buf, size_t size) 
{
	return send(_sock, buf, size, 0);
}

int TCPClient::available() 
{
	_types_fd_set_cc3000 readSet;
	timeval timeout;

	FD_ZERO(&readSet);
	FD_SET(_sock, &readSet);

	timeout.tv_sec = 0;
	timeout.tv_usec = 5000;

	if (select(_sock + 1, &readSet, NULL, NULL, &timeout) > 0)
	{
		if (FD_ISSET(_sock, &readSet))
		{
			return 1;
		}
	}

	return 0;
}

int TCPClient::read() 
{
	uint8_t b;
	if (recv(_sock, &b, 1, 0) > 0)
	{
		return b;
	}
	else
	{
		return -1;
	}
}

int TCPClient::read(uint8_t *buf, size_t size) 
{
	return recv(_sock, buf, size, 0);
}

int TCPClient::peek() 
{
	if (!available())
	{
		return -1;
	}

	return read();
}

void TCPClient::flush() 
{
	while (available())
	{
		read();
	}
}

void TCPClient::stop() 
{
	if (closesocket(_sock) == 0)
	{
		_sock = MAX_SOCK_NUM;
	}
}

uint8_t TCPClient::connected() 
{
	if (_sock == MAX_SOCK_NUM)
	{
		return 0;
	}

	//To Do

	return 1;
}

uint8_t TCPClient::status() 
{
	if (_sock == MAX_SOCK_NUM)
	{
		return 0;
	}

	//To Do

	return 1;
}

TCPClient::operator bool()
{
	return _sock != MAX_SOCK_NUM;
}
