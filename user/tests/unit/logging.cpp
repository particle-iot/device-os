#include <iostream>
#include <queue>

#define CATCH_CONFIG_PREFIX_ALL
#include "catch.hpp"

#include "spark_wiring_logging.h"
#include "service_debug.h"

namespace {

using namespace spark;

class LogMessage {
public:
    LogMessage(const char *msg, LogLevel level, const char *category, uint32_t time, const char *file, int line, const char *func) :
            msg_(msg ? msg : ""),
            cat_(category ? category : ""),
            file_(file ? file : ""),
            func_(func ? func : ""),
            level_(level),
            time_(time),
            line_(line) {
    }

    const LogMessage& checkMessage(const std::string &msg) const {
        CATCH_CHECK(msg_ == msg);
        return *this;
    }

    const LogMessage& checkLevel(LogLevel level) const {
        CATCH_CHECK(level_ == level);
        return *this;
    }

    const LogMessage& checkCategory(const std::string &category) const {
        CATCH_CHECK(cat_ == category);
        return *this;
    }

    const LogMessage& checkTime(uint32_t time) const {
        CATCH_CHECK(time_ == time);
        return *this;
    }

    const LogMessage& checkFile(const std::string &file) const {
        CATCH_CHECK(file_ == file);
        return *this;
    }

    const LogMessage& checkLine(int line) const {
        CATCH_CHECK(line_ == line);
        return *this;
    }

    const LogMessage& checkFunction(const std::string &func) const {
        CATCH_CHECK(func_ == func);
        return *this;
    }

private:
    std::string msg_, cat_, file_, func_;
    LogLevel level_;
    uint32_t time_;
    int line_;
};

class TestLogger: public Logger {
public:
    explicit TestLogger(LogLevel level = ALL_LEVEL, const Filters &filters = {}):
            Logger(level, filters) {
        Logger::install(this);
    }

    virtual ~TestLogger() {
        Logger::uninstall(this);
    }

    LogMessage next() {
        CATCH_REQUIRE(!msgs_.empty());
        const LogMessage m = msgs_.front();
        msgs_.pop();
        return m;
    }

    bool hasNext() const {
        return !msgs_.empty();
    }

    const std::string& buffer() const {
        return buf_;
    }

    const TestLogger& checkBuffer(const std::string &data) const {
        CATCH_CHECK(buf_ == data);
        return *this;
    }

    void clear() {
        msgs_ = std::queue<LogMessage>();
        buf_ = std::string();
    }

    bool isEmpty() const {
        return msgs_.empty() && buf_.empty();
    }

protected:
    // spark::Logger
    virtual void formatMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func) override {
        const LogMessage m(msg, level, category, time, file, line, func);
        msgs_.push(m);
    }

    virtual void write(const char *data, size_t size) override {
        buf_.append(data, size);
    }

private:
    std::queue<LogMessage> msgs_;
    std::string buf_;
};

} // namespace

// Global logging category
LOG_SOURCE_CATEGORY("global");

CATCH_TEST_CASE("Basic logging") {
    CATCH_SECTION("message") {
        TestLogger log(ALL_LEVEL);
        LOG(TRACE, "trace");
        log.next().checkMessage("trace").checkLevel(TRACE_LEVEL);
        LOG(INFO, "info");
        log.next().checkMessage("info").checkLevel(INFO_LEVEL);
        LOG(WARN, "warn");
        log.next().checkMessage("warn").checkLevel(WARN_LEVEL);
        LOG(ERROR, "error");
        log.next().checkMessage("error").checkLevel(ERROR_LEVEL);
    }
    CATCH_SECTION("stream") {
        TestLogger log(ALL_LEVEL);
        LOG_WRITE(TRACE, "a", 1);
        LOG_PRINT(INFO, "b");
        LOG_FORMAT(WARN, "%s", "c");
        log.checkBuffer("abc");
    }
    CATCH_SECTION("dump") {
        TestLogger log(ALL_LEVEL);
        const uint8_t data[] = {
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
        };
        LOG_DUMP(INFO, &data, sizeof(data));
        log.checkBuffer("00112233445566778899aabbccddeeff");
    }
}

CATCH_TEST_CASE("Basic logging (compatibility)") {
    CATCH_SECTION("message") {
        TestLogger log(ALL_LEVEL);
        DEBUG("debug");
        log.next().checkMessage("debug").checkLevel(DEBUG_LEVEL);
        INFO("info");
        log.next().checkMessage("info").checkLevel(INFO_LEVEL);
        WARN("warn");
        log.next().checkMessage("warn").checkLevel(WARN_LEVEL);
        ERROR("error");
        log.next().checkMessage("error").checkLevel(ERROR_LEVEL);
    }
    CATCH_SECTION("stream") {
        TestLogger log(ALL_LEVEL);
        DEBUG_D("%s", "abc");
        log.checkBuffer("abc");
    }
}

