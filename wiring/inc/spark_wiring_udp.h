/**
 ******************************************************************************
 * @file    spark_wiring_udp.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_udp.cpp module
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

#ifndef __SPARK_WIRING_UDP_H
#define __SPARK_WIRING_UDP_H

#include "spark_wiring.h"
#include "spark_wiring_printable.h"
#include "spark_wiring_stream.h"
#include "socket_hal.h"

class UDP : public Stream, public Printable {
private:
        /**
         * The underlying socket handle from the HAL.
         */
	sock_handle_t _sock;
        
        /**
         * The local port this UDP socket is bound to.
         */
	uint16_t _port;
        
        /**
         * The IP address of the peer that sent the received packet. 
         * Available after parsePacket().
         */
	IPAddress _remoteIP;
        
        /**
         * The port of the peer that send the received packet.
         * Available after parsePacket().
         */
	uint16_t _remotePort;
        	
        /**
         * The current read/write offset in the buffer. Set to 0 after 
         * parsePacket(), incremented during write()
         */
	uint16_t _offset;
        
        /**
         * The number of bytes in the buffer. Available after parsePacket()
         */
        uint16_t _total;
        
        /**
         * The dynamically allocated buffer to store the packet that has been read or
         * the packet that is being written.
         */
        uint8_t* _buffer;
              
        /**
         * The size of the buffer.
         */
        size_t _buffer_size;
        
        /**
         * The network interface this UDP socket should bind to.
         */
        network_interface_t _nif;
        
        /**
         * Set to non-zero if the buffer was dynamically allocated by this class.
         */
        uint8_t _buffer_allocated;
        
        
        
public:
	UDP();

        /**
         * 
         * @param port
         * @param nif
         * @param buffer_size The size of the read/write buffer. Can be 0 if
         * only `readPacket()` and `sendPacket()` are used, as these methods 
         * use client-provided buffers.
         * @param buffer    A pre-allocated buffer. This is optional, and if not specified
         *  the UDP class will allocate the buffer dynamically. 
         * @return non-zero on success
         */
	virtual uint8_t begin(uint16_t port, network_interface_t nif=0, size_t buffer_size=512, uint8_t* buffer=NULL); 
        
	virtual void stop();

        
        virtual int sendPacket(const uint8_t* buffer, size_t buffer_size, IPAddress ip, uint16_t port);
#if 0        
        virtual int sendPacket(const uint8_t* buffer, size_t size, IPAddress destination, uint16_t port);
        virtual int sendPacket(const uint8_t* buffer, size_t size, const char*, uint16_t port);
        
        /**
         * Retrieves a whole packet to a buffer. If the buffer is not large enough
         * for the packet, the remainder that doesn't fit is discarded.
         * 
         * The size of the packet can be determined via available()
         * @param buffer
         * @param buf_size
         * @return 
         */
        virtual int readPacket(uint8_t* buffer, size_t buf_size);
#endif
        
	virtual int beginPacket(IPAddress ip, uint16_t port);
	virtual int beginPacket(const char *host, uint16_t port);
	virtual int endPacket();
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buffer, size_t size);
	virtual int parsePacket();
	virtual int available();
        
        /**
         * Read a single byte from the read buffer. Available after parsePacket(). 
         * @return 
         */
	virtual int read();
	virtual int read(unsigned char* buffer, size_t len);
	                
	virtual int read(char* buffer, size_t len) { return read((unsigned char*)buffer, len); };
	virtual int peek();
	virtual void flush();
        
        
	virtual IPAddress remoteIP() { return _remoteIP; };
	virtual uint16_t remotePort() { return _remotePort; };

        /**
         * Prints the current read parsed packet to the given output.
         * @param p
         * @return
         */
        virtual size_t printTo(Print& p) const;

	using Print::write;
};

#endif
