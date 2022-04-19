#include "application.h"
#include "unit-test/unit-test.h"

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
    } // for()
}
