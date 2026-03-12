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
#include "muon_test_util.h"
#include "test_system_cache.h"
#include "exrtc_hal_am18x5.h"
#include "exrtc_hal_internal.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if HAL_PLATFORM_EXTERNAL_RTC

namespace {

constexpr uint32_t EXRTC_SUITE_STATE_MAGIC = 0x45525443; // ERTC
constexpr int EXRTC_EEPROM_ADDR = 0;
constexpr int EXRTC_MAX_TIME_DRIFT_SECONDS = 10;
const char* const SYSTEM_CACHE_PATH = "/sys/cache.dat";
const char* const SYSTEM_CACHE_BACKUP_PATH = "/sys/cache.dat.bak";

struct ExRtcSuiteState {
    uint32_t magic = EXRTC_SUITE_STATE_MAGIC;
    bool initialized = false;
    bool shouldRun = false;
    bool isMuon = false;
    bool wroteLegacyCalibration = false;
    bool wroteNewCalibration = false;
    uint8_t pendingCalibrationScenario = 0;
    bool defaultTimeSource = false;
    uint8_t clockSource = 0;
    uint32_t capabilities = 0;
    int8_t expectedLegacyCalibration = 0;
    int8_t expectedNewCalibration = 0;
    bool legacyCalibrationExistedAtStart = false;
    bool newCalibrationExistedAtStart = false;
    bool cacheBackupAvailable = false;
};

constexpr int8_t LEGACY_CACHE_TEST_CALIBRATION = -17;
constexpr int8_t LEGACY_OVERRIDE_TEST_CALIBRATION = 9;
constexpr int8_t NEW_CACHE_TEST_CALIBRATION = -23;
constexpr int8_t NEW_OVERRIDE_TEST_CALIBRATION = 11;

enum : uint8_t {
    CALIBRATION_SCENARIO_NONE = 0,
    CALIBRATION_SCENARIO_LEGACY = 1,
    CALIBRATION_SCENARIO_NEW = 2,
    CALIBRATION_SCENARIO_DEFAULT = 3
};

using LegacyAm18x5ManufacturingConfig = am18x5_manufacturing_config_t;

ExRtcSuiteState readSuiteState() {
    ExRtcSuiteState state;
    EEPROM.get(EXRTC_EEPROM_ADDR, state);
    return state;
}

void writeSuiteState(const ExRtcSuiteState& state) {
    EEPROM.put(EXRTC_EEPROM_ADDR, state);
}

int64_t absTimeDiff(time_t lhs, time_t rhs) {
    return lhs >= rhs ? (int64_t)lhs - (int64_t)rhs : (int64_t)rhs - (int64_t)lhs;
}

void dumpTimeSources(const char* label) {
    const auto systemTime = Time.now();
    const auto internalTime = InternalTime.now();
    const auto externalTime = ExternalTime.now();
    Test::out->printlnf("%s: source=%d time=%lld internal=%lld external=%lld diff_ie=%lld diff_ti=%lld diff_te=%lld",
            label,
            (int)Time.timeSource(),
            (long long)systemTime,
            (long long)internalTime,
            (long long)externalTime,
            (long long)absTimeDiff(internalTime, externalTime),
            (long long)absTimeDiff(systemTime, internalTime),
            (long long)absTimeDiff(systemTime, externalTime));
}

void dumpExrtcConfig(const char* label) {
    const auto config = ExternalTime.getConfig();
    const auto status = ExternalTime.status();
    Test::out->printlnf("%s: config.valid=%d default=%d sleepExtiCheck=%d clockSource=%d caps=0x%08lx status.valid=%d bound=%d present=%d ready=%d status.clockSource=%d",
            label,
            config.valid(),
            config.valid() ? config.defaultTimeSource() : 0,
            config.valid() ? config.sleepExtiCheck() : 0,
            config.valid() ? (int)config.clockSource() : -1,
            config.valid() ? (unsigned long)config.capabilities().value() : 0ul,
            status.valid(),
            status.valid() ? status.bound() : 0,
            status.valid() ? status.present() : 0,
            status.valid() ? status.ready() : 0,
            status.valid() ? (int)status.clockSource() : -1);
}

Am18x5Configuration currentAm18x5Config() {
    Am18x5Configuration config;
    (void)Am18x5.getConfig(config);
    return config;
}

Am18x5Configuration buildAm18x5Config(bool withCalibration = false, int8_t calibration = 0) {
    auto current = currentAm18x5Config();
    Am18x5Configuration config;
    config.i2c(current.interface())
            .defaultTimeSource(current.defaultTimeSource())
            .sleepExtiCheck(current.sleepExtiCheck())
            .interruptPin(current.interruptPin())
            .watchdogPin(current.watchdogPin())
            .clockSource(current.clockSource())
            .capabilities(current.capabilities());
    if (withCalibration) {
        config.xtalCalibration(calibration);
    }
    return config;
}

int setAm18x5Config(bool withCalibration = false, int8_t calibration = 0) {
    auto config = buildAm18x5Config(withCalibration, calibration);
    const auto originalSleepExtiCheck = config.sleepExtiCheck();
    config.sleepExtiCheck(!originalSleepExtiCheck);
    int r = Am18x5.setConfig(config);
    if (r) {
        return r;
    }
    auto restore = buildAm18x5Config(withCalibration, calibration);
    restore.sleepExtiCheck(originalSleepExtiCheck);
    return Am18x5.setConfig(restore);
}

int withSystemCache(const std::function<int(particle::test::SystemCache&)>& fn) {
    particle::test::SystemCache cache;
    int r = cache.init();
    if (r) {
        return r;
    }
    SCOPE_GUARD({
        cache.deInit();
    });
    return fn(cache);
}

int copyFile(const char* src, const char* dst) {
    int in = ::open(src, O_RDONLY);
    if (in < 0) {
        return errno == ENOENT ? SYSTEM_ERROR_NOT_FOUND : SYSTEM_ERROR_IO;
    }
    SCOPE_GUARD({
        ::close(in);
    });

    int out = ::open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        return SYSTEM_ERROR_IO;
    }
    SCOPE_GUARD({
        ::close(out);
    });

