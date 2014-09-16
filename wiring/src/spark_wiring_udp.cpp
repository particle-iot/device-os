/**
 ******************************************************************************
 * @file    spark_wiring_udp.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Nov-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
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


static bool inline isOpen(long sd)
{
   return sd != MAX_SOCK_NUM;
}

UDP::UDP() : _sock(MAX_SOCK_NUM)
{

}

uint8_t UDP::begin(uint16_t port) 
{
        int bound = 0;
	sockaddr tUDPAddr;

	if(WiFi.ready())
	{
	   _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
           DEBUG("socket=%d",_sock);
           if (_sock >= 0)
            {

              flush();
              _port = port;

              memset(&tUDPAddr, 0, sizeof(tUDPAddr));
              tUDPAddr.sa_family = AF_INET;
              tUDPAddr.sa_data[0] = (_port & 0xFF00) >> 8;
              tUDPAddr.sa_data[1] = (_port & 0x00FF);

              DEBUG("bind socket=%d",_sock);
              bound = bind(_sock, (sockaddr*)&tUDPAddr, sizeof(tUDPAddr)) >= 0;
              DEBUG("socket=%d bound=%d",_sock,bound);

              if(!bound)
              {
                  stop();
              }

            }
	}
        return bound;
}

int UDP::available() 
{
    return _total - _offset;

}

void UDP::stop()
{
  DEBUG("_sock %d closesocket", _sock);
  if (isOpen(_sock))
  {
      int rv = closesocket(_sock);
      DEBUG("_sock %d closed=%d", _sock, rv);
  }
 _sock = MAX_SOCK_NUM;
}

int UDP::beginPacket(const char *host, uint16_t port)
{
        if(WiFi.ready())
        {
	   uint32_t ip_addr = 0;

          if(gethostbyname((char*)host, strlen(host), &ip_addr) > 0)
          {
                  IPAddress remote_addr(BYTE_N(ip_addr, 3), BYTE_N(ip_addr, 2), BYTE_N(ip_addr, 1), BYTE_N(ip_addr, 0));

                  return beginPacket(remote_addr, port);
          }
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

	int rv =  sendto(_sock, buffer, size, 0, &_remoteSockAddr, _remoteSockAddrLen);
        DEBUG("sendto(buffer=%lx, size=%d)=%d",buffer, size , rv);
	return rv;
}

int UDP::parsePacket()
{
  // No data buffered
  if(available() == 0 && WiFi.ready() && isOpen(_sock))
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

                      int ret = recvfrom(_sock, _buffer, arraySize(_buffer), 0, &_remoteSockAddr, &_remoteSockAddrLen);

                      if (ret > 0)
                      {
                              _remotePort = _remoteSockAddr.sa_data[0] << 8 | _remoteSockAddr.sa_data[1];

                              _remoteIP._address[0] = _remoteSockAddr.sa_data[2];
                              _remoteIP._address[1] = _remoteSockAddr.sa_data[3];
                              _remoteIP._address[2] = _remoteSockAddr.sa_data[4];
                              _remoteIP._address[3] = _remoteSockAddr.sa_data[5];

                              _offset = 0;
                              _total = ret;
                      }
              }
      }
   }
   return available();
}

int UDP::read()
{
  return available() ? _buffer[_offset++] : -1;
}

int UDP::read(unsigned char* buffer, size_t len)
{
        int read = -1;
        if (available())
	{
          read = (len > (size_t) available()) ? available() : len;
          memcpy(buffer, &_buffer[_offset], read);
          _offset += read;
	}
	return read;
}

int UDP::peek()
{
     return available() ? _buffer[_offset] : -1;
}

void UDP::flush()
{
  _offset = 0;
  _total = 0;

}
