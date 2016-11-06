#include "application.h"
#include "unit-test/unit-test.h"

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;

SYSTEM_THREAD(ENABLED);

//Serial1LogHandler dbg(115200, LOG_LEVEL_ALL);

/*
 * Tests for issue #1043
 * This test will do a couple of publishes and  enter deep sleep for a short time (which will reset the device).
 * After the reset, the tests should be restarted again.
 */
test(sleep_0_device_wakes_from_deep_sleep_with_short_sleep_time)
{
    if (magick == 0xdeadbeef) {
        magick = 0;
        // We should have woken up from deep sleep
        assertEqual(System.resetReason(), (int)RESET_REASON_POWER_MANAGEMENT);
    } else {
        // Set magic key in retained memory to indicate that we just went into deep sleep
        magick = 0xdeadbeef;
        for  (int i = 0; i < 10; i++) {
            Particle.publish("test", "teststring");
            delay(1100);
        }

        // Do a couple of publishes
        System.sleep(SLEEP_MODE_DEEP, 1);
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
    // Just in case
    magick = 0;

    uint16_t pin = D1;
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    uint16_t exti = PIN_MAP[pin].gpio_pin;
    volatile bool cont = false;

    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    pinMode(pin, INPUT);
    // Configure D1, attach interrupt handler
    attachInterrupt(pin, [&] {
        cont = true;
    }, FALLING);

    // Check that interrupt handler fires
    cont = false;
    EXTI_GenerateSWInterrupt(exti);
    uint32_t s = millis();
    while (!cont && (millis() - s) < 1000);
    assertEqual(static_cast<bool>(cont), true);

    // Sleep for 5 seconds
    System.sleep(pin, FALLING, 5, SLEEP_NETWORK_STANDBY);
    // Ideally there should be a test here checking that the interrupt triggered while sleeping
    // was immediately serviced

    digitalWrite(D7, HIGH);

    // Wait for Host to connect back
    while (!Serial.isConnected());

    // Check that interrupt handler fires
    cont = false;
    EXTI_GenerateSWInterrupt(exti);
    s = millis();
    while (!cont && (millis() - s) < 1000);
    assertEqual(static_cast<bool>(cont), true);

    detachInterrupt(pin);
}

#if PLATFORM_ID==PLATFORM_ELECTRON_PRODUCTION
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
test(sleep_2_electron_all_confirmable_messages_are_sent_before_sleep_step_1)
{
    //Serial.println("Disconnecting...");
    Particle.disconnect();
    waitFor(Particle.disconnected, 60000);
    assertTrue(Particle.disconnected());
    //Serial.println("Disconnected");

    Cellular.off();

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

    Cellular.on();
    waitFor(Cellular.ready, 60000);

    // Connect
    //Serial.println("Connecting...");
    Particle.connect();
    waitFor(Particle.connected, 5 * 60 * 1000);
    assertTrue(Particle.connected());
    //Serial.println("Connected");

    System.on(cloud_status, onCloudStatus);

    // Repeat 10 times
    for (int i = 0; i < 10; i++) {
        assertTrue(Particle.connected());
        sprintf(tmp, "%d/10", i + 1);
        assertTrue(Particle.publish("sleeping", tmp, 60, PRIVATE));
        uint32_t ts = millis();
        System.sleep(65535, RISING, 30, SLEEP_NETWORK_STANDBY);
        assertTrue(Particle.connected());
        assertLessOrEqual(s_cloud_disconnected, ts);
    }
}

test(sleep_2_electron_all_confirmable_messages_are_sent_before_sleep_step_2) {
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);
    waitUntil(Serial.isConnected);
}

test(sleep_3_restore_system_mode) {
    set_system_mode(AUTOMATIC);
}
#endif // PLATFORM_ID==PLATFORM_ELECTRON_PRODUCTION