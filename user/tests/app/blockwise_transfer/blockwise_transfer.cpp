#include "application.h"
#include "random.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
// SYSTEM_THREAD(ENABLED)

namespace {

const auto EVENT_NAME = "my_event";
const size_t EVENT_SIZE = 10 * 1024;

const SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

ProtocolFacade* proto = nullptr;
size_t bytesWritten = 0;
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
        Log.trace("Event %d: READABLE", handle);
        break;
    case PROTOCOL_EVENT_STATUS_WRITABLE:
        Log.trace("Event %d: WRITABLE", handle);
        break;
    case PROTOCOL_EVENT_STATUS_SENT:
        Log.trace("Event %d: SENT", handle);
        break;
    default:
        Log.warn("Event %d: Unknown status: %d", handle, status);
        break;
    }
}

void eventReceived(int handle, const char* name, int type, int size, void* userData) {
    Log.info("Receiving event: %s", name);
    const int r = spark_protocol_begin_event(proto, handle, nullptr /* name */, 0 /* content_type */, 0 /* size */,
            0 /* flags */, eventStatusChanged, nullptr /* user_data */, nullptr /* reserved */);
    if (r < 0) {
        Log.error("spark_protocol_begin_event() failed: %d", r);
        return;
    }
    inEventHandle = handle;
}

void eventReceived(const char* name, const char* data) {
    // Do nothing
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
    const int r = spark_protocol_add_subscription(proto, EVENT_NAME, eventReceived, nullptr /* user_data */,
            nullptr /* reserved */);
    if (r < 0) {
        Log.error("spark_protocol_add_subscription() failed: %d", r);
    }
    Particle.subscribe(EVENT_NAME, eventReceived); // FIXME
    System.on(cloud_status, cloudStatusChanged);
    waitUntil(Serial.isConnected);
    Log.info("Connecting...");
    Particle.connect();
}

void loop() {
    if (!Particle.connected()) {
        return;
    }
    if (outEventHandle >= 0) {
        NAMED_SCOPE_GUARD(g, {
            // eventStream.close();
            spark_protocol_end_event(proto, outEventHandle, nullptr /* reserved */);
            outEventHandle = -1;
        });
        if (!outEventHandle) {
            // eventStream = Particle.beginPublish(...);
            outEventHandle = spark_protocol_begin_event(proto, 0 /* handle */, EVENT_NAME, PROTOCOL_CONTENT_TYPE_PLAIN_TEXT,
                -1 /* size */, 0 /* flags */, eventStatusChanged, nullptr /* user_data */, nullptr /* reserved */);
            if (outEventHandle < 0) {
                Log.error("spark_protocol_begin_event() failed: %d", outEventHandle);
                return;
            }
        }
        // bytesAvail = eventStream.availableForWrite();
        int bytesAvail = spark_protocol_event_data_bytes_available(proto, outEventHandle, nullptr /* reserved */);
        if (bytesAvail < 0) {
            Log.error("spark_protocol_event_data_bytes_available() failed: %d", bytesAvail);
            return;
        }
        while (bytesAvail > 0 && bytesWritten < EVENT_SIZE) {
            const char c = randomChar();
            const bool hasMore = (bytesWritten < EVENT_SIZE - 1);
            // bytesWritten = eventStream.write(...);
            int n = spark_protocol_write_event_data(proto, outEventHandle, &c, 1, hasMore, nullptr /* reserved */);
            if (n != 1) {
                Log.error("spark_protocol_write_event_data() failed: %d", n);
                return;
            }
            --bytesAvail;
            ++bytesWritten;
        }
        if (bytesWritten < EVENT_SIZE) {
            g.dismiss();
        }
    }
    if (inEventHandle > 0) {
        NAMED_SCOPE_GUARD(g, {
            // eventStream.close();
            spark_protocol_end_event(proto, inEventHandle, nullptr /* reserved */);
            outEventHandle = -1;
        });
        size_t bytesRead = 0;
        int r = 0;
        for (;;) {
            char c = 0;
            // bytesRead = eventStream.read(...);
            int r = spark_protocol_read_event_data(proto, inEventHandle, &c, 1, nullptr /* reserved */);
            if (r < 0) {
                break;
            }
            Log.write(&c, 1);
            ++bytesRead;
        }
        if (bytesRead > 0) {
            Log.print("\r\n");
        }
        if (r == SYSTEM_ERROR_WOULD_BLOCK) {
            g.dismiss();
        } else if (r != SYSTEM_ERROR_END_OF_STREAM) {
            Log.error("Error while receiving event: %d", r);
        }
    }
}