    char buf[512];
    for (;;) {
        const ssize_t r = ::read(in, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return SYSTEM_ERROR_IO;
        }
        if (r == 0) {
            break;
        }
        ssize_t written = 0;
        while (written < r) {
            const ssize_t w = ::write(out, buf + written, r - written);
            if (w < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return SYSTEM_ERROR_IO;
            }
            written += w;
        }
    }

    if (::fsync(out) < 0) {
        return SYSTEM_ERROR_IO;
    }
    return SYSTEM_ERROR_NONE;
}

bool cacheKeyExists(particle::test::SystemCacheKey key) {
    return withSystemCache([&](particle::test::SystemCache& cache) {
        return cache.size(key) >= 0 ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_FOUND;
    }) == SYSTEM_ERROR_NONE;
}

int writeLegacyCalibrationIfMissing(ExRtcSuiteState& state, int8_t value) {
    if (cacheKeyExists(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG)) {
        return SYSTEM_ERROR_NONE;
    }
    LegacyAm18x5ManufacturingConfig legacy = {};
    legacy.version = 2;
    legacy.size = sizeof(legacy);
    legacy.mfg_magic = HAL_EXRTC_MFG_MAGIC;
    legacy.mfg_osc_cal_xt = value;
    int r = withSystemCache([&](particle::test::SystemCache& cache) {
        return cache.set(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG, &legacy, sizeof(legacy));
    });
    if (!r) {
        state.wroteLegacyCalibration = true;
        writeSuiteState(state);
    }
    return r;
}

int writeNewCalibrationIfMissing(ExRtcSuiteState& state, int8_t value) {
    if (cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION)) {
        return SYSTEM_ERROR_NONE;
    }
    hal_exrtc_calibration_data_t calibration = {};
    calibration.version = HAL_EXRTC_API_VERSION;
    calibration.size = sizeof(calibration);
    calibration.value = value;
    int r = withSystemCache([&](particle::test::SystemCache& cache) {
        return cache.set(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION, &calibration, sizeof(calibration));
    });
    if (!r) {
        state.wroteNewCalibration = true;
        writeSuiteState(state);
    }
    return r;
}

int deleteCacheKey(particle::test::SystemCacheKey key) {
    return withSystemCache([&](particle::test::SystemCache& cache) {
        int r = cache.del(key);
        return r == SYSTEM_ERROR_NOT_FOUND ? SYSTEM_ERROR_NONE : r;
    });
}

