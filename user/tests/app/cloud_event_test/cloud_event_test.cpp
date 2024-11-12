#include <algorithm>
#include <limits>
#include <cmath>

#include "application.h"
#include "check.h"

#include "event_data.h"

SYSTEM_MODE(SEMI_AUTOMATIC)
SYSTEM_THREAD(ENABLED)

#define TEST_OLD_API 1
#define TEST_OLD_API_WITH_ACK 1
#define TEST_NEW_API 1

namespace {

const auto EVENT_NAME = "test";
const unsigned MEM_USAGE_UPDATE_INTERVAL = 100; // ms
const unsigned PROGRESS_LOG_INTERVAL = 1000;
const unsigned CONNECTION_TIMEOUT = 30000;
const unsigned TEST_DELAY = 3000;

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
    delay(TEST_DELAY);

    curStats.timeStart = millis();
    curStats.freeMemBefore = System.freeMemory();
    curStats.minFreeMem = std::numeric_limits<decltype(curStats.minFreeMem)>::max();
    curStats.testName = name;

    eventDataOffset = 0;
    lastEventDataOffset = 0;

    Log.info("\r\nRunning test: %s", name);

    auto t = millis();
    lastMemUsageUpdateTime = t;
    lastProgressLogTime = t;
}

int finishTest() {
    curStats.timeEnd = millis();
    curStats.freeMemAfter = System.freeMemory();
    if (!allStats.append(std::move(curStats))) {
        return Error::NO_MEMORY;
    }
    Log.info("Test succeeded");
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
    if (eventDataOffset != lastEventDataOffset && t - lastProgressLogTime >= PROGRESS_LOG_INTERVAL) {
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
// OLD_API_*
//

#if TEST_OLD_API

int oldApiInit() {
    lastPublishTime = 0;

    startTest("Old API with 1-second delay");
    return 0;
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
    bool ok = Particle.publish(EVENT_NAME, (const char*)eventData + eventDataOffset, n, ContentType::BINARY);
    if (!ok) {
        Log.error("Particle.publish() failed");
        return Error::NETWORK;
    }
    eventDataOffset += n;
    return 0;
}

int oldApiDone() {
    CHECK(finishTest());
    return 0;
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
    lastPublishTime = 0;
    publishInProgress = false;
    publishError = 0;

    startTest("Old API with completion handling");
    return 0;
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
    auto f = Particle.publish(EVENT_NAME, (const char*)eventData + eventDataOffset, n, ContentType::BINARY);
    f.onSuccess(onPublishSuccess);
    f.onError(onPublishError);
    if (f.isDone() && f.isFailed()) {
        int err = f.error().type();
        Log.error("Particle.publish() failed: %d", err);
        return err;
    }
    eventDataOffset += n;
    return 0;
}

int oldApiWithAckDone() {
    CHECK(finishTest());
    return 0;
}

#endif // TEST_OLD_API_WITH_ACK

//
// NEW_API
//

#if TEST_NEW_API

CloudEvent event;

int newApiInit() {
    event = CloudEvent();

    startTest("New API");
    return 0;
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
    event.write(eventData, eventDataSize);
    bool ok = Particle.publish(event);
    if (!ok) {
        Log.error("Particle.publish() failed");
        return event.ok() ? Error::NETWORK : event.error();
    }
    return 0;
}

int newApiDone() {
    CHECK(finishTest());
    return 0;
}

#endif // TEST_NEW_API

int printStats() {
    for (const auto& s: allStats) {
        Log.info("\r\n***** %s", s.testName);
        Log.info("Test duration:    %gs", std::round((s.timeEnd - s.timeStart) / 100.0) / 10);
        Log.info("Free heap before: %u", s.freeMemBefore);
        Log.info("Free heap after:  %u", s.freeMemAfter);
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
        Log.error("Test failed: %d", r);
        step = Step::DONE;
    }
}
