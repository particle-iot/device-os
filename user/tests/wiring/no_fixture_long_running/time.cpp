#include "application.h"
#include "unit-test/unit-test.h"

namespace {

constexpr int TIME_SOURCE_MAX_DRIFT_SECONDS = 10;

int64_t absTimeDiff(time_t lhs, time_t rhs) {
    return lhs >= rhs ? (int64_t)lhs - (int64_t)rhs : (int64_t)rhs - (int64_t)lhs;
}

bool shouldCheckExternalTime() {
#if HAL_PLATFORM_EXTERNAL_RTC
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    auto status = ExternalTime.status();
    return status.valid() && status.bound() && status.present() && status.ready();
#else
    return true;
#endif
#else
    return false;
#endif
}

void assertTimeSourcesAreSane() {
    const auto systemTime = Time.now();
    const auto internalTime = InternalTime.now();
    assertLessOrEqual(absTimeDiff(systemTime, internalTime), TIME_SOURCE_MAX_DRIFT_SECONDS);
    assertEqual(InternalTime.isValid(), Time.isValid());
    if (shouldCheckExternalTime()) {
#if HAL_PLATFORM_EXTERNAL_RTC
        const auto externalTime = ExternalTime.now();
        assertLessOrEqual(absTimeDiff(systemTime, externalTime), TIME_SOURCE_MAX_DRIFT_SECONDS);
        assertLessOrEqual(absTimeDiff(internalTime, externalTime), TIME_SOURCE_MAX_DRIFT_SECONDS);
        assertEqual(ExternalTime.isValid(), Time.isValid());
#endif // HAL_PLATFORM_EXTERNAL_RTC
    }
}

} // namespace

test(TIME_01_SyncTimeInAutomaticMode) {
    set_system_mode(AUTOMATIC);
    assertEqual(System.mode(),AUTOMATIC);
    Particle.connect();
    waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 120000);
    assertTrue(Particle.syncTimeDone());
    delay(4000);

    for(int x=0; x<2; x++) {
        time_t syncedLastUnix, syncedCurrentUnix;
        system_tick_t syncedCurrentMillis;
        system_tick_t syncedLastMillis = Particle.timeSyncedLast(syncedLastUnix);
        // 2018/01/01 00:00:00
        Time.setTime(1514764800);
        assertLessOrEqual(Time.now(), 1514764800 + 60);
        Particle.disconnect();
        waitFor(Particle.disconnected, 120000);
        assertTrue(Particle.disconnected());
        // set_system_mode(AUTOMATIC);
        // assertEqual(System.mode(),AUTOMATIC);
        delay(20000);

        Particle.connect();
        waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
        assertTrue(Particle.connected());
        // Just in case send sync time request (Device might not send it after handshake if the session was resumed)
        if (!Particle.syncTimePending()) {
            Particle.syncTime();
        }
        waitFor(Particle.syncTimeDone, 120000);
        assertTrue(Particle.syncTimeDone());

        assertTrue(Time.isValid());
        assertMore(Time.year(), 2018);
        syncedCurrentMillis = Particle.timeSyncedLast(syncedCurrentUnix);
        // Serial.printlnf("sCU-sLU: %d, sCM-sLM: %d",
        //     syncedCurrentUnix-syncedLastUnix, syncedCurrentMillis-syncedLastMillis);
        assertMore(syncedCurrentMillis, syncedLastMillis);
        assertMore(syncedCurrentUnix, syncedLastUnix);
        assertTimeSourcesAreSane();
    } // for()
}

test(TIME_02_SyncTimeInManualMode) {
    for(int x=0; x<2; x++) {
        time_t syncedLastUnix, syncedCurrentUnix;
        system_tick_t syncedCurrentMillis;
        system_tick_t syncedLastMillis = Particle.timeSyncedLast(syncedLastUnix);
        // 2018/01/01 00:00:00
        Time.setTime(1514764800);
        assertLessOrEqual(Time.now(), 1514764800 + 60);
        Particle.disconnect();
        waitFor(Particle.disconnected, 120000);
        assertTrue(Particle.disconnected());
        set_system_mode(MANUAL);
        assertEqual(System.mode(),MANUAL);
        delay(20000);

        // Serial.println("CONNECT");
        Particle.connect();
        waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
        assertTrue(Particle.connected());

        // Just in case send sync time request (Device might not send it after handshake if the session was resumed)
        // Serial.println("SYNC TIME");
        if (!Particle.syncTimePending()) {
            Particle.syncTime();
        }
        waitFor(Particle.syncTimeDone, 120000);
        assertTrue(Particle.syncTimeDone());
        assertTrue(Time.isValid());
        assertMore(Time.year(), 2018);
        syncedCurrentMillis = Particle.timeSyncedLast(syncedCurrentUnix);
        // Serial.printlnf("sCU-sLU: %d, sCM-sLM: %d",
        //     syncedCurrentUnix-syncedLastUnix, syncedCurrentMillis-syncedLastMillis);
        assertMore(syncedCurrentMillis, syncedLastMillis);
        assertMore(syncedCurrentUnix, syncedLastUnix);
        assertTimeSourcesAreSane();
    } // for()
}
