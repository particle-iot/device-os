#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

#include "application.h"
#include "check.h"

#include "event_data.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

#define TEST_OLD_API 1
#define TEST_OLD_API_WITH_ACK 1
#define TEST_NEW_API 1
#define TEST_NEW_API_TWO_EVENTS 1

namespace {

const auto EVENT_NAME = "test";
const unsigned MEM_USAGE_UPDATE_INTERVAL = 100; // ms
const unsigned PROGRESS_LOG_INTERVAL = 1000;
const unsigned CONNECTION_TIMEOUT = 60000;

enum Step {
    INIT_TEST,
    WAIT_CONNECT,
#if TEST_OLD_API
    OLD_API_INIT,
    OLD_API_RUN,
    OLD_API_DONE,
#endif // TEST_OLD_API
#if TEST_OLD_API_WITH_ACK
    OLD_API_WITH_ACK_INIT,
    OLD_API_WITH_ACK_RUN,
    OLD_API_WITH_ACK_DONE,
#endif // TEST_OLD_API_WITH_ACK
#if TEST_NEW_API
    NEW_API_INIT,
    NEW_API_RUN,
    NEW_API_DONE,
#endif // TEST_NEW_API
#if TEST_NEW_API_TWO_EVENTS
    NEW_API_TWO_EVENTS_INIT,
    NEW_API_TWO_EVENTS_RUN,
    NEW_API_TWO_EVENTS_DONE,
#endif // TEST_NEW_API_TWO_EVENTS
    PRINT_STATS,
    DONE
};

struct Stats {
    const char* testName;
    system_tick_t timeStart;
    system_tick_t timeEnd;
    unsigned freeMemBefore;
    unsigned freeMemAfter;
    unsigned minFreeMem;

