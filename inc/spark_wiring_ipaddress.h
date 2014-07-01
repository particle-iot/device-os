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

#include "spark_wiring.h"

class IPAddress : public Printable {
private:
	uint8_t _address[4];  // IPv4 address

public:
	// Access the raw byte array containing the address.  Because this returns a pointer
	// to the internal structure rather than a copy of the address this function should only
	// be used when you know that the usage of the returned uint8_t* will be transient and not
	// stored.
	uint8_t* raw_address() { return _address; };
	// Constructors
	IPAddress();
	IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
	IPAddress(uint32_t address);
	IPAddress(const uint8_t* address);

        virtual ~IPAddress() {}

	// Overloaded cast operator to allow IPAddress objects to be used where a pointer
	// to a four-byte uint8_t array, uint32_t or another IPAddress object is expected.
	bool operator==(uint32_t address);
	bool operator==(const uint8_t* address);
	bool operator==(const IPAddress& address);

	// Overloaded index operator to allow getting and setting individual octets of the address
	uint8_t operator[](int index) const { return _address[index]; };
	uint8_t& operator[](int index) { return _address[index]; };

	// Overloaded copy operators to allow initialisation of IPAddress objects from other types
	IPAddress& operator=(const uint8_t* address);
	IPAddress& operator=(uint32_t address);

	virtual size_t printTo(Print& p) const;

	friend class TCPClient;
	friend class TCPServer;
	friend class UDP;
};

const IPAddress INADDR_NONE(0,0,0,0);

#endif
