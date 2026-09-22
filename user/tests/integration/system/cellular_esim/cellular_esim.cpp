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

// cellular_is_idle() is not a stable API, it exists so tests can see the IDLE state
#define PARTICLE_USE_UNSTABLE_API

#include "application.h"
#include "unit-test/unit-test.h"
#include "test_suite.h"
#include "cellular_hal.h"
#include "system_network.h"

#include <algorithm>
#include <cctype>

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

namespace {

// The ICCID of the profile that was enabled when the suite started. Retained so a reset in the
// middle of the run still knows what to put back.
retained char g_savedIccid[21] = {};
retained bool g_savedIccidValid = false;

// Retained, the runner resets the device between tests so a plain bool would come back false and
// the skipped tests would start asserting instead
retained bool g_skip = false;

const uint8_t ISD_R_AID[] = { 0xa0, 0x00, 0x00, 0x05, 0x59, 0x10, 0x10, 0xff, 0xff, 0xff, 0xff, 0x89,
        0x00, 0x00, 0x01, 0x00 };

// ES10c commands we drive by hand. The HAL passes APDUs straight through, so the test owns the
// channel, the ISD-R select and the GET RESPONSE chaining, same as the tooling does.
const uint8_t INS_STORE_DATA = 0xe2;
const uint8_t INS_GET_RESPONSE = 0xc0;

// A card can hand out any channel from 1 to 19, and past 3 the number stops fitting in the low
// bits of the class byte
inline uint8_t isoCla(int channel) {
    return (channel <= 3) ? channel : (((channel - 4) & 0x0f) | 0x40);
}

inline uint8_t gpCla(int channel) {
    return (channel <= 3) ? (channel | 0x80) : (((channel - 4) & 0x0f) | 0xc0);
}

int sendApdu(const uint8_t* cmd, size_t cmdSize, uint8_t* resp, size_t* respSize) {
    size_t size = *respSize;
    const int r = cellular_send_apdu((const char*)cmd, cmdSize, (char*)resp, &size, nullptr);
    if (r < 0) {
        return r;
    }
    *respSize = size;
    return 0;
}

// Status word of the last response, the HAL hands back the trailing SW1 SW2
int statusWord(const uint8_t* resp, size_t respSize) {
    if (respSize < 2) {
        return -1;
    }
    return (resp[respSize - 2] << 8) | resp[respSize - 1];
}

int openChannel(int* channel) {
    const uint8_t cmd[] = { 0x00, 0x70, 0x00, 0x00, 0x01 };
    uint8_t resp[8] = {};
    size_t respSize = sizeof(resp);
    CHECK(sendApdu(cmd, sizeof(cmd), resp, &respSize));
    if (statusWord(resp, respSize) != 0x9000 || respSize != 3) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // The card says it assigned us a channel, so hand the number back even when it then looks
    // wrong. Otherwise the caller has nothing to close and it stays open until the modem resets
    *channel = resp[0];
    if (resp[0] < 1 || resp[0] > 19) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    return 0;
}

int closeChannel(int channel) {
    const uint8_t cmd[] = { 0x00, 0x70, 0x80, (uint8_t)channel, 0x00 };
    uint8_t resp[8] = {};
    size_t respSize = sizeof(resp);
    CHECK(sendApdu(cmd, sizeof(cmd), resp, &respSize));
    return statusWord(resp, respSize) == 0x9000 ? 0 : SYSTEM_ERROR_UNKNOWN;
}

int selectIsdR(int channel) {
    uint8_t cmd[6 + sizeof(ISD_R_AID)] = {};
    cmd[0] = isoCla(channel);
    cmd[1] = 0xa4;
    cmd[2] = 0x04;
    cmd[3] = 0x00;
    cmd[4] = sizeof(ISD_R_AID);
    memcpy(cmd + 5, ISD_R_AID, sizeof(ISD_R_AID));
    uint8_t resp[8] = {};
    size_t respSize = sizeof(resp);
    CHECK(sendApdu(cmd, sizeof(cmd), resp, &respSize));
    const int sw = statusWord(resp, respSize);
    // 61xx means the response is waiting, which is fine, we do not need the FCI
    return (sw >> 8) == 0x61 || sw == 0x9000 ? 0 : SYSTEM_ERROR_NOT_SUPPORTED;
}

// One logical channel held open across a run of ES10c work. Opening and closing per operation
// costs an extra APDU pair, and worse, the NCP client runs AT+CFUN? at the start of every block:
// hitting the card right after a close is what leaves it silent until the command times out.
class ApduSession {
public:
    ~ApduSession() {
        close();
    }

