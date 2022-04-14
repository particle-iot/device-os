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

#include "addr_util.h"

#include "hex_to_bytes.h"
#include "bytes2hexbuf.h"

namespace particle {

bool macAddressToString(const MacAddress& addr, char* str, size_t size) {
    if (size < MAC_ADDRESS_STRING_SIZE) {
        return false;
    }
    for (size_t i = 0; i < MAC_ADDRESS_SIZE; ++i) {
        bytes2hexbuf_lower_case(addr.data + i, 1, str);
        str += 2;
        if (i != MAC_ADDRESS_SIZE - 1) {
            *str++ = ':';
        }
    }
    if (size > MAC_ADDRESS_STRING_SIZE) {
        *str = '\0';
    }
    return true;
}

bool macAddressFromString(MacAddress* addr, const char* str, size_t size) {
    if (size < MAC_ADDRESS_STRING_SIZE) {
        return false;
    }
    for (size_t i = 0; i < MAC_ADDRESS_SIZE; ++i) {
        const size_t n = hexToBytes(str, (char*)addr->data + i, 1);
        if (n != 1) {
            return false;
        }
        str += 2;
        if (i != MAC_ADDRESS_SIZE - 1 && *str++ != ':') {
            return false;
        }
    }
    return true;
}

} // particle
