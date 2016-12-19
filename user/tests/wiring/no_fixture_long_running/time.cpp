#if PLATFORM_ID>=3

#include "application.h"
#include "unit-test/unit-test.h"

test(TIME_01_SyncTimeInAutomaticMode) {
    set_system_mode(AUTOMATIC);
    assertEqual(System.mode(),AUTOMATIC);
    Particle.connect();
    waitFor(Particle.connected, 120000);

    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 10000);
    delay(4000);

    for(int x=0; x<2; x++) {
    time_t syncedLastUnix, syncedCurrentUnix;
    system_tick_t syncedCurrentMillis;
    system_tick_t syncedLastMillis = Particle.timeSyncedLast(syncedLastUnix);
    // Invalid time (year = 00) 2000/01/01 00:00:00
    Time.setTime(946684800);
    // assertFalse(Time.isValid());
    Particle.disconnect();
    waitFor(Particle.disconnected, 60000);
    // set_system_mode(AUTOMATIC);
    // assertEqual(System.mode(),AUTOMATIC);
    delay(20000);

    Particle.connect();
    waitFor(Particle.connected, 120000);
    // Just in case send sync time request (Electron might not send it after handshake if the session was resumed)
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 60000);

    assertTrue(Time.isValid());
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
    // Invalid time (year = 00) 2000/01/01 00:00:00
    Time.setTime(946684800);
    // assertFalse(Time.isValid());
    Particle.disconnect();
    waitFor(Particle.disconnected, 120000);
    set_system_mode(MANUAL);
    assertEqual(System.mode(),MANUAL);
    delay(20000);

    Particle.connect();
    waitFor(Particle.connected, 60000);
    // Just in case send sync time request (Electron might not send it after handshake if the session was resumed)
    Particle.syncTime();
    if (!Time.isValid()) {
        waitFor(Particle.syncTimeDone, 120000);
    }
    assertTrue(Time.isValid());
    syncedCurrentMillis = Particle.timeSyncedLast(syncedCurrentUnix);
    // Serial.printlnf("sCU-sLU: %d, sCM-sLM: %d",
    //     syncedCurrentUnix-syncedLastUnix, syncedCurrentMillis-syncedLastMillis);
    assertMore(syncedCurrentMillis, syncedLastMillis);
    assertMore(syncedCurrentUnix, syncedLastUnix);
    } // for()
}

#endif