#include "application.h"
#include "unit-test/unit-test.h"

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
static retained uint32_t magick = 0;

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
