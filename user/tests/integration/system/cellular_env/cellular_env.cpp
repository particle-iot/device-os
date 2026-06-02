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
#include "ncp_env_var.h"
#include "hex_to_bytes.h"
#include "endian_util.h"

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
//     // { "comm.coap", LOG_LEVEL_ALL },
//     // { "app", LOG_LEVEL_ALL }
// });

namespace {

static const int CELLULAR_KEEPALIVE_SECONDS = 60;
static const int GLOBAL_KEEPALIVE_SECONDS = 70;
static const int PLATFORM_CELLULAR_KEEPALIVE_SECONDS_DEFAULT = HAL_PLATFORM_CELLULAR_CLOUD_KEEPALIVE_INTERVAL / 1000;

retained bool g_SkipTests = false;

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
        case PLATFORM_NCP_QUECTEL_BG95_S5:
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

int cbQCFGBand(int type, const char* buf, int len, CellularBandMask* bands) {
    char bandsStr[24] = {};
    if (type == TYPE_PLUS) {
        int resp = sscanf(buf, "\r\n+QCFG: \"band\",0x%*[^,],0x%23[^,]", bandsStr);
        if (resp == 1) {
            bands->setFromHexString(bandsStr);
        }
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

int cbUBANDMASK(int type, const char* buf, int len, CellularBandMask* bands) {
    char uband01_64Str[24] = {};
    char uband65_128Str[24] = {};
    if (type == TYPE_PLUS) {
        int resp = sscanf(buf, "\r\n+UBANDMASK: 0,%23[^,],%23[^,]", uband01_64Str, uband65_128Str);
        if (resp == 2) {
            bands->setFromDecimalStrings(uband01_64Str, uband65_128Str);
        }
    }
    return WAIT;
}

int cbCPOL(int type, const char* buf, int len, CpolCallbackData* data) {
    if (type == TYPE_PLUS) {
        data->response += String(buf, len);
    }
    return WAIT;
}

CellularBandMask readCurrentLteMask() {
    CellularBandMask bands;
    if (modemType == ModemType::QUECTEL) {
        Cellular.command(cbQCFGBand, &bands, 30000, "AT+QCFG=\"band\"\r\n");
    } else if (modemType == ModemType::UBLOX) {
        Cellular.command(cbUBANDMASK, &bands, 10000, "AT+UBANDMASK?\r\n");
        if (getNcpId() == PLATFORM_NCP_SARA_R410) {
            bands.setHigh(0);
        }
    }
    return bands;
}

CellularBandMask makeDefaultBandMask(bool readMode = false) {
    int ncpId = getNcpId();
    bool isR410 = (ncpId == PLATFORM_NCP_SARA_R410);
    if (modemType == ModemType::QUECTEL) {
        return CellularBandMask(
            getBandMaskByNcpIdForRAT(ncpId, CellularAccessTechnology::LTE_CAT_M1, false /* upper */, false /* legacy */),
            getBandMaskByNcpIdForRAT(ncpId, CellularAccessTechnology::LTE_CAT_M1, true /* upper */, false /* legacy */)
        );
    } else if (modemType == ModemType::UBLOX) {
        return CellularBandMask(
            getBandMaskByNcpIdForRAT(ncpId, CellularAccessTechnology::LTE_CAT_M1, false /* upper */, readMode && isR410 /* legacy */),
            getBandMaskByNcpIdForRAT(ncpId, CellularAccessTechnology::LTE_CAT_M1, true /* upper */, false /* legacy */)
        );
    }
    return CellularBandMask{};
}

int lowestHighBand(const CellularBandMask& mask) {
    uint64_t high = mask.high();
    for (int i = 0; i < 64; i++) {
        if (high & ((uint64_t)1 << i)) {
            return 65 + i;
        }
    }
    return -1;
}

CellularBandMask makeExpectedPostEnvBandMask() {
    CellularBandMask expected = makeDefaultBandMask(false /* readMode */);
    expected.setLow(expected.low() & ~(uint64_t)0x03);
    int highBand = lowestHighBand(expected);
    if (highBand >= 65) {
        expected.setHigh(expected.high() & ~((uint64_t)1 << (highBand - 65)));
    }
    return expected;
}

#endif // HAL_PLATFORM_CELLULAR

enum class FirmwareUpdateStatus {
    NONE,
    STARTED,
    SUCCESS,
    ERROR
};

auto firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
std::atomic<int> firmwareUpdateProgressCount;

void firmwareUpdateEventHandler(system_event_t, int data, void*) {
    switch (data) {
    case firmware_update_begin:
        Test::out->println("firmware_update_begin");
        firmwareUpdateStatus = FirmwareUpdateStatus::STARTED;
        break;
    case firmware_update_complete:
        Test::out->println("firmware_update_complete");
        firmwareUpdateStatus = FirmwareUpdateStatus::SUCCESS;
        break;
    case firmware_update_progress:
        ++firmwareUpdateProgressCount;
        break;
    default:
        Test::out->printlnf("Unexpected firmware update status: %d", data);
        firmwareUpdateStatus = FirmwareUpdateStatus::ERROR;
        break;
    }
}

void prepareForFirmwareUpdate() {
    System.disableReset();
    System.on(firmware_update, firmwareUpdateEventHandler);
    firmwareUpdateStatus = FirmwareUpdateStatus::NONE;
    firmwareUpdateProgressCount = 0;
}

void completeFirmwareUpdate(bool expectSafeMode = false) {
    bool ok = false;
    auto t1 = millis();
    for (;;) {
        Particle.process();
        if (firmwareUpdateStatus == FirmwareUpdateStatus::SUCCESS) {
            ok = true;
            break;
        }
        if (firmwareUpdateStatus == FirmwareUpdateStatus::ERROR) {
            Test::out->println("Firmware update failed");
            break;
        }
        // The JS part of the test waits until the OTA completes so the timeout here is for
        // finalizing the update on the device
        if (millis() - t1 >= 30000) {
            Test::out->println("Firmware update timeout");
            break;
        }
    }
    Test::out->printlnf("firmware_update_progress count: %d", firmwareUpdateProgressCount.load());
    System.off(firmware_update);
    if (ok) {
        if (expectSafeMode) {
            TestRunner::instance()->expectSafeMode();
        } else {
            TestRunner::instance()->expectSystemReset();
        }
    }
    assertTrue(ok);
    System.enableReset();
}

} // anonymous

#if HAL_PLATFORM_CELLULAR

test(1_particle_cellular_preferred_bands_init) {
    if (getModemType() == ModemType::UNSUPPORTED ||
        (TestSuite::instance()->network() != NETWORK_INTERFACE_ALL &&
         TestSuite::instance()->network() != NETWORK_INTERFACE_CELLULAR)) {
        g_SkipTests = true;
    } else {
        g_SkipTests = false;
    }
    if (g_SkipTests) {
        skip();
        return;
    }

    // Just in case
    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);

    if (getModemType() != ModemType::UNSUPPORTED) {
        Cellular.on();
        assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

        CellularBandMask preferred = makeExpectedPostEnvBandMask();
        Log.info("Computed preferred bands mask: %s", (const char*)preferred.toString());
        pushMailboxMsg(String::format("PARTICLE_CELLULAR_PREFERRED_BANDS=%s",
                (const char*)preferred.toString()), 5000 /* wait */);

        Cellular.disconnect();
        waitForNot(Cellular.ready, 60000);
    }

    expectSystemReset();
    System.reset();
}

test(2_particle_cellular_preferred_bands_default) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        CellularBandMask bands;
        SCOPE_GUARD({
            pushMailboxMsg(String::format("Default LTE band mask (%s): %s",
                    modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                    (const char*)bands.toString()), 5000 /* wait */);
        });

