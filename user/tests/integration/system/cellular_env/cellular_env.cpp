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

#include "application.h"
#include "unit-test/unit-test.h"
#include "test_suite.h"
#include "check.h"

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
//     // { "comm.coap", LOG_LEVEL_ALL },
//     // { "app", LOG_LEVEL_ALL }
// });

namespace {

#if HAL_PLATFORM_CELLULAR

enum class ModemType {
    QUECTEL,
    UBLOX,
    UNSUPPORTED
};

ModemType modemType = ModemType::UNSUPPORTED;

int getNcpId() {
    int ncpId = PLATFORM_NCP_NONE;
        auto info = System.hardwareInfo();
        if (info.isValid() && info.ncp().size() > 0) {
    #if HAL_PLATFORM_NCP
            ncpId = info.ncp()[0];
    #endif
        }

    return ncpId;
}

ModemType getModemType() {
    if (modemType != ModemType::UNSUPPORTED) {
        return modemType;
    }

    // Force skip if not testing on cellular network interface
    auto network = (int)TestSuite::instance()->network();
    if (network != NETWORK_INTERFACE_CELLULAR && network != NETWORK_INTERFACE_ALL) {
        modemType = ModemType::UNSUPPORTED;
        return modemType;
    }

    switch (getNcpId()) {
        case PLATFORM_NCP_QUECTEL_BG95_M5:
        case PLATFORM_NCP_QUECTEL_BG96:
        case PLATFORM_NCP_QUECTEL_EG91_NAX:
        case PLATFORM_NCP_QUECTEL_EG91_E:
        case PLATFORM_NCP_QUECTEL_EG91_EX:
            modemType = ModemType::QUECTEL; break;
        case PLATFORM_NCP_SARA_R410:
        case PLATFORM_NCP_SARA_R510:
            modemType = ModemType::UBLOX; break;
        default:
            modemType = ModemType::UNSUPPORTED;
    }

    Log.info("NCPID: %02x %d", getNcpId(), (int) modemType);

    return modemType;
}

struct BandCallbackData {
    String response;
    bool found;
};

struct CpolCallbackData {
    String response;
};

int cbQCFGBand(int type, const char* buf, int len, char* lteBuf) {
    if ((type == TYPE_PLUS) && lteBuf) {
        sscanf(buf, "\r\n+QCFG: \"band\",0x%*[^,],0x%23[^,]", lteBuf);
    }
    return WAIT;
}

int cbUMNOPROF(int type, const char* buf, int len, int* profile) {
    if ((type == TYPE_PLUS) && profile) {
        sscanf(buf, "\r\n+UMNOPROF: %d", profile);
    }
    return WAIT;
}

bool isUbandmaskSupported() {
    int profile = -1;
    if (RESP_OK != Cellular.command(cbUMNOPROF, &profile, 10000, "AT+UMNOPROF?\r\n") || profile < 0) {
        return false;
    }
    return (profile == 90 || profile == 100);
}

int cbUBANDMASK(int type, const char* buf, int len, char* catM1Buf) {
    if ((type == TYPE_PLUS) && catM1Buf) {
        sscanf(buf, "\r\n+UBANDMASK: 0,%23[^,]", catM1Buf);
    }
    return WAIT;
}

int cbCPOL(int type, const char* buf, int len, CpolCallbackData* data) {
    if (type == TYPE_PLUS) {
        data->response += String(buf, len);
    }
    return WAIT;
}

String readCurrentLteMask() {
    if (modemType == ModemType::QUECTEL) {
        char lteBuf[24] = {};
        if (RESP_OK != Cellular.command(cbQCFGBand, lteBuf, 30000, "AT+QCFG=\"band\"\r\n")
                || lteBuf[0] == '\0') {
            return "";
        }
        String mask = String(lteBuf);
        mask.toLowerCase();
        return mask;

    } else if (modemType == ModemType::UBLOX) {
        char catM1Buf[24] = {};
        if (RESP_OK != Cellular.command(cbUBANDMASK, catM1Buf, 10000, "AT+UBANDMASK?\r\n")
                || catM1Buf[0] == '\0') {
            return "";
        }
        char* pEnd = catM1Buf;
        uint64_t val = strtoull(catM1Buf, &pEnd, 10);
        if (pEnd == catM1Buf) {
            return "";
        }

        char hexBuf[17] = {};
        snprintf(hexBuf, sizeof(hexBuf), "%llx", val);

        return String(hexBuf);
    }
    return "";
}

// Simplifed checker for lowest 4 bands/bits only!!
bool isBandEnabled(const String& hexMask, int band) {
    if (hexMask.length() == 0) {
        return true;
    }
    char c = hexMask.charAt(hexMask.length() - 1);
    int v = strtoul(&c, nullptr, 16);
    return (v & (1 << (band - 1)));
}

#endif // HAL_PLATFORM_CELLULAR

} // anonymous