    // Negative on failure, otherwise the channel number to put in the class byte
    int channel() {
        if (channel_ > 0) {
            return channel_;
        }
        int c = 0;
        int r = openChannel(&c);
        if (r >= 0) {
            r = selectIsdR(c);
        }
        if (r < 0) {
            if (c > 0) {
                closeChannel(c);
            }
            return r;
        }
        channel_ = c;
        return channel_;
    }

    void close() {
        if (channel_ > 0) {
            closeChannel(channel_);
            channel_ = 0;
        }
    }

private:
    int channel_ = 0;
};

// Reused by every operation below. The card tears the channel down on its own if it resets, and
// the HAL closes it after five idle minutes, so the retry below drops this and reopens.
ApduSession g_apdu;

// Sends a STORE DATA carrying one ES10c request and collects the response through GET RESPONSE
int storeData(int channel, const uint8_t* req, size_t reqSize, uint8_t* resp, size_t* respSize) {
    uint8_t cmd[5 + 64] = {};
    if (reqSize > sizeof(cmd) - 5) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    cmd[0] = gpCla(channel);
    cmd[1] = INS_STORE_DATA;
    cmd[2] = 0x91;
    cmd[3] = 0x00;
    cmd[4] = (uint8_t)reqSize;
    memcpy(cmd + 5, req, reqSize);

    uint8_t sw[8] = {};
    size_t swSize = sizeof(sw);
    CHECK(sendApdu(cmd, 5 + reqSize, sw, &swSize));
    const int status = statusWord(sw, swSize);
    if ((status >> 8) != 0x61) {
        return SYSTEM_ERROR_BAD_DATA;
    }

    const uint8_t get[] = { gpCla(channel), INS_GET_RESPONSE, 0x00, 0x00,
            (uint8_t)(status & 0xff) };
    CHECK(sendApdu(get, sizeof(get), resp, respSize));
    return statusWord(resp, *respSize) == 0x9000 ? 0 : SYSTEM_ERROR_BAD_DATA;
}

// ICCIDs travel as nibble swapped BCD on the card but read normally over AT+CCID. E.118 allows
// 19 or 20 digits and the TLV is always 10 bytes, so a 19 digit one carries an F in the last
// nibble, which is a pad rather than a digit
void iccidToBcd(const char* iccid, uint8_t* bcd) {
    const size_t len = strlen(iccid);
    for (size_t i = 0; i < 10; i++) {
        const uint8_t lo = (i * 2 < len) ? (iccid[i * 2] - '0') : 0x0f;
        const uint8_t hi = (i * 2 + 1 < len) ? (iccid[i * 2 + 1] - '0') : 0x0f;
        bcd[i] = (uint8_t)((hi << 4) | lo);
    }
}

void bcdToIccid(const uint8_t* bcd, size_t bcdSize, char* iccid, size_t iccidSize) {
    size_t n = 0;
    for (size_t i = 0; i < bcdSize && n + 2 < iccidSize; i++) {
        iccid[n++] = (char)('0' + (bcd[i] & 0x0f));
        if ((bcd[i] >> 4) != 0x0f) {
            iccid[n++] = (char)('0' + (bcd[i] >> 4));
        }
    }
    iccid[n] = '\0';
}

// Minimal BER-TLV walk. Enough for the fixed ES10c shapes here, and unlike scanning for a tag
// byte it cannot trip over an 0xe3 that happens to sit inside a value.
class Tlv {
public:
    Tlv(const uint8_t* data, size_t size) : p_(data), end_(data + size) {}

    bool next() {
        if (p_ >= end_) {
            return false;
        }
        tag_ = *p_++;
        if ((tag_ & 0x1f) == 0x1f) { // Multi byte tag, 0x9f70 and 0xbf2d land here
            if (p_ >= end_) {
                return false;
            }
            tag_ = (tag_ << 8) | *p_++;
        }
        if (p_ >= end_) {
            return false;
        }
        size_t len = *p_++;
        if (len & 0x80) {
            const size_t n = len & 0x7f;
            if (n < 1 || n > 2 || p_ + n > end_) {
                return false;
            }
            len = 0;
            for (size_t i = 0; i < n; ++i) {
                len = (len << 8) | *p_++;
            }
        }
        if (p_ + len > end_) {
            return false;
        }
        val_ = p_;
        valSize_ = len;
        p_ += len;
        return true;
    }

