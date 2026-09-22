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

#include "hal_platform.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.esim");

#include "esim_profiles.h"

#include "check.h"
#include "str_compat.h"
#include "scope_guard.h"
#include "system_error.h"

#include <cstdint>
#include <cstring>

// Can enable this at compile time:
//   GLOBAL_DEFINES=PARTICLE_ENABLE_SYSTEM_ESIM_LOGGING=1 make clean all -s PLATFORM=b5som APP=tinker-serial1-debugging program-dfu
#ifndef PARTICLE_ENABLE_SYSTEM_ESIM_LOGGING
#define PARTICLE_ENABLE_SYSTEM_ESIM_LOGGING (0)
#endif

#if PARTICLE_ENABLE_SYSTEM_ESIM_LOGGING
#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL
#endif

namespace particle {

namespace esim {

namespace {

const uint8_t ISD_R_AID[] = { 0xa0, 0x00, 0x00, 0x05, 0x59, 0x10, 0x10, 0xff, 0xff, 0xff, 0xff, 0x89,
        0x00, 0x00, 0x01, 0x00 };

// The tag list trims each profile down to an ICCID and a state, so a profile costs 18 bytes and
// this holds well over a dozen of them. Kept small because we run on the network interface thread,
// which has 4K of stack and is also carrying the AT command buffer while we're here. An overflow is
// an error, which means we carry on as usual rather than going idle.
const size_t MAX_RESPONSE_SIZE = 320;
const size_t MAX_COMMAND_SIZE = 32;

// Le of 0 asks for the maximum
const size_t MAX_GET_RESPONSE_SIZE = 256;

// SGP.22 5.7.15
const unsigned TAG_PROFILE_INFO_LIST_RESPONSE = 0xbf2d;
const unsigned TAG_PROFILE_INFO_LIST_OK = 0xa0;
const unsigned TAG_PROFILE_INFO = 0xe3;
const unsigned TAG_ICCID = 0x5a;
const unsigned TAG_PROFILE_STATE = 0x9f70;

const int PROFILE_STATE_UNKNOWN = -1;
const int PROFILE_STATE_ENABLED = 1;

// CLA for ISO/IEC 7816-4 commands on a logical channel
inline uint8_t isoCla(int channel) {
    return (channel <= 3) ? channel : (((channel - 4) & 0x0f) | 0x40);
}

// CLA for GlobalPlatform commands on a logical channel
inline uint8_t gpCla(int channel) {
    return (channel <= 3) ? (channel | 0x80) : (((channel - 4) & 0x0f) | 0xc0);
}

class Apdu {
public:
    Apdu(SendApduFn send, void* ctx) :
            send_(send),
            ctx_(ctx),
            sw_(0) {
    }

    // Sends one command and leaves the response data (without the status bytes) in resp, with its
    // size in respSize. The status word is available via sw().
    int sendCmd(const char* cmd, size_t cmdSize, char* resp, size_t& respSize) {
        size_t size = respSize;
        int r = send_(ctx_, cmd, cmdSize, resp, size);
        if (r < 0) {
            return r;
        }
        if (size < 2) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        sw_ = (((unsigned char)resp[size - 2]) << 8) | (unsigned char)resp[size - 1];
        respSize = size - 2;
        return 0;
    }

    unsigned sw() const {
        return sw_;
    }

    unsigned sw1() const {
        return sw_ >> 8;
    }

    unsigned sw2() const {
        return sw_ & 0xff;
    }

private:
    SendApduFn send_;
    void* ctx_;
    unsigned sw_;
};

// Minimal BER-TLV reader. Only handles what GetProfilesInfo actually returns.
class TlvReader {
public:
    TlvReader(const char* data, size_t size) :
            p_((const uint8_t*)data),
            end_((const uint8_t*)data + size) {
    }

    bool hasMore() const {
        return p_ < end_;
    }

    // Reads the next tag and length, leaving the value at value()/valueSize().
    int next() {
        if (end_ - p_ < 2) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        tag_ = *p_++;
        if ((tag_ & 0x1f) == 0x1f) {
            // Two byte tag
            tag_ = (tag_ << 8) | *p_++;
        }
        if (p_ >= end_) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        size_t len = *p_++;
        if (len & 0x80) {
            const size_t lenBytes = len & 0x7f;
            if (lenBytes == 0 || lenBytes > 2 || (size_t)(end_ - p_) < lenBytes) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            len = 0;
            for (size_t i = 0; i < lenBytes; ++i) {
                len = (len << 8) | *p_++;
            }
        }
        if ((size_t)(end_ - p_) < len) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        value_ = p_;
        valueSize_ = len;
        p_ += len;
        return 0;
    }

