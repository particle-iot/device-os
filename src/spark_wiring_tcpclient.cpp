/**
 ******************************************************************************
 * @file    spark_wiring_tcpclient.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Nov-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
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

static bool inline isOpen(long sd)
{
   return sd != MAX_SOCK_NUM;
}

TCPClient::TCPClient() : _sock(MAX_SOCK_NUM)
{
  flush();
}

TCPClient::TCPClient(uint8_t sock) : _sock(sock) 
{
  flush();
}

int TCPClient::connect(const char* host, uint16_t port) 
{
      int rv = 0;
      if(WiFi.ready())
      {

        uint32_t ip_addr = 0;

        if(gethostbyname((char*)host, strlen(host), &ip_addr) > 0)
        {
                IPAddress remote_addr(BYTE_N(ip_addr, 3), BYTE_N(ip_addr, 2), BYTE_N(ip_addr, 1), BYTE_N(ip_addr, 0));

                return connect(remote_addr, port);
        }
      }
      return rv;
}

int TCPClient::connect(IPAddress ip, uint16_t port) 
{
        int connected = 0;
        if(WiFi.ready())
        {
          sockaddr tSocketAddr;
          _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
          DEBUG("socket=%d",_sock);

          if (_sock >= 0)
          {
            flush();

            tSocketAddr.sa_family = AF_INET;

            tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
            tSocketAddr.sa_data[1] = (port & 0x00FF);

            tSocketAddr.sa_data[2] = ip._address[0];
            tSocketAddr.sa_data[3] = ip._address[1];
            tSocketAddr.sa_data[4] = ip._address[2];
            tSocketAddr.sa_data[5] = ip._address[3];


            uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
            DEBUG("_sock %d connect",_sock);
            connected = (socket_connect(_sock, &tSocketAddr, sizeof(tSocketAddr)) >= 0 ? 1 : 0);
            DEBUG("_sock %d connected=%d",_sock, connected);
            SPARK_WLAN_SetNetWatchDog(ot);

            if(!connected)
            {
                stop();
            }
          }
        }
        return connected;
}

size_t TCPClient::write(uint8_t b) 
{
        return write(&b, 1);
}

size_t TCPClient::write(const uint8_t *buffer, size_t size)
{
        return status() ? send(_sock, buffer, size, 0) : -1;
}

int TCPClient::bufferCount()
{
  return _total - _offset;
}

int TCPClient::available() 
{
    int avail = 0;

    // At EOB => Flush it
    if (_total && (_offset == _total))
    {
      flush();
    }

    if(WiFi.ready() && isOpen(_sock))
    {
        // Have room
        if ( _total < arraySize(_buffer))
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
                  int ret = recv(_sock, _buffer + _total , arraySize(_buffer)-_total, 0);
                  DEBUG("recv(=%d)",ret);
                  if (ret > 0)
                  {
                      if (_total == 0) _offset = 0;
                      _total += ret;
                  }
              }
          } // Select
          } // Have Space
    } // WiFi.ready() && isOpen(_sock)
    avail = bufferCount();
    return avail;
}

int TCPClient::read() 
{

  return (bufferCount() || available()) ? _buffer[_offset++] : -1;
}

int TCPClient::read(uint8_t *buffer, size_t size)
{
        int read = -1;
        if (bufferCount() || available())
        {
          read = (size > (size_t) bufferCount()) ? bufferCount() : size;
          memcpy(buffer, &_buffer[_offset], read);
          _offset += read;
        }
        return read;
}

int TCPClient::peek() 
{
  return  (bufferCount() || available()) ? _buffer[_offset] : -1;
}

void TCPClient::flush() 
{
  _offset = 0;
  _total = 0;
}

void TCPClient::stop() 
{
  DEBUG("_sock %d closesocket", _sock);

  if (isOpen(_sock))
  {
      int rv = closesocket(_sock);
      DEBUG("_sock %d closed=%d", _sock, rv);
  }
 _sock = MAX_SOCK_NUM;
}

uint8_t TCPClient::connected() 
{
  // Wlan up, open and not in CLOSE_WAIT or data still in the local buffer
  bool rv = ( 1 == status() || bufferCount()) ? true : false;
  // no data in the local buffer, Socket open but my be in CLOSE_WAIT yet the CC3000 may have data in its buffer
  if(!rv && isOpen(_sock) && (SOCKET_STATUS_INACTIVE == get_socket_active_status(_sock)))
    {
      rv = available(); // Try CC3000
      if (!rv) {        // No more Data and CLOSE_WAIT
          DEBUG("caling Stop No more Data and in CLOSE_WAIT");
          stop();       // Close our side
      }
  }
  return rv;
}

uint8_t TCPClient::status()
{
  return (isOpen(_sock) && WiFi.ready() && (SOCKET_STATUS_ACTIVE == get_socket_active_status(_sock)) ? 1 : 0);

}

TCPClient::operator bool()
{
   return (status() ? 1 : 0);
}
