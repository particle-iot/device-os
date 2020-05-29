#include "Particle.h"

SYSTEM_MODE(MANUAL);

SYSTEM_THREAD(ENABLED);

Serial1LogHandler log(LOG_LEVEL_ALL);

void setup() {
    // Serial1.begin(9600);

    LOG(TRACE, "Application started.");

    LOG(TRACE, "Call Cellular.on()");
    Cellular.on();
    LOG(TRACE, "After Cellular.on()");

    delay(5000);
}

void loop() {
    LOG(TRACE, "Device is going to enter sleep mode.");
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP).gpio(D0, RISING);
    System.sleep(config);
}