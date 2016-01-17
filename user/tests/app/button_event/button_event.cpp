#include "application.h"

static bool buttonStatusEvent = false,
        buttonDoubleClickEvent = false;
static uint32_t buttonStatusEventTime = 0;

void systemEventHandler(system_event_t event, uint32_t data, void*) {
    switch (event) {
    case button_status:
        buttonStatusEvent = true;
        buttonStatusEventTime = data;
        break;
    case button_double_click:
        buttonDoubleClickEvent = true;
        break;
    default:
        break;
    }
}

int testClick() {
    // Waiting the button to be pressed in next 5 seconds
    buttonStatusEvent = false;
    uint32_t t = millis();
    do {
        Particle.process();
    } while (!buttonStatusEvent && millis() - t < 5000);
    if (!buttonStatusEvent || buttonStatusEventTime != 0) {
        return 1;
    }
    // Waiting for the button to be released in next second
    buttonStatusEvent = false;
    buttonStatusEventTime = 0;
    t = millis();
    do {
        Particle.process();
    } while (!buttonStatusEvent && millis() - t < 1000);
    if (!buttonStatusEvent || buttonStatusEventTime == 0 || buttonStatusEventTime > 1000) {
        return 1;
    }
    return 0;
}

int testDoubleClick() {
    // Waiting for the button to be double-clicked in next 5 seconds
    buttonDoubleClickEvent = false;
    uint32_t t = millis();
    do {
        Particle.process();
    } while (!buttonDoubleClickEvent && millis() - t < 5000);
    if (!buttonDoubleClickEvent) {
        return 1;
    }
    return 0;
}

int test(String name) {
    int result = 1;
    if (name == "click") {
        result = testClick();
    } else if (name == "doubleClick") {
        result = testDoubleClick();
    }
    return result;
}

void setup() {
    System.on(button_status | button_double_click, systemEventHandler);
    Particle.function("test", test);
}

void loop() {
}
