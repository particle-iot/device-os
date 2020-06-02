#include "Particle.h"

SYSTEM_MODE(MANUAL);

SYSTEM_THREAD(ENABLED);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

void setup() {
    LOG(TRACE, "Application started.");

    pinMode(A1, INPUT_PULLUP);

    Cellular.on();
}

void loop() {
    if (digitalRead(A1) == 0) {
        LOG(TRACE, "Device is going to enter sleep mode.");
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::STOP).gpio(A0, RISING);
        System.sleep(config);

        Cellular.on();
    }
}