CATCH_TEST_CASE("Basic filtering") {
    CATCH_SECTION("warn") {
        TestLogger log(WARN_LEVEL); // TRACE and INFO should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("cd");
    }
    CATCH_SECTION("none") {
        TestLogger log(NO_LOG_LEVEL); // All levels should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && !LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("");
    }
}

CATCH_TEST_CASE("Scoped category") {
    TestLogger log(ALL_LEVEL);
    LOG(INFO, "");
    log.next().checkCategory("global");
    {
        LOG_CATEGORY("local");
        LOG(INFO, "");
        log.next().checkCategory("local");
    }
    LOG(INFO, "");
    log.next().checkCategory("global");
}

CATCH_TEST_CASE("Category filtering") {
    TestLogger log(ERROR_LEVEL, {
        { "b.b", INFO_LEVEL },
        { "a", WARN_LEVEL },
        { "a.a.a", TRACE_LEVEL },
        { "a.a", INFO_LEVEL }
    });
    CATCH_SECTION("a") {
        LOG_CATEGORY("a"); // TRACE and INFO should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("cd");
    }
    CATCH_SECTION("a.a") {
        LOG_CATEGORY("a.a"); // TRACE should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("bcd");
    }
    CATCH_SECTION("a.a.a") {
        LOG_CATEGORY("a.a.a"); // No messages should be filtered out
        CATCH_CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(TRACE_LEVEL);
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("abcd");
    }
    CATCH_SECTION("a.x") {
        LOG_CATEGORY("a.x"); // TRACE and INFO should be filtered out (according to filter set for "a" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("cd");
    }
    CATCH_SECTION("a.a.x") {
        LOG_CATEGORY("a.a.x"); // TRACE should be filtered out (according to filter set for "a.a" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("bcd");
    }
    CATCH_SECTION("a.a.a.x") {
        LOG_CATEGORY("a.a.a.x"); // No messages should be filtered out (according to filter set for "a.a.a" category)
        CATCH_CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(TRACE_LEVEL);
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("abcd");
    }
    CATCH_SECTION("b") {
        LOG_CATEGORY("b"); // All levels except ERROR should be filtered out (default level)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("d");
    }
    CATCH_SECTION("b.x") {
        LOG_CATEGORY("b.x"); // All levels except ERROR should be filtered out (default level)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("d");
    }
    CATCH_SECTION("b.b") {
        LOG_CATEGORY("b.b"); // TRACE should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("bcd");
    }
    CATCH_SECTION("b.b.x") {
        LOG_CATEGORY("b.b.x"); // TRACE should be filtered out (according to filter set for "b.b" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().checkLevel(INFO_LEVEL);
        log.next().checkLevel(WARN_LEVEL);
        log.next().checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.checkBuffer("bcd");
    }
}

CATCH_TEST_CASE("Malformed category name") {
    TestLogger log(ERROR_LEVEL, {
        { "a", WARN_LEVEL },
        { "a.a", INFO_LEVEL }
    });
    CATCH_SECTION("empty") {
        LOG_CATEGORY(""); // All levels except ERROR should be filtered out (default level)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    CATCH_SECTION(".") {
        LOG_CATEGORY("."); // ditto
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    CATCH_SECTION(".a") {
        LOG_CATEGORY(".a"); // ditto
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    CATCH_SECTION("a.") {
        LOG_CATEGORY("a."); // TRACE and INFO should be filtered out (according to filter set for "a" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    CATCH_SECTION("a..a") {
        LOG_CATEGORY("a."); // ditto
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
}

CATCH_TEST_CASE("Miscellaneous") {
    CATCH_SECTION("Exact subcategory match") {
        TestLogger log(ERROR_LEVEL, {
            { "aaa", TRACE_LEVEL },
            { "aa", INFO_LEVEL },
            { "a", WARN_LEVEL }
        });
        CATCH_CHECK(LOG_ENABLED_C(WARN, "a"));
        CATCH_CHECK(LOG_ENABLED_C(INFO, "aa"));
        CATCH_CHECK(LOG_ENABLED_C(TRACE, "aaa"));
        CATCH_CHECK(LOG_ENABLED_C(ERROR, "x"));
    }
}
