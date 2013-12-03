/**
 ******************************************************************************
 * @file    spark_wiring_udp.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Nov-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
  Copyright (c) 2008 Bjoern Hartmann

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

#include "spark_wiring_udp.h"

UDP::UDP() : _sock(MAX_SOCK_NUM)
{

}

uint8_t UDP::begin(uint16_t port) 
{
	sockaddr tUDPAddr;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock < 0)
	{
		return 0;
	}

	_sock = sock;
	_port = port;
	_offset = 0;
	_remaining = 0;

	tUDPAddr.sa_family = AF_INET;

	tUDPAddr.sa_data[0] = (_port & 0xFF00) >> 8;
	tUDPAddr.sa_data[1] = (_port & 0x00FF);
	tUDPAddr.sa_data[2] = 0;
	tUDPAddr.sa_data[3] = 0;
	tUDPAddr.sa_data[4] = 0;
	tUDPAddr.sa_data[5] = 0;

	if (bind(_sock, (sockaddr*)&tUDPAddr, sizeof(tUDPAddr)) < 0)
	{
		return 0;
	}

	return 1;
}

int UDP::available() 
{
	return _remaining;
}

void UDP::stop()
{
	if (closesocket(_sock) == 0)
	{
		_sock = MAX_SOCK_NUM;
	}
}

int UDP::beginPacket(const char *host, uint16_t port)
{
	uint32_t ip_addr = 0;

	if(gethostbyname((char*)host, strlen(host), &ip_addr) > 0)
	{
		IPAddress remote_addr(BYTE_N(ip_addr, 3), BYTE_N(ip_addr, 2), BYTE_N(ip_addr, 1), BYTE_N(ip_addr, 0));

		return beginPacket(remote_addr, port);
	}

	return 0;
}

int UDP::beginPacket(IPAddress ip, uint16_t port)
{
	_remoteIP = ip;
	_remotePort = port;

	_remoteSockAddr.sa_family = AF_INET;

	_remoteSockAddr.sa_data[0] = (_remotePort & 0xFF00) >> 8;
	_remoteSockAddr.sa_data[1] = (_remotePort & 0x00FF);

	_remoteSockAddr.sa_data[2] = _remoteIP._address[0];
	_remoteSockAddr.sa_data[3] = _remoteIP._address[1];
	_remoteSockAddr.sa_data[4] = _remoteIP._address[2];
	_remoteSockAddr.sa_data[5] = _remoteIP._address[3];

	_remoteSockAddrLen = sizeof(_remoteSockAddr);

	return 1;
}

int UDP::endPacket()
{
	return 1;
}

size_t UDP::write(uint8_t byte)
{
	return write(&byte, 1);
}

size_t UDP::write(const uint8_t *buffer, size_t size)
{
	return sendto(_sock, buffer, size, 0, &_remoteSockAddr, _remoteSockAddrLen);
}

int UDP::parsePacket()
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
			int ret = recvfrom(_sock, _buffer, RX_BUF_MAX_SIZE, 0, &_remoteSockAddr, &_remoteSockAddrLen);

			if (ret > 0)
			{
				_remotePort = _remoteSockAddr.sa_data[0] << 8;
				_remotePort = _remoteSockAddr.sa_data[1] | _remotePort;

				_remoteIP._address[0] = _remoteSockAddr.sa_data[2];
				_remoteIP._address[1] = _remoteSockAddr.sa_data[3];
				_remoteIP._address[2] = _remoteSockAddr.sa_data[4];
				_remoteIP._address[3] = _remoteSockAddr.sa_data[5];

				_offset = 0;
				_remaining = ret;
			}

			return ret;
		}
	}

	return 0;
}

int UDP::read()
{
	uint8_t byte;

	if ((_remaining > 0) && (_offset < RX_BUF_MAX_SIZE))
	{
		byte = _buffer[_offset++];
		_remaining--;
		return byte;
	}

	return -1;
}

int UDP::read(unsigned char* buffer, size_t len)
{
	if ((_remaining > 0) && (_offset < RX_BUF_MAX_SIZE))
	{
		if (_remaining <= len)
		{
			memcpy(buffer, _buffer, _remaining);
			_offset = _remaining;
		}
		else
		{
			memcpy(buffer, _buffer, len);
			_offset = len;
		}

		if (_offset > 0)
		{
			_remaining -= _offset;
			return _offset;
		}
	}

	return -1;
}

int UDP::peek()
{
	if (!available())
	{
		return -1;
	}

	return read();
}

void UDP::flush()
{
	while (available())
	{
		read();
	}
}
