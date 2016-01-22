/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "application.h"
#include "unit-test/unit-test.h"

test(wifi_resolve_3_levels)
{
    IPAddress address = WiFi.resolve("pool.ntp.org");
    assertNotEqual(address[0], 0);

    // ensure the version field is set
    assertNotEqual(address.version(), 0);

    IPAddress compare = IPAddress(address[0], address[1], address[2], address[3]);
    assertTrue(compare==address);
}

test(wifi_resolve_4_levels)
{
    IPAddress address = WiFi.resolve("north-america.pool.ntp.org");
    assertNotEqual(address[0], 0);
}

void checkIPAddress(const char* name, const IPAddress& address)
{
	if (address.version()==0 || address[0]==0)
	{
		Serial.print("address failed:");
		Serial.println(name);
		assertNotEqual(address.version(), 0);
		assertNotEqual(address[0], 0);
	}
}

void checkEtherAddress(const uint8_t* address)
{
	uint8_t sum = 0;
	for (int i=0; i<6; i++)
	{
		sum |= address[i];
	}
	assertNotEqual(sum, 0);
}

test(wifi_config)
{
	checkIPAddress("local", WiFi.localIP());

// WICED doesn't report DHCP or DNS server
#if PLATFORM_ID!=6 && PLATFORM_ID!=8
	checkIPAddress("dnsServer", WiFi.dnsServerIP());
	checkIPAddress("dhcpServer", WiFi.dhcpServerIP());
#endif
	checkIPAddress("gateway", WiFi.gatewayIP());

	uint8_t ether[6];
	uint8_t ether2[6];
	memset(ether, 0, 6);
	memset(ether2, 0, 6);

	assertTrue(WiFi.macAddress(ether)==ether);
	assertTrue(WiFi.macAddress(ether2)==ether2);
	checkEtherAddress(ether);
	assertTrue(!memcmp(ether, ether2, 6))

	memset(ether, 0, 6);
	memset(ether2, 0, 6);
	assertTrue(WiFi.BSSID(ether)==ether);
	assertTrue(WiFi.BSSID(ether2)==ether2);
	checkEtherAddress(ether);
	assertTrue(!memcmp(ether, ether2, 6))
}