    unsigned tag() const { return tag_; }
    const uint8_t* value() const { return val_; }
    size_t valueSize() const { return valSize_; }

private:
    const uint8_t* p_;
    const uint8_t* end_;
    unsigned tag_ = 0;
    const uint8_t* val_ = nullptr;
    size_t valSize_ = 0;
};

// Pin a specific profile when a card carries more than one usable operational profile:
//   GLOBAL_DEFINES=PARTICLE_ESIM_TEST_ICCID='\"89883070000051458816\"'
#ifndef PARTICLE_ESIM_TEST_ICCID
#define PARTICLE_ESIM_TEST_ICCID ""
#endif

// An m635 msom carries two operational profiles, a terrestrial LTE one and an NTN one, and only
// the first can satisfy a registration test. SGP.22 gives ProfileInfo no radio technology field,
// so the operator name is the only signal, and only in the positive direction: the terrestrial
// profile reports Twilio and the NTN one never does, while what the NTN one *is* called varies.
// Override either of these when a card does not fit that:
//   GLOBAL_DEFINES=PARTICLE_ESIM_TEST_OPERATOR='\"Twilio\"'
//   GLOBAL_DEFINES=PARTICLE_ESIM_TEST_ICCID='\"89883070000051458816\"'
#ifndef PARTICLE_ESIM_TEST_OPERATOR
#define PARTICLE_ESIM_TEST_OPERATOR "Twilio"
#endif

#ifndef PARTICLE_ESIM_TEST_ICCID
#define PARTICLE_ESIM_TEST_ICCID ""
#endif

const int MAX_PROFILES = 8;

bool containsIgnoreCase(const char* haystack, const char* needle) {
    const size_t n = strlen(needle);
    if (!n) {
        return false;
    }
    for (const char* p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < n && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) {
            ++i;
        }
        if (i == n) {
            return true;
        }
    }
    return false;
}

struct OperationalProfile {
    char iccid[21];
    // serviceProviderName or profileName matched PARTICLE_ESIM_TEST_OPERATOR
    bool expectedOperator;
};

struct ProfileInfo {
    int total = 0;
    int enabled = 0;
    char enabledIccid[21] = {};
    // Operational profiles only. Test and provisioning profiles are not ours to enable.
    OperationalProfile operational[MAX_PROFILES] = {};
    int operationalCount = 0;

    const OperationalProfile* find(const char* iccid) const {
        for (int i = 0; i < operationalCount; ++i) {
            if (!strcmp(operational[i].iccid, iccid)) {
                return &operational[i];
            }
        }
        return nullptr;
    }
};

// Never guesses between operational profiles. In priority order: an ICCID pinned at build time,
// whatever is already enabled, the profile this device used last, a name match on the expected
// terrestrial operator, and finally a card that only offers one. A card that is still ambiguous
// after all that is reported rather than picked from, because choosing the NTN profile would fail
// the registration tests in a way that looks like a Device OS bug.
int pickProfile(const ProfileInfo& info, char* out, size_t outSize) {
    const char* pinned = PARTICLE_ESIM_TEST_ICCID;
    if (strlen(pinned)) {
        if (!info.find(pinned)) {
            Log.error("Pinned ICCID %s is not an operational profile on this card", pinned);
            return SYSTEM_ERROR_NOT_FOUND;
        }
        Log.info("Using pinned profile %s", pinned);
        strlcpy(out, pinned, outSize);
        return 0;
    }
    if (info.enabled == 1 && info.find(info.enabledIccid)) {
        Log.info("Using the already enabled profile %s", info.enabledIccid);
        strlcpy(out, info.enabledIccid, outSize);
        return 0;
    }
    if (g_savedIccidValid && info.find(g_savedIccid)) {
        Log.info("Reusing the profile this device ran with before, %s", g_savedIccid);
        strlcpy(out, g_savedIccid, outSize);
        return 0;
    }
    int matches = 0;
    const OperationalProfile* match = nullptr;
    for (int i = 0; i < info.operationalCount; ++i) {
        if (info.operational[i].expectedOperator) {
            ++matches;
            match = &info.operational[i];
        }
    }
    if (matches == 1) {
        Log.info("Using %s, the only %s profile", match->iccid, PARTICLE_ESIM_TEST_OPERATOR);
        strlcpy(out, match->iccid, outSize);
        return 0;
    }
    if (info.operationalCount == 1) {
        Log.info("Using %s, the only operational profile", info.operational[0].iccid);
        strlcpy(out, info.operational[0].iccid, outSize);
        return 0;
    }
    Log.error("Cannot choose between %d operational profiles (%d matching %s). Pin one with "
            "PARTICLE_ESIM_TEST_ICCID.", info.operationalCount, matches,
            PARTICLE_ESIM_TEST_OPERATOR);
    return SYSTEM_ERROR_NOT_FOUND;
}

