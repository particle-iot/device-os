#include "application.h"

SYSTEM_THREAD(ENABLED);

namespace {

const Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});

const SerialLogHandler logHandler1(LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});


unsigned logMillis = 0;
unsigned logCount = 0;

bool logDiagnosticData(void* appender, const uint8_t* data, size_t size) {
    Log.write(LOG_LEVEL_INFO, (const char*)data, size);
    return true;
}

} // namespace

void setup() {
    logMillis = millis();
}

void loop() {
    if (millis() - logMillis >= 5000) {
        ++logCount;
        Log.printf(LOG_LEVEL_INFO, "%u: ", logCount);
        system_format_diag_data(nullptr, 0, 0, logDiagnosticData, nullptr, nullptr);
        Log.print(LOG_LEVEL_INFO, "\r\n");
        logMillis = millis();
    }
}
