#include "application.h"

SYSTEM_THREAD(ENABLED);

static volatile uint32_t s_sleep = 0;
static volatile uint32_t s_connected = 0;
static char s_temp[65] = {0};

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    // Parse JSON request
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        if (it.name() == "sleep") {
            s_sleep = 1;
        } else if (it.name() == "data") {
            if (strlen(s_temp) && strlen(s_temp) <= bufSize) {
                memcpy(buf, s_temp, strlen(s_temp));
                *repSize = strlen(s_temp);
                return true;
            }
        }
    }
    // Do not reply unless we are connected
    return s_connected;
}

void onEvent(const char* event, const char* data) {
    ATOMIC_BLOCK() {
        sprintf(s_temp, "%s", data);
    }
}

void setup() {
    Particle.subscribe("emdmpause", onEvent, MY_DEVICES);
}

void loop() {
    if (Particle.connected())
        s_connected = 1;
    if (s_sleep) {
        s_sleep = 0;
        System.sleep(65535, RISING, 30, SLEEP_NETWORK_STANDBY);
    }
}