int readLegacyCalibrationValue(int8_t* value) {
    if (!value) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return withSystemCache([&](particle::test::SystemCache& cache) {
        LegacyAm18x5ManufacturingConfig legacy = {};
        int r = cache.get(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG, &legacy, sizeof(legacy));
        if (r < 0) {
            return r;
        }
        *value = legacy.mfg_osc_cal_xt;
        return 0;
    });
}

int readNewCalibrationValue(int8_t* value) {
    if (!value) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return withSystemCache([&](particle::test::SystemCache& cache) {
        hal_exrtc_calibration_data_t calibration = {};
        int r = cache.get(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION, &calibration, sizeof(calibration));
        if (r < 0) {
            return r;
        }
        *value = calibration.value;
        return 0;
    });
}

void dumpCalibrationCache(const char* label) {
    int8_t legacy = 0;
    int legacyRet = readLegacyCalibrationValue(&legacy);
    int8_t newer = 0;
    int newRet = readNewCalibrationValue(&newer);
    Test::out->printlnf("%s: legacy_ret=%d legacy=%d new_ret=%d new=%d",
            label, legacyRet, (int)legacy, newRet, (int)newer);
}

void assertExrtcConfigured() {
    const auto status = ExternalTime.status();
    assertTrue(status.valid());
    assertTrue(status.bound());
    assertTrue(status.present());
    assertTrue(status.ready());
#if !HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    assertTrue(status.builtIn());
#endif
}

