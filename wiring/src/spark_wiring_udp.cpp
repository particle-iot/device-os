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
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_COMPAT

#include "spark_wiring_udp.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "spark_macros.h"
#include "spark_wiring_network.h"
#include "spark_wiring_constants.h"

using namespace spark;

static bool inline isOpen(sock_handle_t sd)
{
   return sd != socket_handle_invalid();
}

UDP::UDP() : _sock(socket_handle_invalid()), _offset(0), _total(0), _buffer(0), _buffer_size(512)
{
}

bool UDP::setBuffer(size_t buf_size, uint8_t* buffer)
{
    releaseBuffer();

    _buffer = buffer;
    _buffer_size = 0;
    if (!_buffer && buf_size) {         // requested allocation
        _buffer = new uint8_t[buf_size];
        _buffer_allocated = true;
    }
    if (_buffer) {
        _buffer_size = buf_size;
    }
    return _buffer_size;
}

void UDP::releaseBuffer()
{
    if (_buffer_allocated && _buffer) {
        delete _buffer;
    }
    _buffer = NULL;
    _buffer_allocated = false;
    _buffer_size = 0;
    flush_buffer(); // clear buffer
}

uint8_t UDP::begin(uint16_t port, network_interface_t nif)
{
    bool bound = 0;
    if(Network.from(nif).ready())
    {
       _sock = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, port, nif);
        DEBUG("socket=%d",_sock);
        if (socket_handle_valid(_sock))
        {
            flush_buffer(); // clear buffer
            _port = port;
            _nif = nif;
            bound = true;
        }
        else {
            stop();
            bound = false;
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
        socket_close(_sock);
    }
    _sock = socket_handle_invalid();

    flush_buffer(); // clear buffer
}

int UDP::beginPacket(const char *host, uint16_t port)
{
    if(Network.from(_nif).ready())
    {
        HAL_IPAddress ip_addr;

        if(inet_gethostbyname((char*)host, strlen(host), &ip_addr, _nif, NULL) == 0)
        {
            IPAddress remote_addr(ip_addr);
            return beginPacket(remote_addr, port);
        }
    }
    return 0;
}

int UDP::beginPacket(IPAddress ip, uint16_t port)
{
    // default behavior previously was to use a 512 byte buffer, so instantiate that if not already done
    if (!_buffer && _buffer_size) {
        setBuffer(_buffer_size);
    }

    _remoteIP = ip;
    _remotePort = port;
    flush_buffer(); // clear buffer
    return _buffer_size;
}

int UDP::endPacket()
{
    int result = sendPacket(_buffer, _offset, _remoteIP, _remotePort);
    flush(); // wait for send to complete
    return result;
}

int UDP::sendPacket(const uint8_t* buffer, size_t buffer_size, IPAddress remoteIP, uint16_t port)
{
    sockaddr_t remoteSockAddr;
    remoteSockAddr.sa_family = AF_INET;

    remoteSockAddr.sa_data[0] = (port & 0xFF00) >> 8;
    remoteSockAddr.sa_data[1] = (port & 0x00FF);

    remoteSockAddr.sa_data[2] = remoteIP[0];
    remoteSockAddr.sa_data[3] = remoteIP[1];
    remoteSockAddr.sa_data[4] = remoteIP[2];
    remoteSockAddr.sa_data[5] = remoteIP[3];

    int rv = socket_sendto(_sock, buffer, buffer_size, 0, &remoteSockAddr, sizeof(remoteSockAddr));
    DEBUG("sendto(buffer=%lx, size=%d)=%d",buffer, buffer_size , rv);
    return rv;
}

size_t UDP::write(uint8_t byte)
{
    return write(&byte, 1);
}

size_t UDP::write(const uint8_t *buffer, size_t size)
{
    size_t available = _buffer ? _buffer_size - _offset : 0;
    if (size>available)
        size = available;
    memcpy(_buffer+_offset, buffer, size);
    _offset += size;
    return size;
}

int UDP::parsePacket(system_tick_t timeout)
{
    if (!_buffer && _buffer_size) {
        setBuffer(_buffer_size);
    }

    flush_buffer();         // start a new read - discard the old data
    if (_buffer && _buffer_size) {
        int result = receivePacket(_buffer, _buffer_size);
        if (result>0) {
            _total = result;
        }
    };
    return available();
}

int UDP::receivePacket(uint8_t* buffer, size_t size, system_tick_t timeout)
{
    int ret = -1;
    if(Network.from(_nif).ready() && isOpen(_sock) && buffer)
    {
        sockaddr_t remoteSockAddr;
        socklen_t remoteSockAddrLen = sizeof(remoteSockAddr);

        ret = socket_receivefrom(_sock, buffer, size, 0, &remoteSockAddr, &remoteSockAddrLen);
        if (ret >= 0)
        {
            _remotePort = remoteSockAddr.sa_data[0] << 8 | remoteSockAddr.sa_data[1];
            _remoteIP = &remoteSockAddr.sa_data[2];
        }
    }
    return ret;
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
    read = min(int(len), available());
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
}

void UDP::flush_buffer()
{
  _offset = 0;
  _total = 0;
}

size_t UDP::printTo(Print& p) const
{
    // can't use available() since this is a `const` method, and available is part of the Stream interface, and is non-const.
    int size = _total - _offset;
    return p.write(_buffer+_offset, size);
}

int UDP::joinMulticast(const IPAddress& ip)
{
    if (_sock == socket_handle_invalid())
        return -1;
    HAL_IPAddress address = ip.raw();
    socket_multicast_info_t info;
    info.size = sizeof(info);
    info.sock_handle = _sock;
    return socket_join_multicast(&address, _nif, &info);
}

int UDP::leaveMulticast(const IPAddress& ip)
{
    if (_sock == socket_handle_invalid())
        return -1;
    HAL_IPAddress address = ip.raw();
    return socket_leave_multicast(&address, _nif, 0);
}

#endif // HAL_USE_SOCKET_HAL_COMPAT
