#include "application.h"
#include "random.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
// SYSTEM_THREAD(ENABLED)

namespace {

const size_t EVENT_DATA_SIZE = 2048;

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

ProtocolFacade* proto = nullptr;
size_t bytesWritten = 0;
int eventHandle = 0;

void eventStatusChanged(int handle, int status, void* userData) {
    switch (status) {
    case PROTOCOL_EVENT_STATUS_READABLE:
        Log.trace("PROTOCOL_EVENT_STATUS_READABLE");
        break;
    case PROTOCOL_EVENT_STATUS_WRITABLE:
        Log.trace("PROTOCOL_EVENT_STATUS_WRITABLE");
        break;
    case PROTOCOL_EVENT_STATUS_SENT:
        Log.trace("PROTOCOL_EVENT_STATUS_SENT");
        break;
    default:
        Log.warn("Unknown event status: %d", status);
        break;
    }
}

void cloudStatusChanged(system_event_t event, int param, void* data) {
    switch (param) {
    case cloud_status_connected:
        Log.info("Connected");
        break;
    default:
        break;
    }
}

char randomChar() {
    static Random rand;
    char c = 0;
    rand.genBase32(&c, 1);
    return c;
}

} // namespace

void setup() {
    proto = spark_protocol_instance();
    System.on(cloud_status, cloudStatusChanged);
    waitUntil(Serial.isConnected);
    Log.info("Connecting...");
    Particle.connect();
}

void loop() {
    if (eventHandle < 0 || !Particle.connected()) {
        return;
    }
    NAMED_SCOPE_GUARD(g, {
        // eventStream.close();
        spark_protocol_end_event(proto, eventHandle, 0 /* error */, nullptr /* reserved */);
        eventHandle = -1;
    });
    if (!eventHandle) {
        // eventStream = Particle.beginPublish(...);
        eventHandle = spark_protocol_begin_event(proto, "my_event", PROTOCOL_CONTENT_TYPE_PLAIN_TEXT, -1 /* size */,
            0 /* flags */, eventStatusChanged, nullptr /* user_data */, nullptr /* reserved */);
        if (eventHandle < 0) {
            Log.error("spark_protocol_begin_event() failed: %d", eventHandle);
            return;
        }
    }
    // bytesAvail = eventStream.availableForWrite();
    int bytesAvail = spark_protocol_event_data_bytes_available(proto, eventHandle, nullptr /* reserved */);
    if (bytesAvail < 0) {
        Log.error("spark_protocol_event_data_bytes_available() failed: %d", bytesAvail);
        return;
    }
    while (bytesAvail > 0 && bytesWritten < EVENT_DATA_SIZE) {
        const char c = randomChar();
        const bool hasMore = (bytesWritten < EVENT_DATA_SIZE - 1);
        // bytesWritten = eventStream.write(...);
        int n = spark_protocol_write_event_data(proto, eventHandle, &c, 1, hasMore, nullptr /* reserved */);
        if (n != 1) {
            Log.error("spark_protocol_write_event_data() failed: %d", n);
            return;
        }
        --bytesAvail;
        ++bytesWritten;
    }
    if (bytesWritten < EVENT_DATA_SIZE) {
        g.dismiss();
    }
}
