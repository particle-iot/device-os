#include "application.h"

#include <cmath>

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY))

namespace {

retained time_t lastLoopTime = 0;
retained uint32_t magic = 0;

system_tick_t lastLoopMillis = 0;
system_tick_t lastProgressUpdate = 0;
system_tick_t updateStart = 0;
system_tick_t networkConnectStart = 0;
system_tick_t cloudConnectStart = 0;
bool timeChanged = false;

void firmwareUpdateEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case firmware_update_begin: {
        const auto file = (FileTransfer::Descriptor*)data;
        Log.info("Firmware update started, file size: %uK", (unsigned)file->file_length / 1024);
        updateStart = millis();
        lastProgressUpdate = millis();
        break;
    }
    case firmware_update_failed: // FIXME
    case firmware_update_complete: {
        const auto t = millis();
        Log.info("Firmware update completed in %us", (unsigned)(t - updateStart) / 1000);
        break;
    }
    case firmware_update_progress: {
        const auto file = (FileTransfer::Descriptor*)data;
        const auto t = millis();
        if (t - lastProgressUpdate >= 5000) {
            const auto bytesDone = file->chunk_address - file->file_address;
            Log.info("%u%%", (unsigned)floor((double)bytesDone / file->file_length * 100));
            lastProgressUpdate = t;
        }
        break;
    }
    default:
        break;
    }
}

void timeChangedEvent(system_event_t event, int param, void* data) {
    Log.info("System time changed");
    auto t = Time.now();
    if (lastLoopTime) {
        t -= lastLoopTime;
        Log.info("Application downtime: %us", (unsigned)t);
    }
    timeChanged = true;
}

void networkStatusEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case network_status_connecting: {
        Log.info("Connecting to the network");
        networkConnectStart = millis();
        break;
    }
    case network_status_connected: {
        const auto t = millis();
        Log.info("Connected to the network in %us", (unsigned)(t - networkConnectStart) / 1000);
        break;
    }
    default:
        break;
    }
}

void cloudStatusEvent(system_event_t event, int param, void* data) {
    switch (param) {
    case cloud_status_connecting: {
        Log.info("Connecting to the cloud");
        cloudConnectStart = millis();
        break;
    }
    case cloud_status_connected: {
        const auto t = millis();
        Log.info("Connected to the cloud in %us", (unsigned)(t - cloudConnectStart) / 1000);
        break;
    }
    default:
        break;
    }
}

void resetPendingEvent(system_event_t event, int param, void* data) {
    HAL_Delay_Milliseconds(1000);
}

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    // { "comm.ota", LOG_LEVEL_ALL },
    { "app", LOG_LEVEL_ALL }
});

} // namespace

void setup() {
    if (magic != 0x89abcdef || System.resetReason() == RESET_REASON_PIN_RESET) {
        lastLoopTime = 0;
        magic = 0x89abcdef;
    }

    waitUntil(Serial.isConnected);

    Log.info("========================================");

    System.on(firmware_update, firmwareUpdateEvent);
    System.on(time_changed, timeChangedEvent);
    System.on(network_status, networkStatusEvent);
    System.on(cloud_status, cloudStatusEvent);
    System.on(reset_pending, resetPendingEvent);

    Particle.connect();

    lastLoopMillis = millis();
}

void loop() {
    const auto t = millis();
    if (t - lastLoopMillis >= 1000) {
        if (timeChanged) {
            lastLoopTime = Time.now();
        }
        lastLoopMillis = t;
    }
}
