/**
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

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

#pragma once

static inline char ascii_nibble(uint8_t nibble) {
    char hex_digit = nibble + 48;
    if (57 < hex_digit) {
        hex_digit += 7;
    }
    return hex_digit;
}

static inline char ascii_nibble_lower_case(uint8_t nibble) {
    char hex_digit = nibble + 48;
    if (57 < hex_digit) {
        hex_digit += 39;
    }
    return hex_digit;
}

static inline char* concat_nibble(char* p, uint8_t nibble)
{
    *p++ = ascii_nibble(nibble);
    return p;
}

static inline char* concat_nibble_lower_case(char* p, uint8_t nibble)
{
    *p++ = ascii_nibble_lower_case(nibble);
    return p;
}

static inline char* bytes2hexbuf(const uint8_t* buf, unsigned len, char* out)
{
    unsigned i;
    char* result = out;
    for (i = 0; i < len; ++i)
    {
        out = concat_nibble(out, (buf[i] >> 4));
        out = concat_nibble(out, (buf[i] & 0xF));
    }
    return result;
}

static inline char* bytes2hexbuf_lower_case(const uint8_t* buf, unsigned len, char* out)
{
    unsigned i;
    char* result = out;
    for (i = 0; i < len; ++i)
    {
        out = concat_nibble_lower_case(out, (buf[i] >> 4));
        out = concat_nibble_lower_case(out, (buf[i] & 0xF));
    }
    return result;
}
