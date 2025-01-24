#include <cstring>

#include "application.h"
#include "test.h"

#include "scope_guard.h"

namespace {

/*
Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "comm.coap", LOG_LEVEL_ALL },
    { "app", LOG_LEVEL_ALL }
});
*/

const size_t EVENT_DATA_SIZE = 1024;
const unsigned RETRY_DELAY = 1000;

char inEventData[EVENT_DATA_SIZE + 1] = {};
char outEventData[EVENT_DATA_SIZE + 1] = {};
bool retrying = false;

void oldEventHandler(const char* name, const char* data) {
    if (retrying) {
        Log.warn("Unexpected event");
        return;
    }
    if (!data) {
        Log.error("Unexpected event size");
        return;
    }
    auto d = std::strchr(data, ' ');
    if (!d) {
        Log.error("Invalid event format");
        return;
    }
    size_t prefixLen = d - data;
    size_t dataLen = std::strlen(d) + prefixLen;
    if (dataLen != EVENT_DATA_SIZE) {
        Log.error("Unexpected event size");
        return;
    }
    Log.trace("recv %.*s", (int)prefixLen, data);
    std::memcpy(outEventData, data, prefixLen + 1);
    bool ok = Particle.publish("devout1", outEventData);
    if (!ok) {
        Log.warn("Failed to publish event, retrying in %ums", RETRY_DELAY);
        retrying = true;
        SCOPE_GUARD({
            retrying = false;
        });
        delay(RETRY_DELAY);
        ok = Particle.publish("devout1", outEventData);
        if (!ok) {
            Log.error("Failed to publish event");
            return;
        }
    }
    Log.trace("send %.*s", (int)prefixLen, outEventData);
}

void newEventHandler(CloudEvent ev) {
    if (ev.size() != EVENT_DATA_SIZE) {
        Log.error("Unexpected event size");
        return;
    }
    int r = ev.read(inEventData, EVENT_DATA_SIZE);
    if (r != (int)EVENT_DATA_SIZE) {
        Log.error("Failed to read event");
    }
    auto d = std::strchr(inEventData, ' ');
    if (!d) {
        Log.error("Invalid event format");
        return;
    }
    size_t prefixLen = d - inEventData;
    Log.trace("recv %.*s", (int)prefixLen, inEventData);
    CloudEvent ev2 = CloudEvent().name("devout2");
    ev2.write(inEventData, prefixLen + 1); // Write the prefix
    ev2.write(outEventData, EVENT_DATA_SIZE - prefixLen - 1); // Write the fill bytes
    Particle.publish(ev2);
    if (!ev2.isOk()) {
        Log.error("Failed to publish event");
        return;
    }
    Log.trace("send %.*s", (int)prefixLen, inEventData);
}

inline String scopedEventName(const char* name) {
    return System.deviceID() + '/' + name;
}

} // namespace

test(01_connect_and_subscribe) {
    std::memset(outEventData, 'b', EVENT_DATA_SIZE);

    // Prefix the names of events sent towards the device with the device ID. This way, the tests
    // can run simultaneously on multiple devices under the same user account
    Particle.subscribe(scopedEventName("devin1"), oldEventHandler);
    Particle.subscribe(scopedEventName("devin2"), newEventHandler);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 5 * 60000));
}

test(02_ping_pong_old_api) {
}

test(03_ping_pong_new_api) {
}
