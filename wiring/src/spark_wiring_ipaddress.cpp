/**
 ******************************************************************************
 * @file    spark_wiring_ipaddress.cpp
 * @authors Satish Nair, Technobly
 * @version V1.0.0
 * @date    10-Nov-2013
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
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
#include "spark_wiring_platform.h"
#include "string.h"

#if HAL_USE_INET_HAL_POSIX
#include <arpa/inet.h>
#endif // HAL_USE_INET_HAL_POSIX

IPAddress::IPAddress()
{
    clear();
}

IPAddress::IPAddress(const HAL_IPAddress& address)
{
    memcpy(&this->address, &address, sizeof(address));
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

IPAddress::operator bool() const
{
#if Wiring_IPv6
    if (version() == 4) {
        return address.ipv4 != 0;
    } else if (version() == 6) {
        return address.ipv6[0] != 0 && address.ipv6[1] != 0 && address.ipv6[2] != 0 && address.ipv6[3] != 0;
    } else {
        return false;
    }
#else
    return address.ipv4!=0;
#endif
}

void IPAddress::set_ipv4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    address.ipv4 = b0<<24 | b1 << 16 | b2 << 8 | b3;
    setVersion(4);
}

IPAddress& IPAddress::operator=(const uint8_t* address)
{
    set_ipv4(address[0], address[1], address[2], address[3]);
    return *this;
}

IPAddress& IPAddress::operator=(uint32_t ipv4)
{
    address.ipv4 = ipv4;
    setVersion(4);
    return *this;
}

bool IPAddress::operator==(uint32_t ipv4) const
{
    return ipv4==address.ipv4;
}

bool IPAddress::operator==(const uint8_t* address) const
{
    return IPAddress(address)==*this;
}

bool IPAddress::operator==(const IPAddress& that) const
{
#if Wiring_IPv6
	if (address.v!=that.address.v)
		return false;
	if (address.v==6)
		return !memcmp(address.ipv6, that.address.ipv6, sizeof(address.ipv6));
#endif // Wiring_IPv6
	return address.ipv4==that.address.ipv4;
}

size_t IPAddress::printTo(Print& p) const
{
#if Wiring_IPv6
#if HAL_USE_INET_HAL_POSIX
	if (address.v==6) {
		char buf[INET6_ADDRSTRLEN+1];
		buf[0] = 0;
		inet_inet_ntop(AF_INET6, address.ipv6, buf, sizeof(buf));
		return p.write(buf);
	}
#else
#pragma message "HAL_USE_INET_HAL_POSIX is required for IPv6 support in IPAddress::printTo()"
#endif // HAL_USE_INET_HAL_POSIX
#endif // Wiring_IPv6
    size_t n = 0;
    for (int i = 0; i < 4; i++)
    {
        if (n)
            n += p.print('.');
        n += p.print((*this)[i], DEC);
    }
    return n;
}