const unsigned TAG_PROFILE_INFO_LIST_RESPONSE = 0xbf2d;
const unsigned TAG_PROFILE_INFO_LIST_OK = 0xa0;
const unsigned TAG_PROFILE_INFO = 0xe3;
const unsigned TAG_ICCID = 0x5a;
const unsigned TAG_PROFILE_STATE = 0x9f70;
const unsigned TAG_PROFILE_CLASS = 0x95;
const unsigned TAG_SPN = 0x91;
const unsigned TAG_PROFILE_NAME = 0x92;

const int PROFILE_CLASS_OPERATIONAL = 2;

int parseProfiles(const uint8_t* data, size_t size, ProfileInfo* info) {
    Tlv outer(data, size);
    if (!outer.next() || outer.tag() != TAG_PROFILE_INFO_LIST_RESPONSE) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    Tlv body(outer.value(), outer.valueSize());
    if (!body.next() || body.tag() != TAG_PROFILE_INFO_LIST_OK) {
        // profileInfoListError
        return SYSTEM_ERROR_BAD_DATA;
    }
    Tlv list(body.value(), body.valueSize());
    while (list.next()) {
        if (list.tag() != TAG_PROFILE_INFO) {
            continue;
        }
        ++info->total;
        char iccid[21] = {};
        char spn[33] = {};
        char name[33] = {};
        bool haveIccid = false;
        int state = -1;
        int profileClass = -1;
        Tlv profile(list.value(), list.valueSize());
        while (profile.next()) {
            if (profile.tag() == TAG_ICCID && profile.valueSize() == 10) {
                bcdToIccid(profile.value(), profile.valueSize(), iccid, sizeof(iccid));
                haveIccid = true;
            } else if (profile.tag() == TAG_PROFILE_STATE && profile.valueSize() == 1) {
                state = profile.value()[0];
            } else if (profile.tag() == TAG_PROFILE_CLASS && profile.valueSize() == 1) {
                profileClass = profile.value()[0];
            } else if (profile.tag() == TAG_SPN) {
                const size_t n = std::min(profile.valueSize(), sizeof(spn) - 1);
                memcpy(spn, profile.value(), n);
                spn[n] = '\0';
            } else if (profile.tag() == TAG_PROFILE_NAME) {
                const size_t n = std::min(profile.valueSize(), sizeof(name) - 1);
                memcpy(name, profile.value(), n);
                name[n] = '\0';
            }
        }
        if (!haveIccid) {
            continue;
        }
        if (state == 1) {
            ++info->enabled;
            memcpy(info->enabledIccid, iccid, sizeof(iccid));
        }
        const bool expected = containsIgnoreCase(spn, PARTICLE_ESIM_TEST_OPERATOR) ||
                containsIgnoreCase(name, PARTICLE_ESIM_TEST_OPERATOR);
        if (profileClass == PROFILE_CLASS_OPERATIONAL && info->operationalCount < MAX_PROFILES) {
            auto& p = info->operational[info->operationalCount++];
            strlcpy(p.iccid, iccid, sizeof(p.iccid));
            p.expectedOperator = expected;
        }
        Log.info("eSIM profile %s: %s, class %d, spn \"%s\", name \"%s\"%s", iccid,
                state == 1 ? "enabled" : "disabled", profileClass, spn, name,
                expected ? " (" PARTICLE_ESIM_TEST_OPERATOR ")" : "");
    }
    return 0;
}

