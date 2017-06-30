#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(MANUAL)

#if USE_THREADING == 1
SYSTEM_THREAD(ENABLED)
#endif

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL },
    { "wiced", LOG_LEVEL_ALL }
});

String serverHost;
unsigned serverPort = 0;

// TODO: Add USB requests interface to the unit test library
extern bool requestStart;

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    // Parse JSON request
    JSONString cmd, host;
    unsigned port = 0;
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        const JSONString name = it.name();
        if (name == "cmd") {
            cmd = it.value().toString();
        } else if (name == "host") {
            host = it.value().toString();
        } else if (name == "port") {
            port = it.value().toInt();
        }
    }
    // Process request
    JSONBufferWriter json(buf, bufSize); // Reply JSON data
    json.beginObject();
    if (cmd == "start") {
        Log.info("Server address: %s:%u", (const char*)host, (unsigned)port);
        serverHost = (String)host;
        serverPort = port;
        requestStart = true; // Start testing
    } else if (cmd == "status") {
        json.name("done").value(_runner.isComplete());
        if (_runner.isComplete()) {
            json.name("passed").value(_runner.isPassed());
        }
    }
    json.endObject();
    *repSize = json.dataSize();
    return true;
}

void setup() {
    WiFi.connect();
    waitUntil(WiFi.ready);
    // We need to initialize random seed value manually, since we don't connect to the cloud in this test
    randomSeed(HAL_RNG_GetRandomNumber());
    unit_test_setup();
}

void loop() {
    unit_test_loop();
}
