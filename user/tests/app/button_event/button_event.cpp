#include "application.h"

#include <deque>

struct Event {
    system_event_t type;
    uint32_t data;

    explicit Event(system_event_t type, uint32_t data = 0) :
            type(type),
            data(data) {
    }
};

const uint32_t testTimeout = 5000;

std::deque<Event> eventQueue;
uint32_t testStarted = 0;
uint32_t eventData = 0;

#define WAIT_EVENT(event) \
        do { \
            while (eventQueue.empty() && millis() - testStarted < testTimeout) { \
                Particle.process(); \
            } \
            if (eventQueue.empty() || eventQueue.front().type != (event)) { \
                return 1; \
            } \
            eventData = eventQueue.front().data; \
            eventQueue.pop_front(); \
        } while (false)

void eventHandler(system_event_t event, uint32_t data, void*) {
    eventQueue.push_back(Event(event, data));
}

void resetTest() {
    eventQueue = std::deque<Event>();
    testStarted = millis();
}

int testClicks(String arg) {
    resetTest();
    const int expectedClicks = arg.toInt();
    if (expectedClicks <= 0) {
        return 1;
    }
    int clicks = 0;
    do {
        // Waiting for button press event
        WAIT_EVENT(button_status);
        if (eventData != 0) {
            return 1;
        }
        // Waiting for button release event
        WAIT_EVENT(button_status);
        if (eventData == 0 || eventData > 1000) {
            return 1;
        }
        // Waiting for button click event
        WAIT_EVENT(button_click);
        if ((int)eventData != clicks + 1) {
            return 1;
        }
        ++clicks;
        if (clicks == expectedClicks) {
            // Waiting for final click event
            WAIT_EVENT(button_final_click);
            if ((int)eventData != clicks) {
                return 1;
            }
        }
    } while (clicks != expectedClicks && millis() - testStarted < testTimeout);
    if (clicks != expectedClicks) {
        return 1;
    }
    return 0;
}

void setup() {
    System.on(button_status | button_click | button_final_click, eventHandler);
    Particle.function("testClicks", testClicks);
}

void loop() {
}