// Reads the profile list. Negative means the card has no eUICC.
int readProfilesOnce(ProfileInfo* info) {
    const int channel = CHECK(g_apdu.channel());
    // GetProfilesInfo asking for iccid, profileState, profileClass, serviceProviderName and
    // profileName. Only one GET RESPONSE is issued below, so keep the tag list lean: the Le field
    // is a single byte and a card with many named profiles could otherwise overflow 255 bytes.
    const uint8_t req[] = { 0xbf, 0x2d, 0x08, 0x5c, 0x06, 0x5a, 0x9f, 0x70, 0x95, 0x91, 0x92 };
    uint8_t resp[300] = {};
    size_t respSize = sizeof(resp);
    CHECK(storeData(channel, req, sizeof(req), resp, &respSize));
    *info = ProfileInfo();
    CHECK(parseProfiles(resp, respSize, info));
    return info->enabled;
}

// tag is 0xbf31 to enable, 0xbf32 to disable
int setProfileStateOnce(uint16_t tag, const char* iccid) {
    const int channel = CHECK(g_apdu.channel());
    uint8_t req[] = {
        (uint8_t)(tag >> 8), (uint8_t)(tag & 0xff), 0x11,
        0xa0, 0x0c, 0x5a, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x81, 0x01, 0x00 // refreshFlag off, the NCP client cycles CFUN itself
    };
    iccidToBcd(iccid, req + 7);
    uint8_t resp[32] = {};
    size_t respSize = sizeof(resp);
    CHECK(storeData(channel, req, sizeof(req), resp, &respSize));
    // BF31/BF32 <len> 80 01 <result>. 0 is ok, the rest are the SGP.22 reasons: 1 not found,
    // 2 wrong state, 3 disallowed by policy, 4 wrong reenabling, 5 CAT busy, 6 undefined.
    Tlv outer(resp, respSize);
    if (outer.next() && (outer.tag() == 0xbf31 || outer.tag() == 0xbf32)) {
        Tlv body(outer.value(), outer.valueSize());
        if (body.next() && body.tag() == 0x80 && body.valueSize() == 1) {
            const int result = body.value()[0];
            if (result != 0) {
                Log.error("%s profile %s refused by the eUICC, result %d",
                        tag == 0xbf31 ? "Enable" : "Disable", iccid, result);
                return SYSTEM_ERROR_INVALID_STATE;
            }
            return 0;
        }
    }
    Log.error("Unexpected response to %s profile, %u bytes", tag == 0xbf31 ? "enable" : "disable",
            (unsigned)respSize);
    return SYSTEM_ERROR_BAD_DATA;
}

// A dropped byte on the modem UART leaves the AT parser hunting for a command echo that can no
// longer match, and it sits on the parser command timeout before giving up. Seen on msom, where
// listening mode brings up the Wi-Fi and BLE radios right as we start talking to the card. The
// parser resyncs on the next command, and dropping the session first means the retry opens a fresh
// channel, so another go from the top is safe. That also covers the channel going away under us.
template <typename F>
int apduRetry(const char* what, F&& fn) {
    int r = 0;
    for (int i = 0; i < 3; ++i) {
        r = fn();
        if (r >= 0) {
            return r;
        }
        Log.warn("%s failed with %d, retrying", what, r);
        g_apdu.close();
    }
    return r;
}

int readProfiles(ProfileInfo* info) {
    return apduRetry("Reading profiles", [&] {
        return readProfilesOnce(info);
    });
}

int setProfileState(uint16_t tag, const char* iccid) {
    return apduRetry(tag == 0xbf31 ? "Enabling profile" : "Disabling profile", [&] {
        return setProfileStateOnce(tag, iccid);
    });
}

// A lost AT echo holds the whole AT layer until that command times out, up to three minutes, and
// the parser resyncs on the next one. Nothing below cares how quickly the modem answers, only that
// it does, so ride the stall out rather than failing a device that recovers on its own.
const system_tick_t AT_RECOVERY_TIMEOUT = 4 * 60 * 1000;

