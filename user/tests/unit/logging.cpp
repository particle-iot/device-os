#include <iostream>
#include <queue>

#define CATCH_CONFIG_PREFIX_ALL
#include "catch.hpp"

#include "spark_wiring_logging.h"

// Global logging category
LOG_CATEGORY("test");

namespace {

using namespace spark;

class LogMessage {
public:
    LogMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category, const char *file, int line, const char *func) :
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

    const LogMessage& checkLevel(LoggerOutputLevel level) const {
        CATCH_CHECK(level_ == level);
        return *this;
    }

    const LogMessage& checkTime(uint32_t time) const {
        CATCH_CHECK(time_ == time);
        return *this;
    }

    const LogMessage& checkCategory(const std::string &category) const {
        CATCH_CHECK(cat_ == category);
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
    LoggerOutputLevel level_;
    uint32_t time_;
    int line_;
};

class TestLogger: public FormattingLogger {
public:
    explicit TestLogger(LoggerOutputLevel level = ALL_LEVEL, const CategoryFilters &filters = {}):
            FormattingLogger(level, filters) {
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

    void clear() {
        msgs_ = std::queue<LogMessage>();
        buf_ = std::string();
    }

    bool isEmpty() const {
        return msgs_.empty() && buf_.empty();
    }

protected:
    // spark::Logger
    virtual void writeMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func) override {
        const LogMessage m(msg, level, time, category, file, line, func);
        msgs_.push(m);
        FormattingLogger::writeMessage(msg, level, time, category, file, line, func);
    }

    virtual void writeString(const char *str, LoggerOutputLevel) override {
        // std::cout << str;
        buf_.append(str);
    }

private:
    std::queue<LogMessage> msgs_;
    std::string buf_;
};

} // namespace

CATCH_TEST_CASE("Basic logging") {
    TestLogger log(ALL_LEVEL);
    LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
    log.next().checkMessage("log").checkLevel(LOG_LEVEL);
    log.next().checkMessage("debug").checkLevel(DEBUG_LEVEL);
    log.next().checkMessage("info").checkLevel(INFO_LEVEL);
    log.next().checkMessage("warn").checkLevel(WARN_LEVEL);
    log.next().checkMessage("error").checkLevel(ERROR_LEVEL);
}

CATCH_TEST_CASE("Basic filtering") {
    CATCH_SECTION("warn") {
        TestLogger log(WARN_LEVEL);
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkLevel(WARN_LEVEL).checkMessage("warn");
        log.next().checkLevel(ERROR_LEVEL).checkMessage("error");
        CATCH_CHECK(!log.hasNext()); // "log", "debug" and "info" have been filtered out
    }
    CATCH_SECTION("none") {
        TestLogger log(NO_LOG_LEVEL);
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        CATCH_CHECK(!log.hasNext());
    }
}

CATCH_TEST_CASE("Category filtering") {
    TestLogger log(ERROR_LEVEL, {
        { "b.b", INFO_LEVEL },
        { "a", WARN_LEVEL },
        { "a.a.a", DEBUG_LEVEL },
        { "a.a", INFO_LEVEL }
    });
    CATCH_SECTION("a") {
        LOG_CATEGORY("a");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log", "debug" and "info" have been filtered out
    }
    CATCH_SECTION("a.a") {
        LOG_CATEGORY("a.a");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a.a").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("a.a").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a.a").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" and "debug" have been filtered out
    }
    CATCH_SECTION("a.a.a") {
        LOG_CATEGORY("a.a.a");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a.a.a").checkMessage("debug").checkLevel(DEBUG_LEVEL);
        log.next().checkCategory("a.a.a").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("a.a.a").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a.a.a").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" has been filtered out
    }
    CATCH_SECTION("a.x") {
        LOG_CATEGORY("a.x");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a.x").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a.x").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log", "debug" and "info" have been filtered out (according to filter for "a" category)
    }
    CATCH_SECTION("a.a.x") {
        LOG_CATEGORY("a.a.x");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a.a.x").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("a.a.x").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a.a.x").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" and "debug" have been filtered out (according to filter for "a.a" category)
    }
    CATCH_SECTION("a.a.a.x") {
        LOG_CATEGORY("a.a.a.x");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("a.a.a.x").checkMessage("debug").checkLevel(DEBUG_LEVEL);
        log.next().checkCategory("a.a.a.x").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("a.a.a.x").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("a.a.a.x").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" has been filtered out (according to filter for "a.a.a" category)
    }
    CATCH_SECTION("b") {
        LOG_CATEGORY("b");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("b").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // Everything except "error" has been filtered out (default logging level)
    }
    CATCH_SECTION("b.x") {
        LOG_CATEGORY("b.x");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("b.x").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // Everything except "error" has been filtered out (default logging level)
    }
    CATCH_SECTION("b.b") {
        LOG_CATEGORY("b.b");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("b.b").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("b.b").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("b.b").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" and "debug" have been filtered out
    }
    CATCH_SECTION("b.b.x") {
        LOG_CATEGORY("b.b.x");
        LOG("log"); DEBUG("debug"); INFO("info"); WARN("warn"); ERROR("error");
        log.next().checkCategory("b.b.x").checkMessage("info").checkLevel(INFO_LEVEL);
        log.next().checkCategory("b.b.x").checkMessage("warn").checkLevel(WARN_LEVEL);
        log.next().checkCategory("b.b.x").checkMessage("error").checkLevel(ERROR_LEVEL);
        CATCH_CHECK(!log.hasNext()); // "log" and "debug" have been filtered out (according to filter for "b.b" category)
    }
}
