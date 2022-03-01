#include "application.h"

#ifndef USE_AUTOMATIC_MODE
#define USE_AUTOMATIC_MODE 0
#endif

namespace {

enum TestStep {
    NO_VARS_NO_EVENTS = 1,
    VAR1_NO_EVENTS,
    VAR1_EVENT1,
    VAR1_VAR2_EVENT1,
    VAR1_VAR2_EVENT1_AGAIN,
    VAR1_VAR2_EVENT2,
    DONE
};

enum AppState {
    VAR1 = 0x01, // Register the "var1" variable
    VAR2 = 0x02, // Register the "var2" variable
    EVENT1 = 0x04, // Subscribe to "event1" events
    EVENT2 = 0x08 // Subscribe to "event2" events
};

retained unsigned testStep = 1;
bool testStepDone = false;

String var1 = "var1";
String var2 = "var2";

void eventHandler(const char* event, const char* data) {
    Log.info("Received event: %s", event);
}

void setAppState(unsigned flags) {
    if (flags & VAR1) {
        Log.info("Registering variable: var1");
        Particle.variable("var1", var1);
    }
    if (flags & VAR2) {
        Log.info("Registering variable: var2");
        Particle.variable("var2", var2);
    }
    if (flags & EVENT1) {
        Log.info("Subscribing to event: event1");
        Particle.subscribe("event1", eventHandler, MY_DEVICES);
    }
    if (flags & EVENT2) {
        Log.info("Subscribing to event: event2");
        Particle.subscribe("event2", eventHandler, MY_DEVICES);
    }
}

const Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "comm.protocol", LOG_LEVEL_INFO },
    { "app", LOG_LEVEL_ALL }
});

STARTUP({
    System.enableFeature(FEATURE_RETAINED_MEMORY);
    set_system_mode(USE_AUTOMATIC_MODE ? AUTOMATIC : SEMI_AUTOMATIC);
    logHandler.stream()->println("================================================================================");
    logHandler.stream()->printlnf("Test step: %u", testStep);
});

} // namespace

void setup() {
    Log.info("-> setup()");
    switch (testStep) {
    case NO_VARS_NO_EVENTS: {
        setAppState(0);
        Log.info("Device should send an empty application Describe and system subscriptions");
        break;
    }
    case VAR1_NO_EVENTS: {
        setAppState(VAR1);
        Log.info("Device should send a new application Describe");
        break;
    }
    case VAR1_EVENT1: {
        setAppState(VAR1 | EVENT1);
        Log.info("Device should send subscriptions");
        break;
    }
    case VAR1_VAR2_EVENT1: {
        setAppState(VAR1 | VAR2 | EVENT1);
        Log.info("Device should send a new application Describe");
        break;
    }
    case VAR1_VAR2_EVENT1_AGAIN: {
        setAppState(VAR1 | VAR2 | EVENT1);
        Log.info("Device should not send any Describe or subscription messages");
        break;
    }
    case VAR1_VAR2_EVENT2: {
        // FIXME: There's no way to clear subscriptions cached by the server
        setAppState(VAR1 | VAR2 | EVENT2);
        Log.info("Device Service erroneously thinks that there's still a subscription for \"event1\"");
        break;
    }
    default:
        testStepDone = true;
        break;
    }
    Particle.connect();
    Log.info("<- setup()");
}

void loop() {
    if (!testStepDone && Particle.connected()) {
        testStepDone = true;
        logHandler.stream()->println("================================================================================");
        if (++testStep == DONE) {
            logHandler.stream()->println("All test steps completed");
        } else {
            logHandler.stream()->println("Test step completed; reset the device");
        }
    }
}