// APDUs go through checkParser(), which wants the modem powered and the NCP client ON, so every
// test has to bring the modem up for itself. The runner resets the device between tests.
//
// Not Cellular.on(). When the suite runs over another radio the runner blocks the interfaces it is
// not testing, test_suite.cpp calling network_off() with NETWORK_STATE_PARAM_BLOCK, and
// Cellular.on() passes no unblock flag so it cannot undo that. Reaching the eUICC needs the modem
// regardless of which radio carries the cloud, which is the whole point of the multi radio case.
// Unblocking where nothing was blocked is a no-op, blockInterface() only acts on a BLOCKED state.
bool modemUp() {
    network_on(Cellular, 0, NETWORK_STATE_PARAM_UNBLOCK, nullptr);
    return waitFor(Cellular.isOn, AT_RECOVERY_TIMEOUT);
}

// Changing a profile on a registered modem means detaching first, and listening mode is how the
// tooling does it. It parks the interface but leaves the NCP client ON, because downImpl() runs
// disconnect() before disable() and disable() then takes its not-connected early out. A bare
// Cellular.disconnect() gets no such protection and leaves APDUs failing with INVALID_STATE.
bool listenBegin() {
    Cellular.listen();
    return waitFor(Cellular.listening, AT_RECOVERY_TIMEOUT);
}

bool listenEnd() {
    Cellular.listen(false);
    return waitForNot(Cellular.listening, AT_RECOVERY_TIMEOUT);
}

int disableAllProfiles() {
    ProfileInfo info;
    for (int i = 0; i < 8; ++i) {
        const int enabled = CHECK(readProfiles(&info));
        if (enabled == 0) {
            return 0;
        }
        CHECK(setProfileState(0xbf32, info.enabledIccid));
    }
    // Not SYSTEM_ERROR_TIMEOUT, the AT parser returns that too and the two would be unreadable
    // in a failed assertEqual()
    return SYSTEM_ERROR_LIMIT_EXCEEDED;
}

// Belt and braces: g_skip is set by the setup test, and a missing ICCID means setup never got far
// enough for anything below to mean anything
bool shouldSkip() {
    return g_skip || !g_savedIccidValid;
}

// The same budget the system itself gets to bring the cloud up. Anything shorter and a HAL stall
// fails the test where the system would have recovered on its own, which is not what this checks.
const system_tick_t MULTI_RADIO_CLOUD_TIMEOUT = HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME;

// Whether this part is expected to carry the cloud on something other than cellular. The runner
// forces one interface per suite, so this is known up front rather than sampled, msom on Wi-Fi
// being the HIL case. Nothing forced means a bench run, where a b5som on Ethernet is the case but
// an unplugged cable should not fail anyone, so there it stays optional.
bool secondRadioExpected() {
    const auto net = TestSuite::instance()->network();
    return net != NETWORK_INTERFACE_ALL && net != NETWORK_INTERFACE_CELLULAR;
}

// True when some radio other than cellular is carrying the cloud. Evaluated while cellular is
// idle, so a connected cloud can only be coming from elsewhere.
//
// The cloud has to be asked for by hand. The suite runs SEMI_AUTOMATIC and nothing in the harness
// ever calls Particle.connect(), so there is no standing desire to re-raise the other interface
// after test 01's listening mode clears IFF_UP on all of them and exit() leaves them down.
// Raises the interface and the cloud desire without waiting on either. Called early so the cloud
// comes up alongside whatever else the test is waiting for. A single threaded run has one loop for
// everything and cannot afford to do these one after the other, the runner gives up first.
void requestMultiRadioCloud() {
    if (!secondRadioExpected() || Particle.connected()) {
        return;
    }
    const auto net = TestSuite::instance()->network();
    // Both calls are needed. network_on() unblocks the interface and powers the radio, but an
    // unblock only takes it back to DISABLED and network_on() never calls enableInterface(), so on
    // its own it leaves a powered radio that activateConnections() still skips. network_connect()
    // is what marks it enabled and brings the netif up.
    network_on(net, 0, NETWORK_STATE_PARAM_UNBLOCK, nullptr);
    network_connect(net, 0, 0, nullptr);
    Particle.connect();
}

