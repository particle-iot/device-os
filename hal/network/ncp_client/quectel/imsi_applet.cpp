/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.client");

#include "imsi_applet.h"

#include "check.h"
#include "system_error.h"

#include <cstdint>
#include <initializer_list>

namespace particle {

namespace kore {

namespace {

// Each applet ignores the other's status query (9000, no data), which is how we tell them apart
// 80C2000008CF06020282814C00
const uint8_t STATUS_CURRENT[] = { 0x80, 0xc2, 0x00, 0x00, 0x08, 0xcf, 0x06, 0x02, 0x02, 0x82, 0x81, 0x4c,
        0x00 };
// 80C2000009CF0702028281B30100
const uint8_t STATUS_NEW[] = { 0x80, 0xc2, 0x00, 0x00, 0x09, 0xcf, 0x07, 0x02, 0x02, 0x82, 0x81, 0xb3, 0x01,
        0x00 };
// 80C2000009CF07020282814E0101
const uint8_t FORCE_CURRENT[] = { 0x80, 0xc2, 0x00, 0x00, 0x09, 0xcf, 0x07, 0x02, 0x02, 0x82, 0x81, 0x4e,
        0x01, 0x01 };
// 80C2000009CF0702028281A001AA
const uint8_t FORCE_NEW[] = { 0x80, 0xc2, 0x00, 0x00, 0x09, 0xcf, 0x07, 0x02, 0x02, 0x82, 0x81, 0xa0, 0x01,
        0xaa };
// 80C200000DCF0B020282814F050190000000
const uint8_t AUTOMATIC_CURRENT[] = { 0x80, 0xc2, 0x00, 0x00, 0x0d, 0xcf, 0x0b, 0x02, 0x02, 0x82, 0x81, 0x4f,
        0x05, 0x01, 0x90, 0x00, 0x00, 0x00 };
// 80C2000009CF0702028281A001EE
const uint8_t AUTOMATIC_NEW[] = { 0x80, 0xc2, 0x00, 0x00, 0x09, 0xcf, 0x07, 0x02, 0x02, 0x82, 0x81, 0xa0, 0x01,
        0xee };

// Status is 9 bytes, plus the two status bytes
const size_t RESPONSE_SIZE = 32;

} // namespace

int readImsiAppletStatus(SendApduFn send, void* ctx, ImsiApplet& applet, bool& automatic) {
    CHECK_TRUE(send, SYSTEM_ERROR_INVALID_ARGUMENT);
    applet = ImsiApplet::NONE;
    automatic = false;
    Apdu apdu(send, ctx);
    for (auto candidate: { ImsiApplet::CURRENT, ImsiApplet::NEW }) {
        const bool current = candidate == ImsiApplet::CURRENT;
        const auto cmd = current ? STATUS_CURRENT : STATUS_NEW;
        const size_t cmdSize = current ? sizeof(STATUS_CURRENT) : sizeof(STATUS_NEW);
        char resp[RESPONSE_SIZE];
        size_t size = sizeof(resp);
        CHECK(apdu.sendCmd((const char*)cmd, cmdSize, resp, size));
        if (apdu.sw1() == 0x61) {
            // The status is waiting to be fetched, some modems do this for us and some do not
            size = sizeof(resp);
            CHECK(apdu.getResponse(0x00, resp, size));
        }
        if (apdu.sw() != 0x9000 || size < 1) {
            continue;
        }
        const uint8_t mode = resp[0];
        // The current applet always sets bit 7
        if (current && !(mode & 0x80)) {
            continue;
        }
        applet = candidate;
        // Current: bit 0 set is Default IMSI mode, it goes automatic on its own when its timer fires.
        // New: 00 is switching disabled, e.g. a locked IMSI.
        automatic = current ? !(mode & 0x01) : (mode == 0x01);
        return 0;
    }
    return 0;
}

int forceImsiRotation(SendApduFn send, void* ctx, ImsiApplet applet) {
    CHECK_TRUE(send && applet != ImsiApplet::NONE, SYSTEM_ERROR_INVALID_ARGUMENT);
    const bool current = applet == ImsiApplet::CURRENT;
    const auto cmd = current ? FORCE_CURRENT : FORCE_NEW;
    const size_t cmdSize = current ? sizeof(FORCE_CURRENT) : sizeof(FORCE_NEW);
    Apdu apdu(send, ctx);
    char resp[RESPONSE_SIZE];
    size_t size = sizeof(resp);
    CHECK(apdu.sendCmd((const char*)cmd, cmdSize, resp, size));
    if (current) {
        // SW1 0x91, the applet has a proactive command waiting, the REFRESH that rotates.
        // 0x9000 is the applet answering without doing anything.
        if (apdu.sw1() == 0x91) {
            return 1;
        }
    } else if (apdu.sw() == 0x9000 || apdu.sw1() == 0x61 || apdu.sw1() == 0x91) {
        // Documented as 9000 or 6109, neither says whether it switched. The IMSI check will tell.
        return 1;
    }
    LOG(INFO, "IMSI rotation not started (%04X)", apdu.sw());
    return 0;
}

int setImsiAppletAutomatic(SendApduFn send, void* ctx, ImsiApplet applet) {
    CHECK_TRUE(send && applet != ImsiApplet::NONE, SYSTEM_ERROR_INVALID_ARGUMENT);
    const bool current = applet == ImsiApplet::CURRENT;
    const auto cmd = current ? AUTOMATIC_CURRENT : AUTOMATIC_NEW;
    const size_t cmdSize = current ? sizeof(AUTOMATIC_CURRENT) : sizeof(AUTOMATIC_NEW);
    Apdu apdu(send, ctx);
    char resp[RESPONSE_SIZE];
    size_t size = sizeof(resp);
    CHECK(apdu.sendCmd((const char*)cmd, cmdSize, resp, size));
    // Documented as 9000, and 6109 as well for the new applet
    if (apdu.sw() == 0x9000 || (!current && apdu.sw1() == 0x61)) {
        return 0;
    }
    LOG(WARN, "IMSI applet did not go automatic (%04X)", apdu.sw());
    return SYSTEM_ERROR_INVALID_STATE;
}

} // kore

} // particle