void assertTimePreserved(time_t expected) {
    dumpTimeSources("assertTimePreserved");
    const auto systemTime = Time.now();
    const auto internalTime = InternalTime.now();
    const auto externalTime = ExternalTime.now();
    const auto source = Time.timeSource();

    assertLessOrEqual(absTimeDiff(internalTime, expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(externalTime, expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(internalTime, externalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    if (source == TimeSource::EXTERNAL) {
        assertLessOrEqual(absTimeDiff(systemTime, externalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    } else {
        assertEqual((int)source, (int)TimeSource::INTERNAL);
        assertLessOrEqual(absTimeDiff(systemTime, internalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    }
}

void assertTimeSourcesInSync() {
    dumpTimeSources("assertTimeSourcesInSync");
    const auto systemTime = Time.now();
    const auto internalTime = InternalTime.now();
    const auto externalTime = ExternalTime.now();
    const auto source = Time.timeSource();

    assertLessOrEqual(absTimeDiff(internalTime, externalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    if (source == TimeSource::EXTERNAL) {
        assertLessOrEqual(absTimeDiff(systemTime, externalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    } else {
        assertEqual((int)source, (int)TimeSource::INTERNAL);
        assertLessOrEqual(absTimeDiff(systemTime, internalTime), EXRTC_MAX_TIME_DRIFT_SECONDS);
    }
}

ExRtcSuiteState initSuiteState() {
    auto state = readSuiteState();
    if (state.magic == EXRTC_SUITE_STATE_MAGIC && state.initialized) {
#if !HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
        if (!state.shouldRun) {
            state.shouldRun = true;
            writeSuiteState(state);
        }
#endif
        return state;
    }

    state = {};
    state.magic = EXRTC_SUITE_STATE_MAGIC;
    state.initialized = true;

#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    state.isMuon = particle::test::detectMuonBoard();
    if (!state.isMuon) {
        state.shouldRun = false;
        writeSuiteState(state);
        return state;
    }
#else
    state.shouldRun = true;
#endif

#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    const auto status = ExternalTime.status();
    state.shouldRun = status.valid() && status.bound() && status.present() && status.ready();
#endif

    auto config = ExternalTime.getConfig();
    if (config.valid()) {
        state.defaultTimeSource = config.defaultTimeSource();
        state.clockSource = (uint8_t)config.clockSource();
        state.capabilities = config.capabilities().value();
    }
    writeSuiteState(state);
    return state;
}

ExRtcSuiteState loadSuiteState() {
    return initSuiteState();
}

RtcConfiguration baselineConfig(const ExRtcSuiteState& state) {
    RtcConfiguration config;
    config.defaultTimeSource(state.defaultTimeSource);
    config.clockSource((RtcClockSource)state.clockSource);
    config.capabilities(RtcCaps::fromUnderlying(state.capabilities));
    return config;
}

} // anonymous

#include "hex_to_bytes.h"

test(EXRTC_00_initialize_suite_state) {
    auto state = initSuiteState();
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (!state.isMuon) {
        skip();
        return;
    }
#endif

    const auto status = ExternalTime.status();
    assertTrue(status.valid());
    assertTrue(status.bound());
    assertTrue(status.present());
    assertTrue(status.ready());

    const auto config = ExternalTime.getConfig();
    assertTrue(config.valid());

    state.wroteLegacyCalibration = false;
    state.wroteNewCalibration = false;
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_NONE;
    state.expectedLegacyCalibration = 0;
    state.expectedNewCalibration = 0;
    state.legacyCalibrationExistedAtStart = cacheKeyExists(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG);
    state.newCalibrationExistedAtStart = cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION);
    int backupResult = copyFile(SYSTEM_CACHE_PATH, SYSTEM_CACHE_BACKUP_PATH);
    state.cacheBackupAvailable = backupResult == SYSTEM_ERROR_NONE;
    assertTrue(backupResult == SYSTEM_ERROR_NONE || backupResult == SYSTEM_ERROR_NOT_FOUND);
    writeSuiteState(state);

#if !HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    state.defaultTimeSource = config.defaultTimeSource();
    state.clockSource = (uint8_t)config.clockSource();
    state.capabilities = config.capabilities().value();
    writeSuiteState(state);
#endif
}

test(EXRTC_01_time_sources_are_in_sync_on_boot) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();
    assertTimeSourcesInSync();
}

test(EXRTC_02_id_is_non_empty) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    const auto id = ExternalTime.id();
    assertMore(id.length(), 0);
}

test(EXRTC_03_dump_system_cache) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();
    assertTimeSourcesInSync();

    particle::test::SystemCache cache;
    assertEqual(cache.init(), (int)SYSTEM_ERROR_NONE);

    SCOPE_GUARD({
        cache.deInit();
    });

    auto buf = std::make_unique<uint8_t[]>(4096);
    assertTrue((bool)buf);
    VariantMap cacheDump;

    constexpr particle::test::SystemCacheKey keys[] = {
        particle::test::SystemCacheKey::WIFI_NCP_FIRMWARE_VERSION,
        particle::test::SystemCacheKey::WIFI_NCP_MAC_ADDRESS,
        particle::test::SystemCacheKey::ADC_CALIBRATION_OFFSET,
        particle::test::SystemCacheKey::WIZNET_CONFIG_DATA,
        particle::test::SystemCacheKey::CELLULAR_NCP_OPERATION_MODE,
        particle::test::SystemCacheKey::CELLULAR_DEVICE_INFO,
        particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG,
        particle::test::SystemCacheKey::ASSET_MANAGER_CONSUMER_STATE,
        particle::test::SystemCacheKey::EXRTC_CONFIG_DATA,
        particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION
    };

    for (const auto key : keys) {
        const auto size = cache.size(key);
        if (size < 0) {
            continue;
        }
        assertLessOrEqual((size_t)size, 4096);
        assertEqual(cache.get(key, buf.get(), size), size);
        cacheDump.set(String::format("0x%04x", (unsigned)key), Buffer((const char*)buf.get(), size));
    }

    assertEqual(0, pushMailboxMsg(Variant(cacheDump).toJSON(), 5000));
}

test(EXRTC_04_enable_same_config_does_not_reset_or_lose_time) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    const auto expected = Time.now() + 60;
    assertEqual((int)Time.timeSource(), (int)TimeSource::EXTERNAL);
    Time.setTime(expected);
    assertLessOrEqual(absTimeDiff(Time.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(InternalTime.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(ExternalTime.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertTimePreserved(expected);

    auto config = ExternalTime.getConfig();
    assertTrue(config.valid());
    assertEqual(ExternalTime.enable(config), (int)SYSTEM_ERROR_NONE);

    dumpExrtcConfig("enableSameConfig");
    dumpTimeSources("enableSameConfig");
    assertExrtcConfigured();
    assertTimePreserved(expected);
}

test(EXRTC_05_set_config_same_config_does_not_reset_or_lose_time) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    const auto expected = Time.now() + 60;
    assertEqual((int)Time.timeSource(), (int)TimeSource::EXTERNAL);
    Time.setTime(expected);
    assertLessOrEqual(absTimeDiff(Time.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(InternalTime.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertLessOrEqual(absTimeDiff(ExternalTime.now(), expected), EXRTC_MAX_TIME_DRIFT_SECONDS);
    assertTimePreserved(expected);

    auto config = ExternalTime.getConfig();
    assertTrue(config.valid());
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);

    dumpExrtcConfig("setSameConfig");
    dumpTimeSources("setSameConfig");
    assertExrtcConfigured();
    assertTimePreserved(expected);
}

test(EXRTC_06_reconfigure_and_rebind_preserves_time_sync) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    const auto expected = Time.now() + 60;
    assertEqual((int)Time.timeSource(), (int)TimeSource::EXTERNAL);
    Time.setTime(expected);
    assertTimePreserved(expected);

    auto config = ExternalTime.getConfig();
    assertTrue(config.valid());
    const auto originalSleepExtiCheck = config.sleepExtiCheck();
    config.sleepExtiCheck(!originalSleepExtiCheck);
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);

    dumpExrtcConfig("reconfigureAndRebind");
    dumpTimeSources("reconfigureAndRebind");
    assertExrtcConfigured();
    assertTimePreserved(expected);

    auto restored = ExternalTime.getConfig();
    assertTrue(restored.valid());
    restored.sleepExtiCheck(originalSleepExtiCheck);
    assertEqual(ExternalTime.setConfig(restored), (int)SYSTEM_ERROR_NONE);

    dumpExrtcConfig("restoreAfterRebind");
    dumpTimeSources("restoreAfterRebind");
    assertExrtcConfigured();
    assertTimePreserved(expected);
}

test(EXRTC_07_update_configuration) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    auto config = ExternalTime.getConfig();
    assertTrue(config.valid());
    const auto wasDefault = config.defaultTimeSource();
    assertEqual((int)Time.timeSource(), wasDefault ? (int)TimeSource::EXTERNAL : (int)TimeSource::INTERNAL);

    config.defaultTimeSource(!wasDefault);
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);

    auto updated = ExternalTime.getConfig();
    assertTrue(updated.valid());
    assertEqual(updated.defaultTimeSource(), !wasDefault);
    assertEqual((int)Time.timeSource(), wasDefault ? (int)TimeSource::INTERNAL : (int)TimeSource::EXTERNAL);

    updated.defaultTimeSource(wasDefault);
    assertEqual(ExternalTime.setConfig(updated), (int)SYSTEM_ERROR_NONE);

    auto restored = ExternalTime.getConfig();
    assertTrue(restored.valid());
    assertEqual(restored.defaultTimeSource(), wasDefault);
    assertEqual((int)Time.timeSource(), wasDefault ? (int)TimeSource::EXTERNAL : (int)TimeSource::INTERNAL);
}

test(EXRTC_08_power_off_should_fail_without_internal_fallback) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    auto original = ExternalTime.getConfig();
    assertTrue(original.valid());

    auto config = original;
    config.clockSource(RtcClockSource::EXTERNAL);
    config.capabilities(config.capabilities() & ~RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);
    dumpExrtcConfig("powerOffShouldFail");

    auto result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::POWER_OFF).duration(10s));
    Test::out->printlnf("powerOffShouldFail: sleep error=%d", result.error());
    assertNotEqual(result.error(), (int)SYSTEM_ERROR_NONE);

    assertEqual(ExternalTime.setConfig(original), (int)SYSTEM_ERROR_NONE);
}

test(EXRTC_09_power_off_should_succeed_1) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    auto config = ExternalTime.getConfig();
    assertTrue(config.valid());
    config.clockSource(RtcClockSource::EXTERNAL);
    config.capabilities(config.capabilities() | RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);
    dumpExrtcConfig("powerOffShouldSucceed");

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    const auto result = System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::POWER_OFF).duration(10s));
    Test::out->printlnf("powerOffShouldSucceed: sleep error=%d", result.error());
    assertEqual(result.error(), (int)SYSTEM_ERROR_NONE);
}

test(EXRTC_09_power_off_should_succeed_2) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertEqual(System.resetReason(), (int)RESET_REASON_POWER_DOWN);
    assertExrtcConfigured();
}

