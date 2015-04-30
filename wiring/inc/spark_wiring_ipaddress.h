/**
 ******************************************************************************
 * @file    spark_wiring_ipaddress.h
 * @authors Satish Nair, Technobly
 * @version V1.0.0
 * @date    10-Nov-2013
 * @brief   Header for spark_wiring_ipaddress.cpp module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
  Copyright (c) 2011 Adrian McEwen

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

#ifndef __SPARK_WIRING_IPADDRESS_H
#define __SPARK_WIRING_IPADDRESS_H

#include <stdint.h>

#include "spark_wiring_printable.h"
#include "inet_hal.h"

class IPAddress : public Printable {
private:
    
    HAL_IPAddress address;
    
public:
	// Access the raw byte array containing the address.  Because this returns a pointer
	// to the internal structure rather than a copy of the address this function should only
	// be used when you know that the usage of the returned uint8_t* will be transient and not
	// stored.
	const uint8_t* raw_address() const { return address.ipv4; };
	// Constructors
	IPAddress();
	IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
	IPAddress(uint32_t address);
	IPAddress(const uint8_t* address);
        
        IPAddress(HAL_IPAddress);

        virtual ~IPAddress() {}
        
        operator bool();

	// Overloaded cast operator to allow IPAddress objects to be used where a pointer
	// to a four-byte uint8_t array, uint32_t or another IPAddress object is expected.
	bool operator==(uint32_t address);
	bool operator==(const uint8_t* address);
	bool operator==(const IPAddress& address);

	// Overloaded index operator to allow getting and setting individual octets of the address
	uint8_t operator[](int index) const { return address.ipv4[index]; };
	uint8_t& operator[](int index) { return address.ipv4[index]; };

	// Overloaded copy operators to allow initialisation of IPAddress objects from other types
	IPAddress& operator=(const uint8_t* address);
	IPAddress& operator=(uint32_t address);

        operator const HAL_IPAddress& () {
            return address;
        }
        
        const HAL_IPAddress& raw() const {
            return address;
        }
                
        HAL_IPAddress& raw() {
            return address;
        }


        void set_ipv4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
        
	virtual size_t printTo(Print& p) const;
       

	friend class TCPClient;
	friend class TCPServer;
	friend class UDP;
};

extern const IPAddress INADDR_NONE;

#endif
