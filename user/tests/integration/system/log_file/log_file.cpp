#include "application.h"
#include "test.h"

#if HAL_PLATFORM_LOG_FILE

#include "system_config.h"

namespace {

String readLog() {
    String s;
    int r = system_read_log_file(0 /* size */, [](const char* data, size_t size, void* arg) -> int {
        auto s = static_cast<String*>(arg);
        if (!s->concat(data, size)) {
            return Error::NO_MEMORY;
        }
        return 0;
    }, &s, nullptr /* reserved */);
    if (r < 0) {
        return String();
    }
    return s;
}

inline bool logContains(const char* substr, const String& log = readLog()) {
    return log.indexOf(substr) >= 0;
}

String repeat(char c, size_t count) {
    String s;
    if (!s.resize(count)) {
        return String();
    }
    std::memset(&s[0u], c, count);
    return s;
}

bool callbackCalled = false; // For waitFor()

} // namespace

test(01_init) {
    System.disableLogFile(); // Deletes the log files
    assertEqual(readLog().length(), 0);
}

test(02_enable_and_capture) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app")), 0);
    Log.info("message_A");
    assertTrue(logContains("message_A"));
}

test(03_level_filter) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app").level(LOG_LEVEL_WARN)), 0);
    assertEqual(System.clearLogFile(), 0);
    Log.info("message_A"); // Filtered out
    Log.warn("message_B"); // Stored
    auto log = readLog();
    assertFalse(logContains("message_A", log));
    assertTrue(logContains("message_B", log));
}

test(04_category_filter) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app")), 0);
    assertEqual(System.clearLogFile(), 0);
    Log.info("message_A"); // Filtered out
    Logger subLog("app.foo.bar");
    subLog.info("message_B"); // Stored
    auto log = readLog();
    assertFalse(logContains("message_A", log));
    assertTrue(logContains("message_B", log));
}

test(05_clear_and_keep_capturing) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app")), 0);
    assertEqual(System.clearLogFile(), 0);
    Log.info("message_A");
    assertTrue(logContains("message_A"));

    assertEqual(System.clearLogFile(), 0);
    assertEqual(readLog().length(), 0);

    // The log keeps capturing the messages after it's been cleared
    Log.info("message_B");
    assertTrue(logContains("message_B"));
}

test(06_rotation) {
    const size_t maxLogSize = 2000;

    assertEqual(System.enableLogFile(LogFileOptions().category("app").maxSize(maxLogSize)), 0);
    assertEqual(System.clearLogFile(), 0);

    String s = "a" + repeat('b', maxLogSize - 1);
    assertEqual(s.length(), maxLogSize);
    Log.print(s);
    assertEqual(readLog(), s);

    s = repeat('b', maxLogSize - 1) + "c";
    assertEqual(s.length(), maxLogSize);
    Log.print("c");
    assertEqual(readLog(), s);
}

test(07_dropped_bytes_reported) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app").bufferSize(10)), 0);
    assertEqual(System.clearLogFile(), 0);

    Log.print("aaaaaaaaaa");
    Log.print("b");
    assertTrue(logContains("dropped 1 bytes of log data"));
}

test(08_system_thread_auto_flush) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app").bufferSize(10)), 0);
    assertEqual(System.clearLogFile(), 0);

    callbackCalled = false;
    system_thread_invoke([](void* arg) {
        Log.print("aaaaaaaaaa");
        Log.print("bbbbbbbbbb");
        callbackCalled = true;
    }, nullptr /* arg */, nullptr /* reserved */);
    assertTrue(waitFor([]() {
        return callbackCalled;
    }, 10000));

    assertTrue(readLog() == "aaaaaaaaaabbbbbbbbbb");
}

test(09_read_size_limit) {
    assertEqual(System.enableLogFile(LogFileOptions().category("app").maxSize(10)), 0);
    assertEqual(System.clearLogFile(), 0);
    Log.print("aaaaabbbbb");

    int r = system_read_log_file(5, [](const char* data, size_t size, void* arg) {
        if (size != 5 || std::memcmp(data, "bbbbb", 5) != 0) {
            return -1;
        }
        return 0;
    }, nullptr /* arg */, nullptr /* reserved */);
    assertEqual(r, 5);
}

test(10_enable_and_reset) {
    assertEqual(System.enableLogFile(), 0);
    assertEqual(System.clearLogFile(), 0);

    Log.info("message_A");
    expectSystemReset();
    System.reset();
}

test(11_log_survived_reset) {
    Log.info("message_B");

    auto log = readLog();
    assertTrue(logContains("message_A", log));
    assertTrue(logContains("~~~~~~~~~~", log)); // Boot separator
    assertTrue(logContains("message_B", log));
}

test(12_disable_and_reset) {
    System.disableLogFile();
    expectSystemReset();
    System.reset();
}

test(13_still_disabled_after_reset) {
    int r = system_read_log_file(100 /* size */, [](const char* data, size_t size, void* arg) {
        return 0;
    }, nullptr /* arg */, nullptr /* reserved */);
    assertTrue(r == SYSTEM_ERROR_INVALID_STATE);
}

test(99_cleanup) {
    System.disableLogFile();
}

#endif // HAL_PLATFORM_LOG_FILE