    unsigned tag() const {
        return tag_;
    }

    const char* value() const {
        return (const char*)value_;
    }

    size_t valueSize() const {
        return valueSize_;
    }

private:
    const uint8_t* p_;
    const uint8_t* end_;
    const uint8_t* value_ = nullptr;
    size_t valueSize_ = 0;
    unsigned tag_ = 0;
};

int openChannel(Apdu& apdu, char* resp, size_t respSize, int* channel) {
    const char cmd[] = { 0x00, 0x70, 0x00, 0x00, 0x01 }; // MANAGE CHANNEL, open
    size_t size = respSize;
    CHECK(apdu.sendCmd(cmd, sizeof(cmd), resp, size));
    if (apdu.sw() != 0x9000 || size != 1) {
        LOG_DEBUG(TRACE, "MANAGE CHANNEL open returned 0x%04x", apdu.sw());
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // The status said the card assigned us a channel, so report it even if the number then looks
    // wrong. Otherwise the caller has nothing to close and it stays open until the modem resets.
    const int ch = (unsigned char)resp[0];
    *channel = ch;
    CHECK_TRUE(ch >= 1 && ch <= 19, SYSTEM_ERROR_BAD_DATA);
    return 0;
}

int closeChannel(Apdu& apdu, int channel, char* resp, size_t respSize) {
    const char cmd[] = { 0x00, 0x70, 0x80, (char)channel, 0x00 }; // MANAGE CHANNEL, close
    size_t size = respSize;
    return apdu.sendCmd(cmd, sizeof(cmd), resp, size);
}

int selectIsdR(Apdu& apdu, int channel, char* resp, size_t respSize) {
    char cmd[MAX_COMMAND_SIZE];
    cmd[0] = isoCla(channel);
    cmd[1] = 0xa4; // SELECT
    cmd[2] = 0x04; // P1, select by DF name
    cmd[3] = 0x04; // P2, return no FCI
    cmd[4] = sizeof(ISD_R_AID);
    memcpy(cmd + 5, ISD_R_AID, sizeof(ISD_R_AID));
    size_t size = respSize;
    CHECK(apdu.sendCmd(cmd, 5 + sizeof(ISD_R_AID), resp, size));
    // A part with no eUICC lands here, so this is an expected failure rather than a fault
    if (apdu.sw1() != 0x90 && apdu.sw1() != 0x61) {
        LOG_DEBUG(TRACE, "SELECT ISD-R returned 0x%04x", apdu.sw());
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

// Runs ES10c GetProfilesInfo asking only for the ICCID and the profile state, and accumulates the
// whole response into resp.
int getProfilesInfo(Apdu& apdu, int channel, char* resp, size_t respSize, size_t* respLen) {
    // STORE DATA, single block, response expected. Data is a GetProfilesInfoRequest carrying a tag
    // list that keeps the reply down to the two fields we look at.
    const char cmd[] = { (char)gpCla(channel), (char)0xe2, (char)0x91, 0x00, 0x08,
            (char)0xbf, 0x2d, 0x05, 0x5c, 0x03, 0x5a, (char)0x9f, 0x70 };
    size_t used = respSize;
    CHECK(apdu.sendCmd(cmd, sizeof(cmd), resp, used));
    if (apdu.sw1() != 0x90 && apdu.sw1() != 0x61) {
        LOG_DEBUG(TRACE, "GetProfilesInfo returned 0x%04x", apdu.sw());
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    // The eUICC hands the response back in chunks, 0x61 XX meaning XX more bytes are waiting
    while (apdu.sw1() == 0x61) {
        // Check before asking rather than after. sendApdu() truncates a response that does not fit
        // and reports the short length, which would look like a valid but corrupt reply.
        const size_t want = apdu.sw2() ? apdu.sw2() : MAX_GET_RESPONSE_SIZE;
        if (respSize - used < want + 2) {
            LOG(WARN, "eSIM profile response does not fit in %u bytes", (unsigned)respSize);
            return SYSTEM_ERROR_TOO_LARGE;
        }
        const char get[] = { (char)gpCla(channel), (char)0xc0, 0x00, 0x00, (char)apdu.sw2() };
        size_t size = respSize - used;
        CHECK(apdu.sendCmd(get, sizeof(get), resp + used, size));
        if (apdu.sw1() != 0x90 && apdu.sw1() != 0x61) {
            LOG_DEBUG(TRACE, "GET RESPONSE returned 0x%04x", apdu.sw());
            return SYSTEM_ERROR_BAD_DATA;
        }
        used += size;
    }

    *respLen = used;
    return 0;
}

int countEnabledProfiles(const char* data, size_t size, int* total, char* enabledIccid,
        size_t enabledIccidSize) {
    TlvReader outer(data, size);
    CHECK_TRUE(outer.hasMore(), SYSTEM_ERROR_BAD_DATA);
    CHECK(outer.next());
    CHECK_TRUE(outer.tag() == TAG_PROFILE_INFO_LIST_RESPONSE, SYSTEM_ERROR_BAD_DATA);

    TlvReader body(outer.value(), outer.valueSize());
    CHECK_TRUE(body.hasMore(), SYSTEM_ERROR_BAD_DATA);
    CHECK(body.next());
    if (body.tag() != TAG_PROFILE_INFO_LIST_OK) {
        // The eUICC answered with profileInfoListError
        LOG_DEBUG(TRACE, "GetProfilesInfo error, tag 0x%02x", body.tag());
        return SYSTEM_ERROR_BAD_DATA;
    }

    int enabled = 0;
    int count = 0;
    TlvReader list(body.value(), body.valueSize());
    while (list.hasMore()) {
        CHECK(list.next());
        if (list.tag() != TAG_PROFILE_INFO) {
            continue;
        }
        ++count;
        char iccid[21] = {};
        int state = -1;
        TlvReader profile(list.value(), list.valueSize());
        while (profile.hasMore()) {
            CHECK(profile.next());
            if (profile.tag() == TAG_PROFILE_STATE && profile.valueSize() == 1) {
                state = (unsigned char)profile.value()[0];
            } else if (profile.tag() == TAG_ICCID) {
                // Nibble swapped BCD, with an F pad on an odd length ICCID
                size_t n = 0;
                for (size_t i = 0; i < profile.valueSize() && n + 2 < sizeof(iccid); ++i) {
                    const uint8_t b = profile.value()[i];
                    iccid[n++] = '0' + (b & 0x0f);
                    if ((b >> 4) != 0x0f) {
                        iccid[n++] = '0' + (b >> 4);
                    }
                }
                iccid[n] = '\0';
            }
        }
        // profileState is optional in SGP.22, so an eUICC is allowed to leave it out. We cannot
        // call that disabled: treating it as such would idle a device that may well have a working
        // profile, which is the one thing we must never do on a doubtful answer.
        if (state == PROFILE_STATE_UNKNOWN) {
            LOG(WARN, "eSIM profile %s has no state, not trusting the profile list", iccid);
            return SYSTEM_ERROR_NOT_ENOUGH_DATA;
        }
        if (state == PROFILE_STATE_ENABLED) {
            ++enabled;
            if (enabledIccid && enabledIccidSize > 0) {
                strlcpy(enabledIccid, iccid, enabledIccidSize);
            }
        }
        LOG(INFO, "eSIM profile %s: %s", iccid,
                state == PROFILE_STATE_ENABLED ? "enabled" : "disabled");
    }

    *total = count;
    return enabled;
}

} // namespace

int hasEnabledProfile(SendApduFn send, void* ctx, char* iccid, size_t iccidSize) {
    CHECK_TRUE(send, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (iccid && iccidSize > 0) {
        iccid[0] = '\0';
    }

    char resp[MAX_RESPONSE_SIZE];
    Apdu apdu(send, ctx);

    // Armed before the open so that any way out of here gives the channel back, including the
    // failure inside openChannel() where the card assigned one but we did not like the number
    int channel = 0;
    SCOPE_GUARD({
        if (channel > 0) {
            const int rc = closeChannel(apdu, channel, resp, sizeof(resp));
            if (rc < 0) {
                LOG(WARN, "Failed to close APDU channel %d: %d", channel, rc);
            }
        }
    });

    CHECK(openChannel(apdu, resp, sizeof(resp), &channel));
    CHECK(selectIsdR(apdu, channel, resp, sizeof(resp)));
    size_t respLen = 0;
    CHECK(getProfilesInfo(apdu, channel, resp, sizeof(resp), &respLen));

    int total = 0;
    const int enabled = CHECK(countEnabledProfiles(resp, respLen, &total, iccid, iccidSize));
    LOG_DEBUG(INFO, "eSIM has %d profile(s), %d enabled", total, enabled);
    return enabled > 0 ? 1 : 0;
}

} // esim

} // particle