#if HAL_PLATFORM_CELLULAR

test(1_particle_cellular_preferred_bands_init) {
    // Just in case
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(2_particle_cellular_preferred_bands_default) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        Cellular.connect();
        assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

        String defaultMask;
        SCOPE_GUARD({
            pushMailboxMsg(String::format("Default LTE band mask (%s): %s",
                    modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                    defaultMask.c_str()), 5000 /* wait */);
        });

        defaultMask = readCurrentLteMask();
        Log.info("Default LTE band mask (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                defaultMask.c_str());
        assertTrue(defaultMask.length() > 0);


        // band 1 or band 2 should be enabled
        assertTrue(isBandEnabled(defaultMask, 1) || isBandEnabled(defaultMask, 2));

        Cellular.disconnect();
        waitForNot(Cellular.ready, 60000);
    } else {
        pushMailboxMsg(String::format("Skipping default bands check"), 5000 /* wait */);
    }
}

test(3_particle_cellular_preferred_bands_set) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"),
            String("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC")); // disables bands 1 & 2
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"), String("0"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));

    if (modemType == ModemType::UBLOX) {
        // Should be supported now if profile setup properly due to env vars
        assertTrue(isUbandmaskSupported());
    }

    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

    String newMask;
    SCOPE_GUARD({
        pushMailboxMsg(String::format("LTE band mask after PREFERRED_BANDS env var (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                newMask.c_str()), 5000 /* wait */);
    });

    newMask = readCurrentLteMask();
    Log.info("LTE band mask after PREFERRED_BANDS env var (%s): %s",
            modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
            newMask.c_str());
    assertTrue(newMask.length() > 0);

    // band 1 AND band 2 should be disabled
    assertTrue(!isBandEnabled(newMask, 1) && !isBandEnabled(newMask, 2));

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(4_particle_cellular_forbidden_bands_init) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(5_particle_cellular_forbidden_bands_default) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        Cellular.connect();
        assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

        String defaultMask;
        SCOPE_GUARD({
            pushMailboxMsg(String::format("Default LTE band mask (%s): %s",
                    modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                    defaultMask.c_str()), 5000 /* wait */);
        });

        defaultMask = readCurrentLteMask();
        Log.info("Default LTE band mask (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                defaultMask.c_str());
        assertTrue(defaultMask.length() > 0);

        // band 1 or band 2 should be enabled
        assertTrue(isBandEnabled(defaultMask, 1) || isBandEnabled(defaultMask, 2));

        Cellular.disconnect();
        waitForNot(Cellular.ready, 60000);
    } else {
        pushMailboxMsg(String::format("Skipping default bands check"), 5000 /* wait */);
    }
}

test(6_particle_cellular_forbidden_bands_set) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertTrue(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"),
            String("3")); // bits 0 & 1 → bands 1 & 2 forbidden
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"),
            String("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));

    if (modemType == ModemType::UBLOX) {
        // Should be supported now if profile setup properly due to env vars
        assertTrue(isUbandmaskSupported());
    }

    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

    String newMask;
    SCOPE_GUARD({
        pushMailboxMsg(String::format("LTE band mask after FORBIDDEN_BANDS env var (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                newMask.c_str()), 5000 /* wait */);
    });

    newMask = readCurrentLteMask();
    Log.info("LTE band mask after FORBIDDEN_BANDS env var (%s): %s",
            modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
            newMask.c_str());
    assertTrue(newMask.length() > 0);

    // band 1 AND band 2 must now be disabled
    assertTrue(!isBandEnabled(newMask, 1) && !isBandEnabled(newMask, 2));

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(7_particle_cellular_preferred_plmn_init) {
    int ncpId = getNcpId();
    if (getModemType() == ModemType::UNSUPPORTED ||
            ncpId == PLATFORM_NCP_SARA_R410 ||
            ncpId == PLATFORM_NCP_QUECTEL_BG96) {
        skip();
        return;
    }

    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(8_particle_cellular_preferred_plmn_default) {
    int ncpId = getNcpId();
    if (getModemType() == ModemType::UNSUPPORTED ||
            ncpId == PLATFORM_NCP_SARA_R410 ||
            ncpId == PLATFORM_NCP_QUECTEL_BG96) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

    CpolCallbackData data = {};
    SCOPE_GUARD({
        pushMailboxMsg(String::format("Default CPOL response: %s",
                data.response.length() ? data.response.c_str() : "empty"), 5000 /* wait */);
    });

    Cellular.command(cbCPOL, &data, 10000, "AT+CPOL?\r\n");

    bool anyPlmn = (data.response.indexOf("310410") >= 0) ||
            (data.response.indexOf("310260") >= 0) ||
            (data.response.indexOf("311480") >= 0);
    assertFalse(anyPlmn);

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(9_particle_cellular_preferred_plmn_set) {
    int ncpId = getNcpId();
    if (getModemType() == ModemType::UNSUPPORTED ||
            ncpId == PLATFORM_NCP_SARA_R410 ||
            ncpId == PLATFORM_NCP_QUECTEL_BG96) {
        skip();
        return;
    }

    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"),
            String("310410,310260,311480"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

    CpolCallbackData data = {};
    SCOPE_GUARD({
        pushMailboxMsg(String::format("AT+CPOL response after PREFERRED_PLMN env var: %s",
                data.response.length() ? data.response.c_str() : "empty"), 5000 /* wait */);
    });

    Cellular.command(cbCPOL, &data, 10000, "AT+CPOL?\r\n");

    assertTrue(data.response.indexOf("310410") >= 0);
    assertTrue(data.response.indexOf("310260") >= 0);
    assertTrue(data.response.indexOf("311480") >= 0);

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(10_particle_cellular_preferred_plmn_cleanup) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(11_particle_cellular_env_vars_cleared_verify_defaults) {
    if (getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 120000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    int ncpId = getNcpId();

    String mask;
    CpolCallbackData data = {};
    SCOPE_GUARD({
        if (getModemType() != ModemType::UNSUPPORTED &&
                ncpId != PLATFORM_NCP_SARA_R410 &&
                ncpId != PLATFORM_NCP_QUECTEL_BG96) {
            pushMailboxMsg(String::format("Band mask after env clear: %s\nAT+CPOL response after env clear: %s",
                    mask.c_str(), data.response.length() ? data.response.c_str() : "empty"), 5000 /* wait */);
        } else {
            pushMailboxMsg(String::format("Band mask after env clear: %s", mask.c_str()), 5000 /* wait */);
        }
    });

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        // band 1 or band 2 should be re-enabled
        mask = readCurrentLteMask();
        assertTrue(mask.length() > 0);
        assertTrue(isBandEnabled(mask, 1) || isBandEnabled(mask, 2));
    }

    if (getModemType() != ModemType::UNSUPPORTED &&
                ncpId != PLATFORM_NCP_SARA_R410 &&
                ncpId != PLATFORM_NCP_QUECTEL_BG96) {
        // UPLMN list: 310410, 310260, 311480 should no longer be set
        Cellular.command(cbCPOL, &data, 10000, "AT+CPOL?\r\n");
        bool anyPlmn = (data.response.indexOf("310410") >= 0) ||
                (data.response.indexOf("310260") >= 0) ||
                (data.response.indexOf("311480") >= 0);
        assertFalse(anyPlmn);
    }

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

#endif // HAL_PLATFORM_CELLULAR

test(99_cleanup) {
    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}
