#include "application.h"

SYSTEM_MODE(AUTOMATIC)

namespace {

retained unsigned counter = 0;
retained uint32_t magic = 0;

void cloudStatusEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case cloud_status_connecting: {
        Log.info("Connecting to the cloud");
        break;
    }
    case cloud_status_connected: {
        Log.info("Connected to the cloud");
        break;
    }
    default:
        break;
    }
}

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "comm.protocol", LOG_LEVEL_ALL },
    { "app", LOG_LEVEL_ALL }
});

String cloudVar = "hello";

int cloudFunc(const String& arg) {
    Log.info("Cloud function called");
    return 0;
}

void cloudEvent(const char* name, const char* data) {
    Log.info("Cloud event received");
}

} // namespace

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY))
STARTUP(System.on(cloud_status, cloudStatusEvent))

void setup() {
    if (magic != 0x89abcdef || System.resetReason() == RESET_REASON_PIN_RESET) {
        counter = 0;
        magic = 0x89abcdef;
    }

    waitUntil(Serial.isConnected);

    Log.info("========================================");

    if (counter == 3) {
        Log.info("Not registering any functions or variables");
    } else {
        if (counter >= 0) {
            Particle.variable("var", cloudVar);
            Log.info("Registered cloud variable");
        }
        if (counter >= 1) {
            Particle.function("func", cloudFunc);
            Log.info("Registered cloud function");
        }
        if (counter >= 2) {
            Particle.subscribe("event", cloudEvent, MY_DEVICES);
            Log.info("Subscribed to events");
        }
    }

    counter = (counter + 1) % 4;
}

void loop() {
}
