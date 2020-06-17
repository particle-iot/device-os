#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

namespace {

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

const auto DELAY = 10000;

} // namespace

void setup() {
    waitUntil(Serial.isConnected);

    Log.info("========================================");

    Log.info("Connecting to the cloud");
    Particle.connect();
}

void loop() {
    if (Particle.connected()) {
        Log.info("Connected to the cloud");

        Log.info("Waiting %d seconds...", DELAY / 1000);
        delay(DELAY);

        Log.info("Publishing an event");
        Particle.publish("event", PRIVATE);

        Log.info("Disconnecting from the cloud");
        Particle.disconnect(CloudDisconnectOptions().graceful(true));
        waitUntil(Particle.disconnected);
        Log.info("Disconnected from the cloud");

        Log.info("Waiting %d seconds...", DELAY / 1000);
        delay(DELAY);

        Log.info("Connecting to the cloud");
        Particle.connect();
    }
}
