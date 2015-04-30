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
#include "spark_wiring_print.h"
#include "string.h"

IPAddress::IPAddress()
{
    memset(&address, 0, sizeof (address));
}

IPAddress::IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet)
{
    set_ipv4(first_octet, second_octet, third_octet, fourth_octet);        
}

IPAddress::IPAddress(uint32_t address)
{
    *this = address;
}

IPAddress::IPAddress(const uint8_t* address)
{    
    *this = address;
}

IPAddress::operator bool()
{
#if Wiring_IPv6
#error handle me!    
#else
    return address.u32!=0;
#endif    
}

void IPAddress::set_ipv4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    address.ipv4[0] = b0;
    address.ipv4[1] = b1;
    address.ipv4[2] = b2;
    address.ipv4[3] = b3;
}


IPAddress& IPAddress::operator=(const uint8_t* address)
{
    set_ipv4(address[0], address[1], address[2], address[3]);
    return *this;
}

IPAddress& IPAddress::operator=(uint32_t address)
{
    set_ipv4(address & 0x000f, (address >> 8) & 0x000f, (address >> 16) & 0x000f, (address >> 24) & 0x000f);    
    return *this;
}

bool IPAddress::operator==(uint32_t address)
{
    return IPAddress(address)==*this;
}

bool IPAddress::operator==(const uint8_t* address)
{
    return IPAddress(address)==*this;
}

bool IPAddress::operator==(const IPAddress& that)
{
    return memcmp(&this->address, &that.address, sizeof (address)) == 0;
}

size_t IPAddress::printTo(Print& p) const
{
    size_t n = 0;
    for (int i = 0; i < 3; i++)
    {
        if (n)
            n += p.print('.');
        n += p.print((*this)[i], DEC);
    }    
    return n;
}

const IPAddress INADDR_NONE(0, 0, 0, 0);