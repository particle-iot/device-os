#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(DISABLED);

LOG_SOURCE_CATEGORY("test")

namespace {

SerialLogHandler logHandler(LOG_LEVEL_NONE, {
    { "test", LOG_LEVEL_ALL }
});

static bool loopExecuted = false;
static bool testFinished = false;

static void fail() {
    testFinished = true;
    RGB.control(true);
    RGB.color(0xff0000); // Red
    LOG(ERROR, "TEST FAILED");
}

static void pass() {
    testFinished = true;
    RGB.control(true);
    RGB.color(0x00ff00); // Green
    LOG(INFO, "TEST SUCCEEDED");
}

} // namespace

void setup() {
    waitUntil(Serial.isConnected);

    LOG(INFO, "Test started");
    LOG(INFO, "Connecting to WiFi network");
    WiFi.on();
    WiFi.connect();
    waitUntil(WiFi.ready);
    LOG(INFO, "Connected to WiFi network");

    LOG(INFO, "Connecting to the cloud");
    Particle.connect();
    if (Particle.connected()) {
        fail();
    }
}

void loop() {
    if (testFinished) {
        return;
    }

    if (!loopExecuted) {
        loopExecuted = true;

        // First time the loop is running. We should be connected to the cloud
        if (!Particle.connected()) {
            fail();
            return;
        }

        LOG(INFO, "Connected to the cloud");

        // Disconnect from the cloud
        LOG(INFO, "Disconnecting from the cloud");
        Particle.disconnect();
        waitUntil(Particle.disconnected);
        LOG(INFO, "Disconnected from the cloud");

        LOG(INFO, "Connecting to the cloud");
        Particle.connect();
    } else {
        // Second time the loop is running, we should also be connected to the cloud again
        if (!Particle.connected()) {
            fail();
            return;
        }
        LOG(INFO, "Connected to the cloud");

        pass();
    }
}
