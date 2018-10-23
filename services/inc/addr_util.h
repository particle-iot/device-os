/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstring>
#include <cstdint>

namespace particle {

const size_t MAC_ADDRESS_SIZE = 6;

struct MacAddress {
    uint8_t data[MAC_ADDRESS_SIZE];
};

const MacAddress INVALID_MAC_ADDRESS = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

const size_t MAC_ADDRESS_STRING_SIZE = 17;

bool macAddressToString(const MacAddress& addr, char* str, size_t size);
bool macAddressFromString(MacAddress* addr, const char* str, size_t size);

inline bool macAddressFromString(MacAddress* addr, const char* str) {
    return macAddressFromString(addr, str, strlen(str));
}

inline bool operator==(const MacAddress& addr1, const MacAddress& addr2) {
    return (memcmp(addr1.data, addr2.data, MAC_ADDRESS_SIZE) == 0);
}

inline bool operator!=(const MacAddress& addr1, const MacAddress& addr2) {
    return !(addr1 == addr2);
}

} // particle