bool multiRadioCloud() {
    if (!secondRadioExpected()) {
        // Cellular-only part, or a bench run with no forced network. Asking a deliberately idle
        // modem for the cloud would just burn the full connect timeout, so only report what is
        // already up.
        const bool r = Particle.connected() && !Cellular.ready();
        Log.info("Multi radio cloud checks %s", r ? "enabled" : "skipped, no second radio on this part");
        return r;
    }
    if (!Particle.connected()) {
        requestMultiRadioCloud();
        // The spec side gives this test a matching window, so let it run its course
        waitFor(Particle.connected, MULTI_RADIO_CLOUD_TIMEOUT);
    }
    const bool r = Particle.connected() && !Cellular.ready();
    Log.info("Multi radio cloud checks %s", r ? "enabled" : "FAILED, second radio expected but no cloud");
    return r;
}

bool isIdle() {
    return cellular_is_idle(nullptr) == 1;
}

// waitFor()'s exit condition and its return value are non-atomic, it evaluates the condition twice
// and hands back the second read. cellular_is_idle() is an unlocked live read of state the NCP
// thread owns, so a change landing between the two fails a test that already saw what it was
// waiting for. Latch the sighting and wait on that instead.
bool g_sawIdle = false;

bool sawIdle() {
    g_sawIdle = g_sawIdle || isIdle();
    return g_sawIdle;
}

bool modemAnswersAt() {
    return Cellular.command(10000, "AT\r\n") == RESP_OK;
}

template <typename F>
bool retryUntil(const char* what, F&& fn) {
    const system_tick_t start = millis();
    for (;;) {
        if (fn()) {
            return true;
        }
        if (millis() - start >= AT_RECOVERY_TIMEOUT) {
            return false;
        }
        Log.warn("%s failed, retrying", what);
        delay(1000);
    }
}

} // anonymous

test(00_esim_setup) {
    // Deliberately not gated on the network under test. A Wi-Fi msom still carries a modem and an
    // eUICC, and provisioning it is a real case: the cloud rides Wi-Fi while the eSIM sits idle.
    // Having an eUICC at all is the only thing that matters, and readProfiles() answers that below.
    assertTrue(modemUp());

    ProfileInfo info;
    const int enabled = readProfiles(&info);
    if (enabled < 0) {
        // No eUICC on this part, nothing to test and nothing to restore
        Log.info("No eUICC present (%d), skipping the eSIM suite", enabled);
        g_skip = true;
        g_savedIccidValid = false;
        skip();
        return;
    }
    Log.info("eUICC has %d profile(s), %d enabled, %d operational", info.total, enabled,
            info.operationalCount);

    // Every test device is expected to carry an operational profile
    char iccid[21] = {};
    assertEqual(pickProfile(info, iccid, sizeof(iccid)), 0);

    if (strcmp(info.enabledIccid, iccid)) {
        // Either nothing is enabled because a previous run was interrupted, or the wrong profile
        // is. Registration has to happen on the one we picked, so make it so.
        Log.info("Enabling %s", iccid);
        assertTrue(listenBegin());
        int r = disableAllProfiles();
        if (r == 0) {
            r = setProfileState(0xbf31, iccid);
        }
        assertTrue(listenEnd());
        assertEqual(r, 0);
        assertEqual(readProfiles(&info), 1);
    }

    // What the suite restores at the end
    strlcpy(g_savedIccid, iccid, sizeof(g_savedIccid));
    g_savedIccidValid = true;
    pushMailboxMsg(String::format("esim_profile=%s", g_savedIccid), 5000 /* wait */);

    assertEqual(String(info.enabledIccid), String(g_savedIccid));
}

test(01_esim_disable_all_profiles) {
    if (shouldSkip()) {
        skip();
        return;
    }
    assertTrue(modemUp());

    assertTrue(listenBegin());
    const int r = disableAllProfiles();
    assertTrue(listenEnd());
    assertEqual(r, 0);

    expectSystemReset();
    System.reset();
}

