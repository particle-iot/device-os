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
const unsigned RETRY_DELAY = 3000;

char inEventData[EVENT_DATA_SIZE + 1] = {};
char outEventData[EVENT_DATA_SIZE + 1] = {};
bool retrying = false;
bool sCloudDisconnected = false;
bool sPublishFailed = false;
bool sEventNotHandled = false;


void oldEventHandler(const char* name, const char* data) {
    NAMED_SCOPE_GUARD(sg, {
        sEventNotHandled = true;
    });
    if (retrying) {
        Test::out->printlnf("Unexpected event");
        return;
    }
    if (!data) {
        Test::out->printlnf("Unexpected event size");
        return;
    }
    auto d = std::strchr(data, ' ');
    if (!d) {
        Test::out->printlnf("Invalid event format");
        return;
    }
    size_t prefixLen = d - data;
    size_t dataLen = std::strlen(d) + prefixLen;
    if (dataLen != EVENT_DATA_SIZE) {
        Test::out->printlnf("Unexpected event size");
        return;
    }
    Log.trace("recv %.*s", (int)prefixLen, data);
    std::memcpy(outEventData, data, prefixLen + 1);
    SCOPE_GUARD({
        std::memset(outEventData, 'b', prefixLen + 1); // Restore the fill bytes
    });
    bool ok = Particle.publish("devout1", outEventData);
    if (!ok) {
        Test::out->printlnf("Failed to publish event, retrying in %ums", RETRY_DELAY);
        retrying = true;
        SCOPE_GUARD({
            retrying = false;
        });
        delay(RETRY_DELAY);
        ok = Particle.publish("devout1", outEventData);
        if (!ok) {
            sPublishFailed = true;
            Test::out->printlnf("Failed to publish event");
            return;
        }
    }
    Log.trace("send %.*s", (int)prefixLen, outEventData);
    sg.dismiss();
}

void newEventHandler(CloudEvent ev) {
    NAMED_SCOPE_GUARD(sg, {
        sEventNotHandled = true;
    });
    if (ev.size() != EVENT_DATA_SIZE) {
        Test::out->printlnf("Unexpected event size");
        return;
    }
    int r = ev.read(inEventData, EVENT_DATA_SIZE);
    if (r != (int)EVENT_DATA_SIZE) {
        Test::out->printlnf("Failed to read event");
    }
    auto d = std::strchr(inEventData, ' ');
    if (!d) {
        Test::out->printlnf("Invalid event format");
        return;
    }
    size_t prefixLen = d - inEventData;
    Log.trace("recv %.*s", (int)prefixLen, inEventData);
    CloudEvent ev2 = CloudEvent().name("devout2");
    ev2.write(inEventData, prefixLen + 1); // Write the prefix
    ev2.write(outEventData, EVENT_DATA_SIZE - prefixLen - 1); // Write the fill bytes
    Particle.publish(ev2);
    if (!ev2.isOk()) {
        Test::out->printlnf("Failed to publish event");
        sPublishFailed = true;
        return;
    }
    Log.trace("send %.*s", (int)prefixLen, inEventData);
    sg.dismiss();
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
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    System.on(cloud_status, +[](system_event_t ev, int info) {
        if (info == cloud_status_disconnected) {
            sCloudDisconnected = true;
        }
    });
}

test(02_ping_pong_old_api) {
}

test(03_ping_pong_new_api) {
    SCOPE_GUARD({
        sCloudDisconnected = false;
        sPublishFailed = false;
        sEventNotHandled = false;
    });
    assertFalse(sPublishFailed);
    assertFalse(sEventNotHandled);
    assertFalse(sCloudDisconnected);
}

test(04_ping_pong_done) {
    assertFalse(sPublishFailed);
    assertFalse(sEventNotHandled);
    assertFalse(sCloudDisconnected);
}