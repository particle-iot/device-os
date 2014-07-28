/**
 ******************************************************************************
 * @file    spark_wiring_ipaddress.cpp
 * @authors Satish Nair, Technobly
 * @version V1.0.0
 * @date    10-Nov-2013
 * @brief   
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

#include "spark_wiring_ipaddress.h"

IPAddress::IPAddress()
{
	memset(_address, 0, sizeof(_address));
}

IPAddress::IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet)
{
	_address[0] = first_octet;
	_address[1] = second_octet;
	_address[2] = third_octet;
	_address[3] = fourth_octet;
}

IPAddress::IPAddress(uint32_t address)
{
	// done this way because endianness of octet vs. array order doesn't work correctly with memcpy()
	_address[3] = address & 0x000f;
	_address[2] = (address >> 8) & 0x000f;
	_address[1] = (address >> 16) & 0x000f;
	_address[0] = (address >> 24) & 0x000f;
}

IPAddress::IPAddress(const uint8_t* address)
{
	memcpy(_address, address, sizeof(_address));
}

IPAddress& IPAddress::operator=(const uint8_t* address)
{
	memcpy(_address, address, sizeof(_address));
	return *this;
}

IPAddress& IPAddress::operator=(uint32_t address)
{
	// done this way because endianness of octet vs. array order doesn't work correctly with memcpy()
	_address[3] = address & 0x000f;
	_address[2] = (address >> 8) & 0x000f;
	_address[1] = (address >> 16) & 0x000f;
	_address[0] = (address >> 24) & 0x000f;
	return *this;
}

bool IPAddress::operator==(uint32_t address)
{ 
	// done this way because endianness of octet vs. array order doesn't work correctly with memcmp()
	return (_address[3] == (address & 0x000f) &&
		_address[2] == ((address >> 8) & 0x000f) &&
		_address[1] == ((address >> 16) & 0x000f) &&
		_address[0] == ((address >> 24) & 0x000f));
}

bool IPAddress::operator==(const uint8_t* address)
{
	return memcmp(address, _address, sizeof(_address)) == 0;
}

bool IPAddress::operator==(const IPAddress& address)
{
	return memcmp(address._address, _address, sizeof(_address)) == 0;
}

size_t IPAddress::printTo(Print& p) const
{
	size_t n = 0;
	for (int i =0; i < 3; i++)
	{
		n += p.print(_address[i], DEC);
		n += p.print('.');
	}
	n += p.print(_address[3], DEC);
	return n;
}
