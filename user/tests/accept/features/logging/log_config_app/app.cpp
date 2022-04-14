#include "application.h"

SYSTEM_MODE(MANUAL)

// Enable dynamically configurable logging
STARTUP(System.enableFeature(FEATURE_CONFIGURABLE_LOGGING))

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    const char* msg = "Test message";
    const char* cat = "app"; // Category name
    LogLevel level = LOG_LEVEL_INFO;
    // Parse JSON request
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        if (it.name() == "msg") {
            msg = (const char*)it.value().toString();
        } else if (it.name() == "cat") {
            cat = (const char*)it.value().toString();
        } else if (it.name() == "level") {
            JSONString s = it.value().toString();
            if (s == "trace") {
                level = LOG_LEVEL_TRACE;
            } else if (s == "info") {
                level = LOG_LEVEL_INFO;
            } else if (s == "warn") {
                level = LOG_LEVEL_WARN;
            } else if (s == "error") {
                level = LOG_LEVEL_ERROR;
            }
        }
    }
    // Generate log message
    Logger log(cat);
    log(level, "%s", msg);
    return true;
}

void setup() {
}

void loop() {
}
