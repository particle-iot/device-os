#include "application.h"
#include "unit-test/unit-test.h"
#include "util.h"

SYSTEM_MODE(MANUAL)

#if USE_THREADING == 1
SYSTEM_THREAD(ENABLED)
#endif

namespace {

using namespace particle::util;

const Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL },
    { "wiced", LOG_LEVEL_ALL }
});

} // namespace

// TODO: Add USB requests interface to the unit test library
extern bool requestStart;

bool usb_request_custom_handler(char* buf, size_t bufSize, size_t reqSize, size_t* repSize) {
    // Parse JSON request
    JSONString cmd, serverHost;
    unsigned serverEchoPort = 0, serverDiscardPort = 0, serverChargenPort = 0;
    JSONObjectIterator it(JSONValue::parse(buf, reqSize));
    while (it.next()) {
        const JSONString name = it.name();
        if (name == "cmd") {
            cmd = it.value().toString();
        } else if (name == "host") {
            serverHost = it.value().toString();
        } else if (name == "echoPort") {
            serverEchoPort = it.value().toInt();
        } else if (name == "discardPort") {
            serverDiscardPort = it.value().toInt();
        } else if (name == "chargenPort") {
            serverChargenPort = it.value().toInt();
        }
    }
    // Process request
    JSONBufferWriter json(buf, bufSize); // Reply JSON data
    json.beginObject();
    if (cmd == "start") {
        Log.info("Server address: %s", (const char*)serverHost);
        setServerHost((const char*)serverHost);
        setServerPort(ServerType::ECHO, serverEchoPort);
        setServerPort(ServerType::DISCARD, serverDiscardPort);
        setServerPort(ServerType::CHARGEN, serverChargenPort);
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
