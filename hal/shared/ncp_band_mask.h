/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <cstddef>
#include <cstdint>
#include <stdint.h>
#include <string.h>
#include "c_string.h"
#include "hex_to_bytes.h"
#include "endian_util.h"
#if PLATFORM_ID != PLATFORM_GCC
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "platform_ncp.h"
#include "ncp_env_var.h"
#endif

namespace particle {

inline void hexString128toUint64Array(const char* hexString128, uint64_t bands[2]) {
    uint8_t bytes[16] = {};
    size_t len = strlen(hexString128);

    // zero pad odd strings
    char normalized[32+1] = {};
    if (len & 1) {
        normalized[0] = '0';
        memcpy(normalized + 1, hexString128, len);
        hexString128 = normalized;
        len++;
    }

    size_t byteLen = len / 2;
    hexToBytes(hexString128, (char*)(bytes + sizeof(bytes) - byteLen), byteLen);

    memcpy(&bands[1], bytes, sizeof(uint64_t));
    memcpy(&bands[0], bytes + 8, sizeof(uint64_t));
    bands[0] = reverseByteOrder(bands[0]);
    bands[1] = reverseByteOrder(bands[1]);
}

const uint64_t UBLOX_NCP_BANDMASK_1_64_R510 = 0xB0E189F;      // Bands 1,2,3,4,5,8,12,13,18,19,20,25,26,28 [all default enabled]
const uint64_t UBLOX_NCP_BANDMASK_65_128_R510 = 0x100002;     // Band 66,85 enabled, 71 disabled
const uint64_t UBLOX_NCP_BANDMASK_1_64_R410 = 0x10E189E;      // Bands 2,3,4,5,8,12,13,18,19,20,25
const uint64_t UBLOX_NCP_BANDMASK_1_64_R410_LEGACY = 0x181A;  // Bands 2,4,5,12,13
const uint64_t UBLOX_NCP_BANDMASK_65_128_R410 = 0;            // Bands 65-128 are not supported on R410

// Bandmask same for BG95-M1 ~ BG95-M6 & BG95-MF
// Note: BG95-M1 does not support NB mode
// BG95-M4 not supported (bands are different as well)
const uint64_t QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95 = 0xF0E189F;      // Bands 1,2,3,4,5,8,12,13,18,19,20,25,26,27,28 [all default enabled]
const uint64_t QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95 = 0x100002;     // Band 66,85 enabled
// const uint64_t QUECTEL_NCP_BANDMASK_CATNB_1_64_BG95 = 0x90E189F;      // Bands 1,2,3,4,5,8,12,13,18,19,20,25,26,27,28 [all default enabled]
// const uint64_t QUECTEL_NCP_BANDMASK_CATNB_65_128_BG95 = 0x100002;     // Band 66,85 enabled

// BG95-S5
const uint64_t QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95_S5 = 0xF0E189F;   // Bands 1,2,3,4,5,8,12,13,18,19,20,25,26,27,28 [all default enabled]
const uint64_t QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95_S5 = 0x100002;  // Band 66,85 enabled
//const uint64_t QUECTEL_NCP_BANDMASK_NTN_1_64_BG95_S5 = 0x07; // NTN Band 23, 255, 256 enabled

// BG96-MC
const uint64_t QUECTEL_NCP_BANDMASK_CATM1_1_64_BG96_MC = 0x40090E189F;  // Bands 1,2,3,4,5,8,12,13,17(shows up in mask, but not listed in datasheet),
                                                                        //       18,19,20,25,26(v1.2 hardware),28,39

// EG91-E/EX (LTE Cat-1)
const uint64_t QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX = 0x80800C5;  // Bands 1,3,7,8,20,28
// const uint64_t QUECTEL_NCP_BANDMASK_CAT1_65_128_EG91_E_EX = 0x0;   // No bands in 65-128 range

// EG91-NA (LTE Cat-1)
const uint64_t QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NA = 0x1836;       // Bands 2,4,5,12,13
// EG91-NAX (LTE Cat-1)
const uint64_t QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NAX = 0x300181A;   // Bands 2,4,5,12,13,25,26
// const uint64_t QUECTEL_NCP_BANDMASK_CAT1_65_128_EG91_NA_NAX = 0x0; // No bands in 65-128 range

inline uint64_t getBandMaskByNcpIdForRAT(int ncpid, CellularAccessTechnology rat, bool upper = false, bool legacy = false) {
    if (rat == CellularAccessTechnology::LTE_CAT_M1) {
        switch ((PlatformNCPIdentifier)ncpid) {
            case PLATFORM_NCP_SARA_R510:
                return upper ? UBLOX_NCP_BANDMASK_65_128_R510 : UBLOX_NCP_BANDMASK_1_64_R510;
            case PLATFORM_NCP_SARA_R410:
                return upper ? UBLOX_NCP_BANDMASK_65_128_R410 : (legacy ? UBLOX_NCP_BANDMASK_1_64_R410_LEGACY : UBLOX_NCP_BANDMASK_1_64_R410);
            case PLATFORM_NCP_QUECTEL_BG95_M5:
                return upper ? QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95 : QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95;
            case PLATFORM_NCP_QUECTEL_BG95_S5:
                return upper ? QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95_S5 : QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95_S5;
            case PLATFORM_NCP_QUECTEL_BG96:
                return upper ? 0 : QUECTEL_NCP_BANDMASK_CATM1_1_64_BG96_MC;
            case PLATFORM_NCP_QUECTEL_EG91_E:
            case PLATFORM_NCP_QUECTEL_EG91_EX:
                return upper ? 0 : QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX;
            case PLATFORM_NCP_QUECTEL_EG91_NAX:
                return upper ? 0 : QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NAX;
            case PLATFORM_NCP_QUECTEL_EG91_NA:
                return upper ? 0 : QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NA;

            default:
                return 0;
        }
    }

    return 0;
}

class CellularBandMask {
public:
    CellularBandMask() {
        bands[0] = 0;
        bands[1] = 0;
    }

