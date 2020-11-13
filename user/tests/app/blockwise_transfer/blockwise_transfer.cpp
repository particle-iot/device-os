#include "application.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
// SYSTEM_THREAD(ENABLED)

namespace {

const auto IN_EVENT_NAME = "event1";
const auto OUT_EVENT_NAME = "event2";

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

ProtocolFacade* proto = nullptr;
size_t bytesSent = 0;
size_t bytesReceived = 0;
int inEventHandle = 0;
int outEventHandle = 0;

void cloudStatusChanged(system_event_t event, int param, void* data) {
    switch (param) {
    case cloud_status_connected:
        Log.info("Connected");
        break;
    default:
        break;
    }
}

void eventStatusChanged(int handle, int status, void* userData) {
    switch (status) {
    case PROTOCOL_EVENT_STATUS_READABLE:
        Log.trace("Event %d: PROTOCOL_EVENT_STATUS_READABLE", handle);
        break;
    case PROTOCOL_EVENT_STATUS_WRITABLE:
        Log.trace("Event %d: PROTOCOL_EVENT_STATUS_WRITABLE", handle);
        break;
    case PROTOCOL_EVENT_STATUS_SENT:
        Log.trace("Event %d: PROTOCOL_EVENT_STATUS_SENT", handle);
        break;
    default:
        Log.warn("Event %d: Unknown status: %d", handle, status);
        break;
    }
}

void reset_() {
    // inEventStream.close();
    spark_protocol_end_event(proto, inEventHandle, nullptr /* reserved */);
    inEventHandle = 0;
    bytesReceived = 0;
    // outEventStream.close();
    spark_protocol_end_event(proto, outEventHandle, nullptr /* reserved */);
    outEventHandle = 0;
    bytesSent = 0;
}

void eventReceived(int handle, const char* name, int type, int size, void* userData) {
    inEventHandle = handle;
    NAMED_SCOPE_GUARD(g, {
        reset_();
    });
    Log.info("Receiving event: %s", name);
    if (size >= 0) {
        Log.info("Size: %d", size);
    }
    // inEventStream.open()
    int r = spark_protocol_begin_event(proto, inEventHandle, nullptr /* name */, 0 /* content_type */, 0 /* size */,
            0 /* flags */, eventStatusChanged, nullptr /* user_data */, nullptr /* reserved */);
    if (r < 0) {
        Log.error("spark_protocol_begin_event() failed: %d", r);
        return;
    }
    Log.info("Sending event: %s", OUT_EVENT_NAME);
    // outEventStream = Particle.beginPublish(...);
    r = spark_protocol_begin_event(proto, 0 /* handle */, OUT_EVENT_NAME, PROTOCOL_CONTENT_TYPE_PLAIN_TEXT,
        -1 /* size */, 0 /* flags */, eventStatusChanged, nullptr /* user_data */, nullptr /* reserved */);
    if (r < 0) {
        Log.error("spark_protocol_begin_event() failed: %d", r);
        return;
    }
    outEventHandle = r;
    g.dismiss();
}

void eventReceived(const char* name, const char* data) {
    // Do nothing
}

} // namespace

void setup() {
    proto = spark_protocol_instance();
    const int r = spark_protocol_add_subscription(proto, IN_EVENT_NAME, eventReceived, nullptr /* user_data */,
            nullptr /* reserved */);
    if (r < 0) {
        Log.error("spark_protocol_add_subscription() failed: %d", r);
    }
    Particle.subscribe(IN_EVENT_NAME, eventReceived); // FIXME
    System.on(cloud_status, cloudStatusChanged);
    waitUntil(Serial.isConnected);
    Log.info("Connecting...");
    Particle.connect();
}

void loop() {
    if (!Particle.connected() || !inEventHandle || !outEventHandle) {
        return;
    }
    NAMED_SCOPE_GUARD(g, {
        reset_();
    });
    // bytesAvail = outEventStream.availableForWrite();
    int availForWrite = spark_protocol_event_data_bytes_available(proto, outEventHandle, nullptr /* reserved */);
    if (availForWrite < 0) {
        Log.error("spark_protocol_event_data_bytes_available() failed: %d", availForWrite);
        return;
    }
    while (availForWrite > 0) {
        char buf[128] = {};
        // bytesRead = inEventStream.read(...);
        int n = spark_protocol_read_event_data(proto, inEventHandle, buf, sizeof(buf), nullptr /* reserved */);
        if (n > 0) {
            bytesReceived += n;
            // bytesWritten = outEventStream.write(...);
            n = spark_protocol_write_event_data(proto, outEventHandle, buf, n, true /* has_more */, nullptr /* reserved */);
            if (n > 0) {
                bytesSent += n;
                availForWrite -= n;
            } else {
                Log.error("spark_protocol_write_event_data() failed: %d", n);
                return;
            }
        } else if (n == SYSTEM_ERROR_END_OF_STREAM) {
            // outEventStream.close();
            n = spark_protocol_write_event_data(proto, outEventHandle, nullptr, 0, false /* has_more */, nullptr /* reserved */);
            if (n == 0) {
                Log.info("Bytes received: %u", (unsigned)bytesReceived);
                Log.info("Bytes sent: %u", (unsigned)bytesSent);
            } else {
                Log.error("spark_protocol_write_event_data() failed: %d", n);
            }
            return;
        } else if (n == SYSTEM_ERROR_WOULD_BLOCK) {
            break;
        }
    }
    g.dismiss();
}
