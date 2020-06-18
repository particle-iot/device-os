#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

namespace {

retained unsigned counter = 0;
retained uint32_t magic = 0;
bool done = false;

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

void cloudStatusEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case cloud_status_connecting: {
        Log.info("Connecting to the cloud...");
        break;
    }
    case cloud_status_connected: {
        Log.info("Connected to the cloud");
        break;
    }
    case cloud_status_disconnecting: {
        Log.info("Disconnecting from the cloud...");
        break;
    }
    case cloud_status_disconnected: {
        Log.info("Disconnected from the cloud");
        break;
    }
    default:
        break;
    }
}

void networkStatusEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case network_status_connecting: {
        Log.info("Connecting to the network...");
        break;
    }
    case network_status_connected: {
        Log.info("Connected to the network");
        break;
    }
    case network_status_disconnecting: {
        Log.info("Disconnecting from the network...");
        break;
    }
    case network_status_disconnected: {
        Log.info("Disconnected from the network");
        break;
    }
    default:
        break;
    }
}

} // namespace

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY))
STARTUP(System.on(cloud_status, cloudStatusEvent))
STARTUP(System.on(network_status, networkStatusEvent))

void setup() {
    if (magic != 0x6789abcd) {
        counter = 0;
        magic = 0x6789abcd;
    }

    waitUntil(Serial.isConnected);

    Log.info("========================================");

    ++counter;

    Particle.connect();
}

void loop() {
    if (!Particle.connected() || done) {
        return;
    }
    switch (counter) {
    case 1: {
        Log.info("Waiting 3 seconds...");
        delay(3000);

        Log.info("Entering HIBERNATE mode");
        Log.info("Reset the device via the reset button to proceed");
        delay(1000);

        SystemSleepConfiguration conf;
        conf.mode(SystemSleepMode::HIBERNATE);
        System.sleep(conf);
        break;
    }
    case 2: {
        Log.info("Waiting 3 seconds...");
        delay(3000);

        Log.info("Entering STOP mode");
        Log.info("The device will wake up in 3 seconds and reconnect to the cloud");
        delay(1000);

        SystemSleepConfiguration conf;
        conf.mode(SystemSleepMode::STOP);
        conf.duration(3s);
        conf.gpio(D0, RISING);
        // FIXME: This flag shouldn't be necessary now that we have an API to control
        // graceful disconnection from the cloud
        conf.flag(SystemSleepFlag::WAIT_CLOUD);
        System.sleep(conf);

        Particle.connect();
        break;
    }
    default:
        counter = 0;
        break;
    }
    done = true;
}