    Stats() :
            testName(""),
            timeStart(0),
            timeEnd(0),
            freeMemBefore(0),
            freeMemAfter(0),
            minFreeMem(0) {
    }
};

SerialLogHandler logHandler(LOG_LEVEL_WARN, {
    // { "comm.coap", LOG_LEVEL_ALL },
    { "app", LOG_LEVEL_ALL }
});

Vector<Stats> allStats;
Stats curStats;
system_tick_t lastProgressLogTime = 0;
system_tick_t lastMemUsageUpdateTime = 0;
size_t eventDataOffset = 0;
size_t lastEventDataOffset = 0;
int step = Step::INIT_TEST;

void startTest(const char* name) {
    delay(1000);

    curStats.timeStart = millis();
    curStats.freeMemBefore = System.freeMemory();
    curStats.minFreeMem = std::numeric_limits<decltype(curStats.minFreeMem)>::max();
    curStats.testName = name;

    eventDataOffset = 0;
    lastEventDataOffset = 0;

    Log.print("\r\n");
    Log.info("Running test: %s", name);

    auto t = millis();
    lastMemUsageUpdateTime = t;
    lastProgressLogTime = t;
}

int finishTest() {
    curStats.timeEnd = millis();
    auto freeMem = System.freeMemory();
    if (freeMem < curStats.minFreeMem) {
        curStats.minFreeMem = freeMem;
    }
    if (!allStats.append(std::move(curStats))) {
        return Error::NO_MEMORY;
    }
    Log.info("Test succeeded");

    // Wait a few seconds before getting the amount of free memory after the test
    delay(3000);
    allStats.last().freeMemAfter = System.freeMemory();

    return 0;
}

void testIdle(system_tick_t t = millis()) {
    if (t - lastMemUsageUpdateTime >= MEM_USAGE_UPDATE_INTERVAL) {
        auto freeMem = System.freeMemory();
        if (freeMem < curStats.minFreeMem) {
            curStats.minFreeMem = freeMem;
        }
        lastMemUsageUpdateTime = millis();
    }
    if (eventDataOffset != lastEventDataOffset && (!lastEventDataOffset || eventDataOffset >= eventDataSize ||
                t - lastProgressLogTime >= PROGRESS_LOG_INTERVAL)) {
        Log.info("Sent %u of %u bytes", (unsigned)eventDataOffset, (unsigned)eventDataSize);
        lastEventDataOffset = eventDataOffset;
        lastProgressLogTime = millis();
    }
}

int nextStep() {
    ++step;
    return 0;
}

void logFreeHeap() {
    Log.info("Free heap: %u", (unsigned)System.freeMemory());
}

//
// INIT_TEST
//

system_tick_t connectStartTime = 0;

int initTest() {
    if (!allStats.reserve(10)) {
        return Error::NO_MEMORY;
    }
    Log.info("Connecting");
    Particle.connect();
    connectStartTime = millis();
    nextStep();
    return 0;
}

//
// WAIT_CONNECT
//

int waitConnect() {
    if (Particle.connected()) {
        Log.info("Connected");
        logFreeHeap();

        Log.info("Maximum event data size (classic API): %d", (int)Particle.maxEventDataSize());

        return nextStep();
    }
    if (millis() - connectStartTime >= CONNECTION_TIMEOUT) {
        Log.error("Connection timeout");
        return Error::TIMEOUT;
    }
    return 0;
}

system_tick_t lastPublishTime = 0;

//
// OLD_API
//

#if TEST_OLD_API

int oldApiInit() {
    lastPublishTime = 0;

    startTest("Old API with 1-second delay");
    return nextStep();
}

int oldApiRun() {
    auto t = millis();
    if (t - lastPublishTime < 1000) {
        testIdle(t);
        return 0;
    }
    if (eventDataOffset >= eventDataSize) {
        return nextStep();
    }
    size_t n = std::min<size_t>(eventDataSize - eventDataOffset, 1024);
    auto f = Particle.publish(EVENT_NAME, (const char*)eventData + eventDataOffset, n, ContentType::BINARY);
    if (!f.isDone()) {
        return Error::INVALID_STATE; // Should not happen
    }
    if (f.isFailed()) {
        int err = f.error().type();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    lastPublishTime = millis();
    eventDataOffset += n;
    return 0;
}

int oldApiDone() {
    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_OLD_API

//
// OLD_API_WITH_ACK
//

#if TEST_OLD_API_WITH_ACK

int publishError = 0;
bool publishInProgress = false;

void onPublishSuccess(bool /* ok */) {
    publishInProgress = false;
}

void onPublishError(Error err) {
    publishError = err.type();
    publishInProgress = false;
}

int oldApiWithAckInit() {
    publishInProgress = false;
    publishError = 0;

    startTest("Old API with completion handling");
    return nextStep();
}

int oldApiWithAckRun() {
    if (publishInProgress) {
        testIdle();
        return 0;
    }
    if (publishError < 0) {
        return publishError;
    }
    if (eventDataOffset >= eventDataSize) {
        return nextStep();
    }
    size_t n = std::min<size_t>(eventDataSize - eventDataOffset, 1024);
    auto f = Particle.publish(EVENT_NAME, (const char*)eventData + eventDataOffset, n, ContentType::BINARY, WITH_ACK);
    if (f.isDone()) {
        if (!f.isFailed()) {
            return Error::INVALID_STATE; // Should not happen
        }
        int err = f.error().type();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    publishInProgress = true;
    eventDataOffset += n;
    f.onSuccess(onPublishSuccess);
    f.onError(onPublishError);
    return 0;
}

int oldApiWithAckDone() {
    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_OLD_API_WITH_ACK

//
// NEW_API
//

#if TEST_NEW_API

CloudEvent event;

int newApiInit() {
    event.reset();

    startTest("New API with one event");
    return nextStep();
}

int newApiRun() {
    if (event.sending()) {
        testIdle();
        return 0;
    }
    if (!event.ok()) {
        return event.error();
    }
    if (event.sent()) {
        return nextStep();
    }
    event.name(EVENT_NAME);
    event.contentType(ContentType::BINARY);

    // XXX: The filesystem API fails when writing the entire event data at once
    char buf[128];
    size_t offs = 0;
    while (offs < eventDataSize) {
        size_t n = std::min(sizeof(buf), eventDataSize - offs);
        std::memcpy(buf, eventData + offs, n);
        event.write(buf, n);
        offs += n;
    }

    bool ok = Particle.publish(event);
    if (!ok) {
        int err = event.ok() ? Error::UNKNOWN : event.error();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    return 0;
}

int newApiDone() {
    event.reset(); // Free the payload data
    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_NEW_API

//
// NEW_API_WITH_TWO_EVENTS
//

#if TEST_NEW_API_TWO_EVENTS

CloudEvent event2;

int newApiTwoEventsInit() {
    event.reset();
    event2.reset();

    startTest("New API with two events");
    return nextStep();
}

int newApiTwoEventsRun() {
    if (!event.ok() || !event2.ok()) {
        return event.ok() ? event2.error() : event.error();
    }
    if (event.sent() && event2.sent()) {
        return nextStep();
    }
    if (event.sending() || event2.sending()) {
        testIdle();
        return 0;
    }
    event.name(EVENT_NAME);
    event2.name(EVENT_NAME);
    event.contentType(ContentType::BINARY);
    event2.contentType(ContentType::BINARY);
    
    char buf[128];
    auto size1 = eventDataSize / 2; // The amount of data to be sent in the first event
    size_t offs = 0;
    while (offs < size1) {
        size_t n = std::min(sizeof(buf), size1 - offs);
        std::memcpy(buf, eventData + offs, n);
        event.write(buf, n);
        offs += n;
    }
    while (offs < eventDataSize) {
        size_t n = std::min(sizeof(buf), eventDataSize - offs);
        std::memcpy(buf, eventData + offs, n);
        event2.write(buf, n);
        offs += n;
    }

    bool ok = Particle.publish(event);
    if (!ok) {
        int err = event.ok() ? Error::UNKNOWN : event.error();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    ok = Particle.publish(event2);
    if (!ok) {
        int err = event2.ok() ? Error::UNKNOWN : event2.error();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    return 0;
}

int newApiTwoEventsDone() {
    event.reset(); // Free the payload data
    event2.reset();
    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_NEW_API_TWO_EVENTS

int printStats() {
    for (const auto& s: allStats) {
        Log.print("\r\n");
        Log.info("***** %s", s.testName);
        Log.info("Test duration:    %gs", std::round((s.timeEnd - s.timeStart) / 100.0) / 10);
        Log.info("Free heap before: %u", s.freeMemBefore);
        Log.info("Free heap after:  %u", s.freeMemAfter);
        unsigned notFreedMem = (s.freeMemBefore > s.freeMemAfter) ? (s.freeMemBefore - s.freeMemAfter) : 0;
        Log.info("Heap not freed:   %u", notFreedMem);
        unsigned maxMemUsed = (s.freeMemBefore > s.minFreeMem) ? (s.freeMemBefore - s.minFreeMem) : 0;
        Log.info("Max heap usage:   %u", maxMemUsed);
    }
    nextStep();
    return 0;
}

int run() {
    switch (step) {
    case INIT_TEST:
        return initTest();
    case WAIT_CONNECT:
        return waitConnect();
#if TEST_OLD_API
    case OLD_API_INIT:
        return oldApiInit();
    case OLD_API_RUN:
        return oldApiRun();
    case OLD_API_DONE:
        return oldApiDone();
#endif // TEST_OLD_API
#if TEST_OLD_API_WITH_ACK
    case OLD_API_WITH_ACK_INIT:
        return oldApiWithAckInit();
    case OLD_API_WITH_ACK_RUN:
        return oldApiWithAckRun();
    case OLD_API_WITH_ACK_DONE:
        return oldApiWithAckDone();
#endif // TEST_OLD_API_WITH_ACK
#if TEST_NEW_API
    case NEW_API_INIT:
        return newApiInit();
    case NEW_API_RUN:
        return newApiRun();
    case NEW_API_DONE:
        return newApiDone();
#endif // TEST_NEW_API
#if TEST_NEW_API_TWO_EVENTS
    case NEW_API_TWO_EVENTS_INIT:
        return newApiTwoEventsInit();
    case NEW_API_TWO_EVENTS_RUN:
        return newApiTwoEventsRun();
    case NEW_API_TWO_EVENTS_DONE:
        return newApiTwoEventsDone();
#endif // TEST_NEW_API_TWO_EVENTS
    case PRINT_STATS:
        return printStats();
    default:
        return 0;
    }
}

} // namespace

void setup() {
    waitUntil(Serial.isConnected);

    Log.info("Started");
    logFreeHeap();
}

void loop() {
    int r = run();
    if (r < 0) {
        Log.print("\r\n");
        Log.error("***** Test failed: %d", r);
        step = Step::DONE;
    }
}
