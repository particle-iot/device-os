#include "application.h"

SYSTEM_MODE(MANUAL);

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    // Parse JSON request
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        if (it.name() == "test") {
            const char str[] = "It works!";
            memcpy(buf, str, strlen(str));
            *repSize = strlen(str);
            return true;
        }
    }
    return false;
}


void setup() {
    Serial.end();
}

void loop() {
}
