
#include "application.h"
#include "unit-test/unit-test.h"

class RecursiveLogger : LogHandler
{
private:

    volatile bool active;
    int count;

public:

    RecursiveLogger(LogLevel level = LOG_LEVEL_INFO) : LogHandler(level)
    {
        active = false;
        count = 0;
    }

    void addHandler() {
        LogManager::instance()->addHandler(this);
    }

    void removeHandler() {
        LogManager::instance()->removeHandler(this);
    }

    int getCount() {
        return count;
    }

    void logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr)
    {
        if (active) {
            // Fail if we are allowed to re-enter this logging handler, as the count won't increase
            count--;
            return;
        }
        active = true;

        // Serial.printlnf("logMessage: %d", count); // DEBUG
        if (count++ >= 5) {
            Log.info("force re-entry into this logMessage function");
        }

        active = false;
    }
};

// SerialLogHandler logHandler(LOG_LEVEL_ALL);  // DEBUG

/**
 * This tests two things related to Issue #1497: https://github.com/particle-iot/firmware/issues/1497
 * 1) that the application thread doesn't lock up waiting for a logger mutex to clear
 * 2) that the logMessage function isn't allowed to be called in a recursive manner
 */
test(LOGGING_01_recursive_log_handling)
{
    Serial.println("IF THIS HANGS HERE, THE TEST FAILED!");
    RecursiveLogger recursiveLogger;
    recursiveLogger.addHandler();
    for (int x=0; x<10; x++) {
        Log.info("recursive logging %lu", millis());
    }
    recursiveLogger.removeHandler();
    assertEqual(recursiveLogger.getCount(), 10);
}