        bands = readCurrentLteMask();
        Log.info("Default LTE band mask (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                (const char*)bands.toString());
        assertFalse(bands.isEmpty() > 0);

        assertEqual((const char*)bands.toString(), (const char*)makeDefaultBandMask(true /* readMode */).toString());

        Cellular.disconnect();
        waitForNot(Cellular.ready, 60000);
    } else {
        pushMailboxMsg(String::format("Skipping default bands check"), 5000 /* wait */);
    }
}

test(3_particle_cellular_preferred_bands_set) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    CellularBandMask preferred = makeExpectedPostEnvBandMask();
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"), String((const char*)preferred.toString()));
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"), String("0"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    if (modemType == ModemType::UBLOX) {
        // Should be supported now if profile setup properly due to env vars
        assertTrue(isUbandmaskSupported());
    }

    CellularBandMask expectedDefault = makeExpectedPostEnvBandMask();

    CellularBandMask bands;
    SCOPE_GUARD({
        pushMailboxMsg(String::format("LTE band mask after PREFERRED_BANDS env var (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                (const char*)bands.toString()), 5000 /* wait */);
    });

    bands = readCurrentLteMask();
    Log.info("LTE band mask after PREFERRED_BANDS env var (%s): %s",
            modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
            (const char*)bands.toString());
    assertFalse(bands.isEmpty());
    assertEqual((const char*)bands.toString(), (const char*)expectedDefault.toString());

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(4_particle_cellular_forbidden_bands_init) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
    System.clearEnv(false /* reset */);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    CellularBandMask forbidden = ~makeExpectedPostEnvBandMask();
    Log.info("Computed forbidden bands mask: %s", (const char*)forbidden.toString());
    pushMailboxMsg(String::format("PARTICLE_CELLULAR_FORBIDDEN_BANDS=%s",
            (const char*)forbidden.toString()), 5000 /* wait */);

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);

    expectSystemReset();
    System.reset();
}

