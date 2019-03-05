#include "application.h"
#include "unit-test/unit-test.h"

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;

SYSTEM_THREAD(ENABLED);

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

/*
 * Tests for issue #1043
 * This test will do a couple of publishes and  enter deep sleep for a short time (which will reset the device).
 * After the reset, the tests should be restarted again.
 */
test(sleep_0_device_wakes_from_deep_sleep_with_short_sleep_time)
{
    waitFor(Particle.connected, 5 * 60 * 1000);
    if (magick == 0xdeadbeef) {
        magick = 0;
        // We should have woken up from deep sleep
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);

        /**
         * Issue #1447 "WKP/A7 ignores pinMode after waking from deep sleep"
         * A7/WKP pin must be jumpered to D6 because
         * digitalWrite(A7) will be equal to digitalRead(A7) despite this issue.
         */
        Serial.println("SETUP: Make sure to jumper A7/WKP pin to D6 pin.");
        pinMode(A7, OUTPUT);
        pinMode(D6, INPUT);
        digitalWrite(A7, HIGH);
        delay(1);
        assertEqual((int)digitalRead(D6), (int)HIGH);
        digitalWrite(A7, LOW);
        delay(1);
        assertEqual((int)digitalRead(D6), (int)LOW);
        // Issue #1447
    } else {
        Serial.println("Publishing 10 messages, see them at 'particle subscribe mine'");
        // Set magic key in retained memory to indicate that we just went into deep sleep
        magick = 0xdeadbeef;
        for  (int i = 0; i < 10; i++) {
            Particle.publish("test", String("teststring ") + String(i+1));
            delay(1100);
        }
        Serial.println("deep sleeping for 5 seconds, please reconnect serial and run tests again!");

        // Do a couple of publishes
        auto r = System.sleep(SLEEP_MODE_DEEP, 5);
        if (r.error() == SYSTEM_ERROR_NOT_SUPPORTED) {
            // Most likely platform doesn't wake up by RTC, instruct the user to manually wake
            // the device with WKP pin
            Serial.println("Failed to sleep. Platform probably doesn't support wakeup by RTC");
            Serial.println("Going to sleep with wakeup by WKP pin only in 5 seconds");
            Serial.println("Please wake up the device manully");
            delay(5000);
            System.sleep(SLEEP_MODE_DEEP, 0);
        }
    }
}

/*
 * Tests for issue #1029
 * This test will enter STOP mode for 5 seconds, which will cause USB Serial connection to close.
 * USB Serial port should be opened again for the test to progress after waking from STOP mode.
 * The test will wait for user to open USB Serial session.
 */
test(sleep_1_interrupts_attached_handler_is_not_detached_after_stop_mode)
{
    Serial.println("Please make sure D1 is connected to D0");
    // Just in case
    magick = 0;

    uint16_t pin = D1;
    volatile bool cont = false;

    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    pinMode(D0, OUTPUT);
    digitalWrite(D0, HIGH);
    delay(1);

    pinMode(pin, INPUT);
    // Configure D1, attach interrupt handler
    attachInterrupt(pin, [&] {
        cont = true;
    }, FALLING);

    // Check that interrupt handler fires
    cont = false;
    digitalWrite(D0, LOW);
    uint32_t s = millis();
    while (!cont && (millis() - s) < 1000);
    assertEqual(static_cast<bool>(cont), true);
    digitalWrite(D0, HIGH);

    Serial.println("sleeping in stop mode for 5 seconds, please reconnect serial!");
    delay(100);
    // Sleep for 5 seconds
    System.sleep(pin, FALLING, 5, SLEEP_NETWORK_STANDBY);
    // Ideally there should be a test here checking that the interrupt triggered while sleeping
    // was immediately serviced

    digitalWrite(D7, HIGH);

    // Wait for Host to connect back
    while (!Serial.isConnected());
    Serial.println("welcome back!");

    // Check that interrupt handler fires
    cont = false;
    digitalWrite(D0, LOW);
    s = millis();
    while (!cont && (millis() - s) < 1000);
    assertEqual(static_cast<bool>(cont), true);

    detachInterrupt(pin);
    pinMode(D0, INPUT);
}

static bool NotReady() {
    return !Network.ready();
}

// This test will not work at the moment on Mesh-only devices
#if HAL_PLATFORM_CLOUD_UDP && (HAL_PLATFORM_CELLULAR || HAL_PLATFORM_WIFI)
static int testfunc(String s) {
    return 1337;
}

static uint32_t s_cloud_disconnected = 0;

static void onCloudStatus(system_event_t ev, int param) {
    switch (param) {
        case cloud_status_disconnected:
            s_cloud_disconnected = millis();
            break;
        default:
            break;
    }
}

/*
 * Tests for issue #1133
 * NOTE: This test will take approximately 5 minutes to finish
 */
test(sleep_2_udp_all_confirmable_messages_are_sent_before_sleep_step_1)
{
    //Serial.println("Disconnecting...");
    Particle.disconnect();
    waitFor(Particle.disconnected, 60000);
    assertTrue(Particle.disconnected());
    //Serial.println("Disconnected");

    Network.off();
    waitFor(NotReady, 60000);

    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    // Switch to MANUAL mode
    //Serial.println("Switching to MANUAL");
    set_system_mode(MANUAL);
    //Serial.println("Switched");

    // Register a function with a random name just in case
    // so that session resume will definitely not happen
    randomSeed((uint32_t)Time.now());
    int r = random(0, 99999999);
    char tmp[13] = {0};
    sprintf(tmp, "f%d", r);
    //Serial.println("Registering random function");
    Particle.function(tmp, testfunc);
    //Serial.println("Registered");

    Network.on();
    Network.connect();
    waitFor(Network.ready, 60000);

    // Connect
    //Serial.println("Connecting...");
    Particle.connect();
    waitFor(Particle.connected, 5 * 60 * 1000);
    assertTrue(Particle.connected());
    //Serial.println("Connected");

    System.on(cloud_status, onCloudStatus);

    Serial.println("Publishing 10 messages over next 2.5 minutes, see them at 'particle subscribe mine'");
    // Repeat 10 times
    for (int i = 0; i < 10; i++) {
        assertTrue(Particle.connected());
        sprintf(tmp, "%d/10", i + 1);
        if (i == 9) {
            assertTrue(Particle.publish("please reconnect serial in 15 seconds!", tmp, 60, PRIVATE));
        } else {
            assertTrue(Particle.publish("sleeping", tmp, 60, PRIVATE));
        }
        uint32_t ts = millis();
        // RTC wakeup only
        System.sleep((pin_t*)nullptr, 0, RISING, 15, SLEEP_NETWORK_STANDBY);
        assertTrue(Particle.connected());
        assertLessOrEqual(s_cloud_disconnected, ts);
    }
}

test(sleep_2_udp_all_confirmable_messages_are_sent_before_sleep_step_2) {
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);
    waitUntil(Serial.isConnected);
}

test(sleep_3_restore_system_mode) {
    set_system_mode(AUTOMATIC);
}
#endif // HAL_PLATFORM_CLOUD_UDP

/*
 * Issue #1155, broken by PR #1051/#1076
 */
test(sleep_4_system_sleep_sleep_mode_wlan_works_correctly)
{
    Serial.println("sleeping network for 10 seconds, will reconnect to Cloud shortly!");
    System.sleep(10);
    waitFor(Particle.disconnected, 60000);
    waitFor(NotReady, 60000);
    waitFor(Particle.connected, 120000);
    assertTrue(Particle.connected());
}
