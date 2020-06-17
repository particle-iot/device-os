#include "application.h"

#if USE_SEMI_AUTOMATIC
SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)
#else
SYSTEM_MODE(AUTOMATIC)
SYSTEM_THREAD(DISABLED)
#endif

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
    { "comm.protocol", LOG_LEVEL_INFO },
    { "app", LOG_LEVEL_ALL }
});

String cloudVar = "This is a test variable";

int cloudFunc(const String& arg) {
    Log.info("Cloud function called");
    return 0;
}

void cloudEvent(const char* name, const char* data) {
    Log.info("Cloud event received");
}

void registerCloudVar() {
    Particle.variable("var", cloudVar);
    Log.info("Registered cloud variable: \"var\"");
}

void registerCloudFunc() {
    Particle.function("func", cloudFunc);
    Log.info("Registered cloud function: \"func\"");
}

void subscribeToCloudEvents() {
    Particle.subscribe("event", cloudEvent, MY_DEVICES);
    Log.info("Subscribed to events: \"event\"");
}

} // namespace

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY))
STARTUP(System.on(cloud_status, cloudStatusEvent))

void setup() {
    if (magic != 0x789abcde) {
        counter = 0;
        magic = 0x789abcde;
    }

    waitUntil(Serial.isConnected);

    Log.info("========================================");
    Log.info("-> setup()");

    ++counter;

    switch (counter) {
    case 1: {
        registerCloudVar();
        break;
    }
    case 2: {
        registerCloudVar();
        registerCloudFunc();
        break;
    }
    case 3: {
        registerCloudVar();
        registerCloudFunc();
        subscribeToCloudEvents();
        break;
    }
    case 4: {
        // Register the same variables, functions and subscriptions as on the previous step
        registerCloudVar();
        registerCloudFunc();
        subscribeToCloudEvents();
        break;
    }
    case 5: {
        Log.info("Not registering any functions, variables or subscriptions");
        break;
    }
#if !USE_SEMI_AUTOMATIC
    case 6: {
        Log.info("Waiting 10 seconds...");
        delay(10000);
        registerCloudVar();
        break;
    }
#endif // !USE_SEMI_AUTOMATIC
    default:
        counter = 0;
        break;
    }

#if USE_SEMI_AUTOMATIC
    Particle.connect();
#endif

    Log.info("<- setup()");
}

void loop() {
}