test(5_particle_cellular_forbidden_bands_default) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        Cellular.connect();
        assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));

        CellularBandMask bands;
        SCOPE_GUARD({
            pushMailboxMsg(String::format("Default LTE band mask (%s): %s",
                    modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                    (const char*)bands.toString()), 5000 /* wait */);
        });

        bands = readCurrentLteMask();
        Log.info("Default LTE band mask (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                (const char*)bands.toString());
        assertFalse(bands.isEmpty());

        assertEqual((const char*)bands.toString(), (const char*)makeDefaultBandMask(true /* readMode */).toString());

        Cellular.disconnect();
        waitForNot(Cellular.ready, 60000);
    } else {
        pushMailboxMsg(String::format("Skipping default bands check"), 5000 /* wait */);
    }
}

test(6_particle_cellular_forbidden_bands_set) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    CellularBandMask forbidden = ~makeExpectedPostEnvBandMask();
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"), String("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"), String((const char*)forbidden.toString()));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    if (modemType == ModemType::UBLOX) {
        // Should be supported now if profile setup properly due to env vars
        assertTrue(isUbandmaskSupported());
    }

    CellularBandMask expectedDefault = makeExpectedPostEnvBandMask();

    CellularBandMask bands;
    SCOPE_GUARD({
        pushMailboxMsg(String::format("LTE band mask after FORBIDDEN_BANDS env var (%s): %s",
                modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
                (const char*)bands.toString()), 5000 /* wait */);
    });

    bands = readCurrentLteMask();
    Log.info("LTE band mask after FORBIDDEN_BANDS env var (%s): %s",
            modemType == ModemType::QUECTEL ? "QUECTEL" : "UBLOX",
            (const char*)bands.toString());
    assertFalse(bands.isEmpty());
    assertEqual((const char*)bands.toString(), (const char*)expectedDefault.toString());

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(7_particle_cellular_preferred_plmn_init) {
    int ncpId = getNcpId();
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED ||
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
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED ||
            ncpId == PLATFORM_NCP_SARA_R410 ||
            ncpId == PLATFORM_NCP_QUECTEL_BG96) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
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
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED ||
            ncpId == PLATFORM_NCP_SARA_R410 ||
            ncpId == PLATFORM_NCP_QUECTEL_BG96) {
        skip();
        return;
    }

    assertTrue(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));
    assertEqual(System.getEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"),
            String("310410,310260,311480"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
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

test(10_particle_cellular_keepalive_init) {
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE"));
    assertFalse(System.hasEnv("PARTICLE_CLOUD_KEEP_ALIVE"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    assertEqual(Particle.getKeepAlive(), PLATFORM_CELLULAR_KEEPALIVE_SECONDS_DEFAULT);
}

test(11_particle_cellular_keepalive) {
    assertTrue(System.hasEnv("PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE"));
    int keepAlive = 0;
    assertTrue(System.getEnv("PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE", keepAlive));
    assertEqual(keepAlive, CELLULAR_KEEPALIVE_SECONDS);

    assertTrue(System.hasEnv("PARTICLE_CLOUD_KEEP_ALIVE"));
    int globalKeepAlive = 0;
    assertTrue(System.getEnv("PARTICLE_CLOUD_KEEP_ALIVE", globalKeepAlive));
    assertEqual(globalKeepAlive, GLOBAL_KEEPALIVE_SECONDS);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    assertEqual(Particle.getKeepAlive(), CELLULAR_KEEPALIVE_SECONDS);
    assertEqual(Particle.getKeepAlive(Cellular), CELLULAR_KEEPALIVE_SECONDS);
    Particle.disconnect();
    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

test(12_particle_cellular_preferred_plmn_cleanup) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    System.clearEnv(false /* reset */);
    expectSystemReset();
    System.reset();
}

test(13_particle_cellular_env_vars_cleared_verify_defaults) {
    if (g_SkipTests || getModemType() == ModemType::UNSUPPORTED) {
        skip();
        return;
    }

    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_FORBIDDEN_BANDS"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_PREFERRED_PLMN"));
    assertFalse(System.hasEnv("PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE"));
    assertFalse(System.hasEnv("PARTICLE_CLOUD_KEEP_ALIVE"));

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    int ncpId = getNcpId();

    CellularBandMask bands;
    CpolCallbackData data = {};
    SCOPE_GUARD({
        if (getModemType() != ModemType::UNSUPPORTED &&
                ncpId != PLATFORM_NCP_SARA_R410 &&
                ncpId != PLATFORM_NCP_QUECTEL_BG96) {
            pushMailboxMsg(String::format("Band mask after env clear: %s\nAT+CPOL response after env clear: %s",
                    (const char*)bands.toString(), data.response.length() ? data.response.c_str() : "empty"), 5000 /* wait */);
        } else {
            pushMailboxMsg(String::format("Band mask after env clear: %s", (const char*)bands.toString()), 5000 /* wait */);
        }
    });

    if ((modemType == ModemType::UBLOX && isUbandmaskSupported()) ||
            (modemType == ModemType::QUECTEL)) {

        bands = readCurrentLteMask();
        assertFalse(bands.isEmpty());
        assertEqual((const char*)bands.toString(), (const char*)makeDefaultBandMask(true /* readMode */).toString());
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

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual(Particle.getKeepAlive(), PLATFORM_CELLULAR_KEEPALIVE_SECONDS_DEFAULT);

    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
}

#endif // HAL_PLATFORM_CELLULAR

test(97_cleanup) {
    if (g_SkipTests) {
        skip();
        return;
    }

    System.clearEnv(false /* reset */);
    unlink("/sys/env_app");
    unlink("/sys/env_app.staged");
    unlink("/sys/env_snapshot");
    unlink("/sys/env_snapshot.staged");
    expectSystemReset();
    System.reset();
}

test(98_cleanup) {
    if (g_SkipTests) {
        skip();
        return;
    }
    prepareForFirmwareUpdate();
    Particle.disconnect(CloudDisconnectOptions().clearSession(true));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    // We are supposed to get an empty env
}

test(99_cleanup_1) {
    if (g_SkipTests) {
        skip();
        return;
    }
    completeFirmwareUpdate();
}

test(99_cleanup_2) {
    if (g_SkipTests) {
        skip();
        return;
    }

    if (firmwareUpdateStatus != FirmwareUpdateStatus::SUCCESS) {
        expectSystemReset();
        System.enableReset();
        // Should normally be unreachable
        delay(5000);
        System.reset();
    }
}
