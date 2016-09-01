#include "application.h"

SYSTEM_MODE(MANUAL)
SYSTEM_THREAD(ENABLED)

LOG_SOURCE_CATEGORY("test")

namespace {

// Logging may hide race condition issues that are checked in this test, so it's commented out
//
// Serial1LogHandler logHandler(57600, LOG_LEVEL_WARN, {
//     { "test", LOG_LEVEL_INFO }
// });

const int TIMEOUT = 2000; // Timeout for entering/leaving listening mode
const int REPEAT_COUNT = 10;

bool listeningStarted = false;
bool listeningStopped = false;

bool testStarted = false;
bool testFinished = false;
int testCount = 0;

void onListeningChanged(system_event_t event, int data, void*) {
    switch (event) {
    case wifi_listen_begin:
        listeningStarted = true;
        LOG(TRACE, "Listening mode started");
        break;
    case wifi_listen_end:
        listeningStopped = true;
        LOG(TRACE, "Listening mode stopped");
        break;
    default:
        break;
    }
}

} // namespace

void setup() {
    // Set bogus WiFi credentials to cause repeated connection attempts
    WiFi.on();
    WiFi.clearCredentials();
    WiFi.setCredentials("param-pam-pam");
    WiFi.connect();

    System.on(wifi_listen_begin | wifi_listen_end, onListeningChanged);
}

void loop() {
    if (testFinished) {
        return;
    }

    if (!testStarted) {
        // Give the system thread some time to start a connection
        delay(3000);
        testStarted = true;
        LOG(INFO, "Test started");
    }

    LOG(TRACE, "%d / %d", testCount + 1, REPEAT_COUNT);

    bool failed = false;

    // Enter listening mode
    delay(50); // Gives the system thread some time to leave listening loop cleanly
    LOG(TRACE, "Starting listening mode");
    WiFi.listen();
    uint32_t t = millis();
    listeningStarted = false;
    do {
        Particle.process();
        if (millis() - t > TIMEOUT) {
            failed = true;
            break;
        }
    } while (!listeningStarted);

    if (!failed) {
        // Exit listening mode
        delay(50);
        LOG(TRACE, "Stopping listening mode");
        WiFi.listen(false);
        t = millis();
        listeningStopped = false;
        do {
            Particle.process();
            if (millis() - t > TIMEOUT) {
                failed = true;
                break;
            }
        } while (!listeningStopped);
    }

    if (!failed) {
        ++testCount;
        if (testCount == REPEAT_COUNT) {
            testFinished = true;
            RGB.control(true);
            RGB.color(0x00ff00); // Green
            LOG(INFO, "TEST SUCCEEDED");
        }
    } else {
        testFinished = true;
        RGB.control(true);
        RGB.color(0xff0000); // Red
        LOG(ERROR, "TEST FAILED");
    }
}