    explicit CellularBandMask(const char* hexStr) {
        bands[0] = 0;
        bands[1] = 0;
        if (hexStr) {
            hexString128toUint64Array(hexStr, bands);
        }
    }

    explicit CellularBandMask(uint64_t low, uint64_t high = 0) {
        bands[0] = low;
        bands[1] = high;
    }


    uint64_t low()  const { return bands[0]; }
    uint64_t high() const { return bands[1]; }

    void setLow(uint64_t val)  { bands[0] = val; }
    void setHigh(uint64_t val) { bands[1] = val; }

    void set(uint64_t low, uint64_t high = 0) {
        bands[0] = low;
        bands[1] = high;
    }

    void setFromHexString(const char* hexStr) {
        bands[0] = 0;
        bands[1] = 0;
        if (hexStr) {
            hexString128toUint64Array(hexStr, bands);
        }
    }

    void setFromDecimalStrings(const char* lowStr, const char* highStr = nullptr) {
        bands[0] = lowStr  ? strtoull(lowStr,  nullptr, 10) : 0;
        bands[1] = highStr ? strtoull(highStr, nullptr, 10) : 0;
    }

    CString toString() const {
        char buf[32 + 1];
        if (bands[1] != 0) {
            snprintf(buf, sizeof(buf), "%llx%016llx", bands[1], bands[0]);
        } else {
            snprintf(buf, sizeof(buf), "%llx", bands[0]);
        }
        return CString(buf);
    }

    bool operator==(const CellularBandMask& other) const {
        return bands[0] == other.bands[0] && bands[1] == other.bands[1];
    }

    bool operator!=(const CellularBandMask& other) const {
        return !(*this == other);
    }

    bool isEmpty() const {
        return bands[0] == 0 && bands[1] == 0;
    }

    CellularBandMask operator|(const CellularBandMask& other) const {
        return CellularBandMask(bands[0] | other.bands[0], bands[1] | other.bands[1]);
    }

    CellularBandMask operator&(const CellularBandMask& other) const {
        return CellularBandMask(bands[0] & other.bands[0], bands[1] & other.bands[1]);
    }

    CellularBandMask operator~() const {
        return CellularBandMask(~bands[0], ~bands[1]);
    }

    CellularBandMask& operator|=(const CellularBandMask& other) {
        bands[0] |= other.bands[0];
        bands[1] |= other.bands[1];
        return *this;
    }

    CellularBandMask& operator&=(const CellularBandMask& other) {
        bands[0] &= other.bands[0];
        bands[1] &= other.bands[1];
        return *this;
    }


private:
    uint64_t bands[2];
};

} // namespace particle
