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
#include "spark_wiring_network.h"
#include "system_task.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "spark_macros.h"

using namespace spark;

static bool inline isOpen(sock_handle_t sd)
{
   return socket_handle_valid(sd);
}

TCPClient::TCPClient() : TCPClient(socket_handle_invalid())
{
}

TCPClient::TCPClient(sock_handle_t sock) :
        d_(std::make_shared<Data>(sock))
{
  flush_buffer();
}

int TCPClient::connect(const char* host, uint16_t port, network_interface_t nif)
{
    stop();
      int rv = 0;
      if(Network.ready())
      {
        IPAddress ip_addr;

        if((rv = inet_gethostbyname(host, strlen(host), ip_addr, nif, NULL)) == 0)
        {
                return connect(ip_addr, port, nif);
        }
        else
            DEBUG("unable to get IP for hostname");
      }
      return rv;
}

int TCPClient::connect(IPAddress ip, uint16_t port, network_interface_t nif)
{
    stop();
        int connected = 0;
        if(Network.from(nif).ready())
        {
          sockaddr_t tSocketAddr;
          d_->sock = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, port, nif);
          LOG(TRACE, "TCPClient socket=%x", d_->sock);

          if (socket_handle_valid(d_->sock))
          {
            flush_buffer();

            tSocketAddr.sa_family = AF_INET;

            tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
            tSocketAddr.sa_data[1] = (port & 0x00FF);

            tSocketAddr.sa_data[2] = ip[0];        // Todo IPv6
            tSocketAddr.sa_data[3] = ip[1];
            tSocketAddr.sa_data[4] = ip[2];
            tSocketAddr.sa_data[5] = ip[3];


            uint32_t ot = HAL_NET_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
            DEBUG("sock %d connect",d_->sock);
            connected = (socket_connect(d_->sock, &tSocketAddr, sizeof(tSocketAddr)) == 0 ? 1 : 0);
            DEBUG("sock %d connected=%d",d_->sock, connected);
            HAL_NET_SetNetWatchDog(ot);
            d_->remoteIP = ip;
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
    return write(&b, 1, SPARK_WIRING_TCPCLIENT_DEFAULT_SEND_TIMEOUT);
}

size_t TCPClient::write(const uint8_t *buffer, size_t size)
{
    return write(buffer, size, SPARK_WIRING_TCPCLIENT_DEFAULT_SEND_TIMEOUT);
}

size_t TCPClient::write(uint8_t b, system_tick_t timeout)
{
    return write(&b, 1, timeout);
}

size_t TCPClient::write(const uint8_t *buffer, size_t size, system_tick_t timeout)
{
    clearWriteError();
    int ret = status() ? socket_send_ex(d_->sock, buffer, size, 0, timeout, nullptr) : -1;
    if (ret < 0) {
        setWriteError(ret);
    }

    /*
     * FIXME: We should not be returning negative numbers here
     */
    return ret;
}

int TCPClient::bufferCount()
{
  return d_->total - d_->offset;
}

int TCPClient::available()
{
    int avail = 0;

    // At EOB => Flush it
    if (d_->total && (d_->offset == d_->total))
    {
        flush_buffer();
    }

    if(Network.from(nif).ready() && isOpen(d_->sock))
    {
        // Have room
        if ( d_->total < arraySize(d_->buffer))
        {
            int ret = socket_receive(d_->sock, d_->buffer + d_->total , arraySize(d_->buffer)-d_->total, 0);
            if (ret > 0)
            {
                DEBUG("recv(=%d)",ret);
                if (d_->total == 0) d_->offset = 0;
                d_->total += ret;
            }
        } // Have Space
    } // WiFi.ready() && isOpen(d_->sock)
    avail = bufferCount();
    return avail;
}

int TCPClient::read()
{
  return (bufferCount() || available()) ? d_->buffer[d_->offset++] : -1;
}

int TCPClient::read(uint8_t *buffer, size_t size)
{
        int read = -1;
        if (bufferCount() || available())
        {
          read = (size > (size_t) bufferCount()) ? bufferCount() : size;
          memcpy(buffer, &d_->buffer[d_->offset], read);
          d_->offset += read;
        }
        return read;
}

int TCPClient::peek()
{
  return  (bufferCount() || available()) ? d_->buffer[d_->offset] : -1;
}

void TCPClient::flush_buffer()
{
  d_->offset = 0;
  d_->total = 0;
}

void TCPClient::flush()
{
}


void TCPClient::stop()
{
  // This log line pollutes the log too much
  // DEBUG("sock %d closesocket", d_->sock);

  if (isOpen(d_->sock))
      socket_close(d_->sock);
  d_->sock = socket_handle_invalid();
  d_->remoteIP.clear();
  flush_buffer();
}

uint8_t TCPClient::connected()
{
  // Wlan up, open and not in CLOSE_WAIT or data still in the local buffer
  bool rv = (status() || bufferCount());
  // no data in the local buffer, Socket open but my be in CLOSE_WAIT yet the CC3000 may have data in its buffer
  if(!rv && isOpen(d_->sock) && (SOCKET_STATUS_INACTIVE == socket_active_status(d_->sock)))
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
  return (isOpen(d_->sock) && Network.from(nif).ready() && (SOCKET_STATUS_ACTIVE == socket_active_status(d_->sock)));
}

TCPClient::operator bool()
{
   return (status()!=0);
}

IPAddress TCPClient::remoteIP()
{
    return d_->remoteIP;
}

TCPClient::Data::Data(sock_handle_t sock)
        : sock(sock),
          offset(0),
          total(0) {
}

TCPClient::Data::~Data() {
    if (socket_handle_valid(sock)) {
        socket_close(sock);
    }
}

#endif // HAL_USE_SOCKET_HAL_COMPAT