test(02_esim_enters_idle) {
    if (shouldSkip()) {
        skip();
        return;
    }
    assertTrue(modemUp());

    // On a multi interface part the cloud rides Wi-Fi or Ethernet, and cellular dropping into idle
    // must not disturb it. Asked for here and checked below, so it connects during the idle wait
    // instead of adding its own.
    requestMultiRadioCloud();

    Cellular.connect();

    // connect() probes the eUICC and parks the radio instead of registering. A stalled AT command
    // on the way in can push that out by the length of its timeout, so allow for one
    g_sawIdle = false;
    assertTrue(waitFor(sawIdle, AT_RECOVERY_TIMEOUT));
    assertFalse(Cellular.ready());

    // Cellular-only parts have no cloud here at all, so this only asserts where it means something
    const bool cloudViaOtherIface = multiRadioCloud();
    // Required where the runner forced a non-cellular network, so a missing cloud fails here
    // rather than quietly dropping the checks below
    if (secondRadioExpected()) {
        assertTrue(cloudViaOtherIface);
    }

    if (cloudViaOtherIface) {
        Log.info("Cloud is up on another interface, checking idle left it alone");
        assertTrue(Particle.connected());
    }

    // Two things IDLE buys us. The modem stays powered and reachable, so nothing resets it out
    // from under a tool, and with no profile enabled the radio stops thrashing at a network it
    // cannot join.
    assertTrue(retryUntil("AT probe", modemAnswersAt));
    assertTrue(Cellular.isOn());
}

test(03_esim_idle_survives_apdu_traffic) {
    if (shouldSkip()) {
        skip();
        return;
    }
    assertTrue(modemUp());
    assertTrue(isIdle());

    // Tooling reads the profile list while idle. This must not knock the modem over or move us
    // out of IDLE, which is the regression the netif used to cause. Tooling will normally use
    // listening mode, but idle has to hold up on its own.
    for (int i = 0; i < 3; ++i) {
        ProfileInfo info;
        assertEqual(readProfiles(&info), 0);
        assertTrue(isIdle());
    }
    assertTrue(retryUntil("AT probe", modemAnswersAt));
    assertFalse(Cellular.ready());
}

test(04_esim_leaves_idle_when_profile_enabled) {
    if (shouldSkip()) {
        skip();
        return;
    }
    assertTrue(modemUp());
    requestMultiRadioCloud();
    assertTrue(isIdle());

    // Cellular is idle, so anything connected here is on another radio. That link has to survive
    // the whole idle to enabled transition, APDU traffic and cellular reconnect included.
    const bool cloudViaOtherIface = multiRadioCloud();
    // Required where the runner forced a non-cellular network, so a missing cloud fails here
    // rather than quietly dropping the checks below
    if (secondRadioExpected()) {
        assertTrue(cloudViaOtherIface);
    }

    assertEqual(setProfileState(0xbf31, g_savedIccid), 0);
    ProfileInfo info;
    assertEqual(readProfiles(&info), 1);
    assertEqual(String(info.enabledIccid), String(g_savedIccid));
    if (cloudViaOtherIface) {
        assertTrue(Particle.connected());
    }

    // A reconnect is what re-probes the eUICC, idle is deliberately sticky until then
    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
    if (cloudViaOtherIface) {
        assertTrue(Particle.connected());
    }
    Cellular.connect();

    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
    assertFalse(isIdle());
    if (cloudViaOtherIface) {
        Log.info("Cloud held across the whole idle to enabled transition");
        assertTrue(Particle.connected());
    }
}

test(05_esim_iccid_matches_enabled_profile) {
    if (shouldSkip()) {
        skip();
        return;
    }
    // The NCP client cycles CFUN until the modem loads the profile we enabled, so the ICCID the
    // modem reports has to agree with the eUICC
    CellularDevice dev = {};
    assertTrue(retryUntil("Reading device info", [&] {
        dev = {};
        dev.size = sizeof(dev);
        return cellular_device_info(&dev, nullptr) == 0;
    }));
    Log.info("Modem reports ICCID %s, expected %s", dev.iccid, g_savedIccid);
    assertEqual(String(dev.iccid), String(g_savedIccid));
}

test(99_esim_cleanup) {
    if (shouldSkip()) {
        skip();
        return;
    }
    // Leave the part the way we found it even if something above failed. Re-read rather than
    // trusting the saved value, the operational profile is the one that has to end up enabled.
    assertTrue(modemUp());
    ProfileInfo info;
    if (readProfiles(&info) >= 0) {
        // g_savedIccid is what we picked in setup, so pickProfile() will land on it again
        char iccid[21] = {};
        if (info.enabled == 0 && pickProfile(info, iccid, sizeof(iccid)) == 0) {
            Log.info("Restoring profile %s", iccid);
            if (listenBegin()) {
                setProfileState(0xbf31, iccid);
                listenEnd();
            }
        }
        assertEqual(readProfiles(&info), 1);
    }

    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