test(EXRTC_10_prepare_legacy_cache_calibration_and_reset) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    if (cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION)) {
        skip();
        return;
    }
    assertEqual(writeLegacyCalibrationIfMissing(state, LEGACY_CACHE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    dumpCalibrationCache("prepareLegacyCalibration");
    int8_t legacy = 0;
    assertEqual(readLegacyCalibrationValue(&legacy), (int)SYSTEM_ERROR_NONE);
    state.expectedLegacyCalibration = legacy;
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_LEGACY;
    writeSuiteState(state);
    expectSystemReset();
    System.reset();
}

test(EXRTC_11_legacy_cache_calibration_is_used_when_no_override_is_set) {
    auto state = loadSuiteState();
    if (!state.shouldRun || state.pendingCalibrationScenario != CALIBRATION_SCENARIO_LEGACY) {
        skip();
        return;
    }
    assertExrtcConfigured();
    dumpCalibrationCache("legacyCalibrationAfterReset");
    int8_t legacy = 0;
    assertEqual(readLegacyCalibrationValue(&legacy), (int)SYSTEM_ERROR_NONE);
    assertEqual((int)legacy, (int)state.expectedLegacyCalibration);
    auto config = currentAm18x5Config();
    assertFalse(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), 0);
    assertEqual(ExternalTime.status().xtalCalibration(), (int)state.expectedLegacyCalibration);
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_NONE;
    writeSuiteState(state);
}

