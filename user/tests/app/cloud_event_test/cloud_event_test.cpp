#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

#include "application.h"
#include "check.h"

#include "event_data.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

#define TEST_OLD_API_WITH_DELAY 1
#define TEST_OLD_API_WITH_ACK 1
#define TEST_NEW_API_ONE_EVENT 1
#define TEST_NEW_API_TWO_EVENTS 1
#define TEST_NEW_API_FOUR_EVENTS 1

namespace {

const auto EVENT_NAME = "test";

const unsigned MEM_USAGE_UPDATE_INTERVAL = 100; // ms
const unsigned PROGRESS_LOG_INTERVAL = 1000;
const unsigned CONNECTION_TIMEOUT = 60000;

enum Step {
    INIT_TEST,
    WAIT_CONNECT,

#if TEST_OLD_API_WITH_DELAY
    OLD_API_WITH_DELAY_INIT,
    OLD_API_WITH_DELAY_RUN,
    OLD_API_WITH_DELAY_DONE,
#endif // TEST_OLD_API_WITH_DELAY

#if TEST_OLD_API_WITH_ACK
    OLD_API_WITH_ACK_INIT,
    OLD_API_WITH_ACK_RUN,
    OLD_API_WITH_ACK_DONE,
#endif // TEST_OLD_API_WITH_ACK

#if TEST_NEW_API_ONE_EVENT
    NEW_API_ONE_EVENT_INIT,
    NEW_API_ONE_EVENT_RUN,
    NEW_API_ONE_EVENT_DONE,
#endif // TEST_NEW_API_ONE_EVENT

#if TEST_NEW_API_TWO_EVENTS
    NEW_API_TWO_EVENTS_INIT,
    NEW_API_TWO_EVENTS_RUN,
    NEW_API_TWO_EVENTS_DONE,
#endif // TEST_NEW_API_TWO_EVENTS

#if TEST_NEW_API_FOUR_EVENTS
    NEW_API_FOUR_EVENTS_INIT,
    NEW_API_FOUR_EVENTS_RUN,
    NEW_API_FOUR_EVENTS_DONE,
#endif // TEST_NEW_API_FOUR_EVENTS

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

int finishTest() {
    auto t = millis();
    curStats.timeEnd = t;
    testIdle(t);
    if (!allStats.append(std::move(curStats))) {
        return Error::NO_MEMORY;
    }
    Log.info("Test succeeded");

    // Wait a few seconds before getting the amount of free memory after the test
    delay(3000);
    allStats.last().freeMemAfter = System.freeMemory();

    return 0;
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

        Log.info("Maximum event data size (old API): %d", (int)Particle.maxEventDataSize());

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
// TEST_OLD_API_WITH_DELAY
//

#if TEST_OLD_API_WITH_DELAY

int oldApiWithDelayInit() {
    lastPublishTime = 0;

    startTest("Old API with 1-second delay");
    return nextStep();
}

int oldApiWithDelayRun() {
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

int oldApiWithDelayDone() {
    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_OLD_API_WITH_DELAY

//
// TEST_OLD_API_WITH_ACK
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
// TEST_NEW_API_*
//

#if TEST_NEW_API_ONE_EVENT || TEST_NEW_API_TWO_EVENTS || TEST_NEW_API_FOUR_EVENTS

Vector<CloudEvent> events;

int newApiInit(const char* testName, unsigned eventCount) {
    events.clear();
    if (!events.resize(eventCount)) {
        return Error::NO_MEMORY;
    }

    startTest(testName);
    return nextStep();
}

int newApiRun() {
    // Fail the test if any of the events have failed
    for (auto& ev: events) {
        if (!ev.isOk()) {
            return ev.error();
        }
    }
    // Proceed to the next step if all the events have been sent successfully
    bool allSent = true;
    for (auto& ev: events) {
        if (!ev.isSent()) {
            allSent = false;
            break;
        }
    }
    if (allSent) {
        return nextStep();
    }
    // Keep waiting if some of the events are still in progress
    for (auto& ev: events) {
        if (ev.isSending()) {
            testIdle();
            return 0;
        }
    }
    // Prepare the events for publishing
    char buf[128];
    size_t offs = 0;
    size_t bytesPerEvent = eventDataSize / events.size();
    for (auto& ev: events) {
        ev.name(EVENT_NAME);
        ev.contentType(ContentType::BINARY);
        size_t endOffs = (ev == events.last()) ? eventDataSize : (offs + bytesPerEvent);
        while (offs < endOffs) {
            size_t n = std::min(sizeof(buf), endOffs - offs);
            std::memcpy(buf, eventData + offs, n);
            ev.write(buf, n);
            offs += n;
        }
    }
    // Publish the events
    for (auto& ev: events) {
        bool ok = Particle.publish(ev);
        if (!ok) {
            int err = ev.isOk() ? Error::BUSY : ev.error();
            Log.error("Particle.publish() failed: %d", err);
            return err;
        }
    }
    return 0;
}

int newApiDone() {
    events.clear();

    CHECK(finishTest());
    return nextStep();
}

#endif // TEST_NEW_API_ONE_EVENT || TEST_NEW_API_TWO_EVENTS || TEST_NEW_API_FOUR_EVENTS

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
    return nextStep();
}

int run() {
    switch (step) {
    case INIT_TEST:
        return initTest();
    case WAIT_CONNECT:
        return waitConnect();

#if TEST_OLD_API_WITH_DELAY
    case OLD_API_WITH_DELAY_INIT:
        return oldApiWithDelayInit();
    case OLD_API_WITH_DELAY_RUN:
        return oldApiWithDelayRun();
    case OLD_API_WITH_DELAY_DONE:
        return oldApiWithDelayDone();
#endif // TEST_OLD_API_WITH_DELAY

#if TEST_OLD_API_WITH_ACK
    case OLD_API_WITH_ACK_INIT:
        return oldApiWithAckInit();
    case OLD_API_WITH_ACK_RUN:
        return oldApiWithAckRun();
    case OLD_API_WITH_ACK_DONE:
        return oldApiWithAckDone();
#endif // TEST_OLD_API_WITH_ACK

#if TEST_NEW_API_ONE_EVENT
    case NEW_API_ONE_EVENT_INIT:
        return newApiInit("New API with one event", 1);
    case NEW_API_ONE_EVENT_RUN:
        return newApiRun();
    case NEW_API_ONE_EVENT_DONE:
        return newApiDone();
#endif // TEST_NEW_API_ONE_EVENT

#if TEST_NEW_API_TWO_EVENTS
    case NEW_API_TWO_EVENTS_INIT:
        return newApiInit("New API with two events", 2);
    case NEW_API_TWO_EVENTS_RUN:
        return newApiRun();
    case NEW_API_TWO_EVENTS_DONE:
        return newApiDone();
#endif // TEST_NEW_API_TWO_EVENTS

#if TEST_NEW_API_FOUR_EVENTS
    case NEW_API_FOUR_EVENTS_INIT:
        return newApiInit("New API with four events", 4);
    case NEW_API_FOUR_EVENTS_RUN:
        return newApiRun();
    case NEW_API_FOUR_EVENTS_DONE:
        return newApiDone();
#endif // TEST_NEW_API_FOUR_EVENTS

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
