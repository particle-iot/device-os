/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include <cstdint>
#include "system_tick_hal.h"
#include "random.h"

namespace particle {
namespace ntp {

const char DEFAULT_SERVER[] = "pool.ntp.org";
const uint16_t PORT = 123;
const system_tick_t RETRY_INTERVAL = 2000;

const uint8_t LI_MASK = 0x03 << 6;
const uint8_t VERSION_MASK = 0x07 << 3;
const uint8_t MODE_MASK = 0x07;
const uint8_t VERSION = 4 << 3;

enum Li {
    LI_NO_WARNING = 0 << 6,
    LI_LAST_MINUTE_61 = 1 << 6,
    LI_LAST_MINUTE_59 = 2 << 6,
    LI_ALARM = 3 << 6
};

enum Mode {
    MODE_RESERVED = 0,
    MODE_SYMMETRIC_ACTIVE = 1,
    MODE_SYMMETRIC_PASSIVE = 2,
    MODE_CLIENT = 3,
    MODE_SERVER = 4,
    MODE_BROADCAST = 5,
    MODE_RESERVED_CONTROL_MESSAGE = 6,
    MODE_RESERVED_PRIVATE = 7,
};

enum Stratum {
    STRATUM_KOD = 0
};

const uint32_t SECONDS_FROM_1970_TO_1900 = 2208988800UL;
const uint32_t USEC_IN_SEC = 1000000;
const uint32_t FRACTIONS_IN_SEC_POW = 32;

// FIXME
#if !HAL_USE_SOCKET_HAL_POSIX
inline uint16_t htons(uint16_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return val;
#else
    return __builtin_bswap16(val);
#endif
}

inline uint16_t ntohs(uint16_t val) {
    return htons(val);
}

inline uint32_t htonl(uint32_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return val;
#else
    return __builtin_bswap32(val);
#endif
}

inline uint32_t ntohl(uint32_t val) {
    return htonl(val);
}
#endif // !HAL_USE_SOCKET_HAL_POSIX


struct Timestamp {
    uint32_t seconds;
    uint32_t fraction;

    static Timestamp fromUnixtime(uint64_t unixMicros) {
        uint32_t seconds = unixMicros / USEC_IN_SEC;
        uint32_t frac = ((unixMicros - seconds) << FRACTIONS_IN_SEC_POW) / USEC_IN_SEC;
        particle::Random rand;
        rand.gen((char*)&frac, sizeof(char));
        return {htonl(seconds + SECONDS_FROM_1970_TO_1900), htonl(frac)};
    }

    uint64_t toUnixtime() const {
        uint64_t unixMicros = (uint64_t)(ntohl(seconds) - SECONDS_FROM_1970_TO_1900) * USEC_IN_SEC;
        uint64_t frac = (((uint64_t)ntohl(fraction)) * USEC_IN_SEC) >> FRACTIONS_IN_SEC_POW;
        return unixMicros + frac;
    }
} __attribute__((packed));

struct Message {
    uint8_t liVnMode; // LI + VN + mode
    uint8_t stratum; // stratum
    uint8_t poll; // poll exponent
    uint8_t precision; // precision exponent
    uint32_t rootDelay; // rootdelay
    uint32_t rootDisp; // rootdisp
    uint32_t refId; // refid
    Timestamp refTime; // reftime
    Timestamp originTimestamp; // org
    Timestamp receiveTimestamp; // rec
    Timestamp destinationTimestamp; // dst
    // uint32_t keyId; // key identifier
    // uint8_t digest[16]; // message digest
} __attribute__((packed));

} // ntp

} // particle