test(EXRTC_12_config_calibration_overrides_legacy_cache_value) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    if (cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION)) {
        skip();
        return;
    }
    assertEqual(writeLegacyCalibrationIfMissing(state, LEGACY_CACHE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    assertEqual(setAm18x5Config(true, LEGACY_OVERRIDE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    assertExrtcConfigured();
    auto config = currentAm18x5Config();
    assertTrue(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), (int)LEGACY_OVERRIDE_TEST_CALIBRATION);
    assertEqual(ExternalTime.status().xtalCalibration(), (int)LEGACY_OVERRIDE_TEST_CALIBRATION);
}

test(EXRTC_13_prepare_new_cache_calibration_and_reset) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertEqual(setAm18x5Config(false), (int)SYSTEM_ERROR_NONE);
    auto config = currentAm18x5Config();
    assertFalse(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), 0);
    assertEqual(writeNewCalibrationIfMissing(state, NEW_CACHE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    dumpCalibrationCache("prepareNewCalibration");
    int8_t newer = 0;
    assertEqual(readNewCalibrationValue(&newer), (int)SYSTEM_ERROR_NONE);
    state.expectedNewCalibration = newer;
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_NEW;
    writeSuiteState(state);
    expectSystemReset();
    System.reset();
}

test(EXRTC_14_new_cache_calibration_is_used_when_no_override_is_set) {
    auto state = loadSuiteState();
    if (!state.shouldRun || state.pendingCalibrationScenario != CALIBRATION_SCENARIO_NEW) {
        skip();
        return;
    }
    assertExrtcConfigured();
    dumpCalibrationCache("newCalibrationAfterReset");
    int8_t newer = 0;
    assertEqual(readNewCalibrationValue(&newer), (int)SYSTEM_ERROR_NONE);
    assertEqual((int)newer, (int)state.expectedNewCalibration);
    auto config = currentAm18x5Config();
    assertFalse(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), 0);
    assertEqual(ExternalTime.status().xtalCalibration(), (int)state.expectedNewCalibration);
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_NONE;
    writeSuiteState(state);
}

test(EXRTC_15_config_calibration_overrides_new_cache_value) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertEqual(writeNewCalibrationIfMissing(state, NEW_CACHE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    assertEqual(setAm18x5Config(true, NEW_OVERRIDE_TEST_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    assertExrtcConfigured();
    auto config = currentAm18x5Config();
    assertTrue(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), (int)NEW_OVERRIDE_TEST_CALIBRATION);
    assertEqual(ExternalTime.status().xtalCalibration(), (int)NEW_OVERRIDE_TEST_CALIBRATION);
}

test(EXRTC_16_prepare_default_calibration_case_and_reset) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertEqual(setAm18x5Config(false), (int)SYSTEM_ERROR_NONE);
    auto config = currentAm18x5Config();
    assertFalse(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), 0);

    const bool legacyExists = cacheKeyExists(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG);
    const bool newExists = cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION);
    const bool canDeleteLegacy = !legacyExists || state.wroteLegacyCalibration;
    const bool canDeleteNew = !newExists || state.wroteNewCalibration;
    if (!canDeleteLegacy || !canDeleteNew) {
        skip();
        return;
    }

    assertEqual(deleteCacheKey(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG), (int)SYSTEM_ERROR_NONE);
    assertEqual(deleteCacheKey(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    dumpCalibrationCache("prepareDefaultCalibration");
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_DEFAULT;
    writeSuiteState(state);
    expectSystemReset();
    System.reset();
}

test(EXRTC_17_default_calibration_is_used_when_no_cache_or_override_exists) {
    auto state = loadSuiteState();
    if (!state.shouldRun || state.pendingCalibrationScenario != CALIBRATION_SCENARIO_DEFAULT) {
        skip();
        return;
    }
    assertExrtcConfigured();
    dumpCalibrationCache("defaultCalibrationAfterReset");
    int8_t legacy = 0;
    int8_t newer = 0;
    assertEqual(readLegacyCalibrationValue(&legacy), (int)SYSTEM_ERROR_NOT_FOUND);
    assertEqual(readNewCalibrationValue(&newer), (int)SYSTEM_ERROR_NOT_FOUND);

    auto config = currentAm18x5Config();
    assertFalse(config.xtalCalibrationSet());
    assertEqual(config.xtalCalibration(), 0);
    assertEqual(ExternalTime.status().xtalCalibration(), (int)HAL_PLATFORM_EXTERNAL_RTC_CAL_XT);
    state.pendingCalibrationScenario = CALIBRATION_SCENARIO_NONE;
    writeSuiteState(state);
}

test(EXRTC_99_restore_default_configuration) {
    auto state = loadSuiteState();
    if (!state.shouldRun) {
        skip();
        return;
    }
    assertExrtcConfigured();

    if (state.wroteNewCalibration) {
        assertEqual(deleteCacheKey(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION), (int)SYSTEM_ERROR_NONE);
    }
    if (state.wroteLegacyCalibration) {
        assertEqual(deleteCacheKey(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG), (int)SYSTEM_ERROR_NONE);
    }

    if (state.legacyCalibrationExistedAtStart) {
        assertTrue(cacheKeyExists(particle::test::SystemCacheKey::AM18X5_MANUFACTURING_CONFIG));
    }
    if (state.newCalibrationExistedAtStart) {
        assertTrue(cacheKeyExists(particle::test::SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION));
    }

#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (state.isMuon) {
        assertEqual(particle::test::configureMuonExrtc(), (int)SYSTEM_ERROR_NONE);
    }
#else
    auto config = baselineConfig(state);
    assertEqual(ExternalTime.setConfig(config), (int)SYSTEM_ERROR_NONE);
#endif

    assertExrtcConfigured();

    auto restored = ExternalTime.getConfig();
    assertTrue(restored.valid());
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    assertTrue(restored.defaultTimeSource());
    assertEqual((int)restored.clockSource(), (int)RtcClockSource::EXTERNAL);
    assertTrue((restored.capabilities() & RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL) == RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
#else
    auto baseline = baselineConfig(state);
    assertEqual(restored.defaultTimeSource(), baseline.defaultTimeSource());
    assertEqual((int)restored.clockSource(), (int)baseline.clockSource());
    assertEqual(restored.capabilities().value(), baseline.capabilities().value());
#endif

    state.initialized = false;
    state.shouldRun = false;
    writeSuiteState(state);

    if (state.cacheBackupAvailable) {
        assertEqual(copyFile(SYSTEM_CACHE_BACKUP_PATH, SYSTEM_CACHE_PATH), (int)SYSTEM_ERROR_NONE);
        ::unlink(SYSTEM_CACHE_BACKUP_PATH);
        expectSystemReset();
        System.reset();
    }
}

#else

test(EXRTC_00_not_supported) {
    assertTrue(true);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
