#include "application.h"

#include <deque>

struct Event {
    system_event_t type;
    int data;

    explicit Event(system_event_t type, int data = 0) :
            type(type),
            data(data) {
    }
};

const uint32_t testTimeout = 10000;

std::deque<Event> eventQueue;
uint32_t testStarted = 0;
int eventData = 0;

#define WAIT_EVENT(event) \
        do { \
            while (eventQueue.empty() && millis() - testStarted < testTimeout) { \
                Particle.process(); \
            } \
            if (eventQueue.empty() || eventQueue.front().type != (event)) { \
                return stopTest(false); \
            } \
            eventData = eventQueue.front().data; \
            eventQueue.pop_front(); \
        } while (false)

void eventHandler(system_event_t event, int data, void*) {
    eventQueue.push_back(Event(event, data));
}

void startTest() {
    digitalWrite(D7, HIGH); // User can now start clicking SETUP button
    eventQueue = std::deque<Event>();
    testStarted = millis();
}

int stopTest(bool ok) {
    digitalWrite(D7, LOW);
    return ok ? 0 : 1;
}

int testClicks(String arg) {
    startTest();
    const int expectedClicks = arg.toInt();
    if (expectedClicks <= 0) {
        return stopTest(false);
    }
    int clicks = 0;
    do {
        // Waiting for button press event
        WAIT_EVENT(button_status);
        if (eventData != 0) {
            return stopTest(false);
        }
        // Waiting for button release event
        WAIT_EVENT(button_status);
        if (eventData == 0 || eventData > 1000) {
            return stopTest(false);
        }
        // Waiting for button click event
        WAIT_EVENT(button_click);
        if ((int)eventData != clicks + 1) {
            return stopTest(false);
        }
        ++clicks;
        if (clicks == expectedClicks) {
            // Waiting for final click event
            WAIT_EVENT(button_final_click);
            if ((int)eventData != clicks) {
                return stopTest(false);
            }
        }
    } while (clicks != expectedClicks && millis() - testStarted < testTimeout);
    if (clicks != expectedClicks) {
        return stopTest(false);
    }
    return stopTest(true);
}

void setup() {
    pinMode(D7, OUTPUT);
    System.on(button_status | button_click | button_final_click, eventHandler);
    Particle.function("testClicks", testClicks);
}

void loop() {
}
