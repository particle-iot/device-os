#include "application.h"

#define MIN_CONNECT_DURATION 1000 // Milliseconds
#define MAX_CONNECT_DURATION 5000
#define SLEEP_DURATION 5 // Seconds

SYSTEM_MODE(MANUAL)
SYSTEM_THREAD(ENABLED)

namespace {

SerialLogHandler logHandler(LOG_LEVEL_WARN, { // Default logging level
    { "app", LOG_LEVEL_ALL } // Logging level for application messages
});

uint32_t t = 0, d = 0;

void resetTimer() {
    d = rand() % (MAX_CONNECT_DURATION - MIN_CONNECT_DURATION) + MIN_CONNECT_DURATION;
    t = millis();
}

} // namespace

void setup() {
    // Set incorrect WiFi credentials to cause repeated connection attempts
    WiFi.on();
    WiFi.clearCredentials();
    WiFi.setCredentials("param-pam-pam");
    WiFi.connect();
    resetTimer();
}

void loop() {
    if (millis() - t >= d) {
        LOG(INFO, "Entering stop mode");
        // The current taken by the device should be around 3 mA when the stop mode is active
        System.sleep(D1, RISING, SLEEP_DURATION);
        LOG(INFO, "Leaving stop mode");
        resetTimer();
    }
}
