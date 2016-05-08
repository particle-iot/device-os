#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

#include <iostream>
#include <random>
#include <queue>

#define CATCH_CONFIG_PREFIX_ALL
#include "catch.hpp"

#define LOG_INCLUDE_SOURCE_INFO
#include "spark_wiring_logging.h"
#include "service_debug.h"

namespace {

using namespace spark;

class LogMessage {
public:
    LogMessage(const char *msg, LogLevel level, const char *category, uint32_t time, const LogHandler::SourceInfo &info) :
            msg_(msg ? msg : ""),
            cat_(category ? category : ""),
            file_(info.file ? info.file : ""),
            func_(info.function ? info.function : ""),
            level_(level),
            time_(time),
            line_(info.line) {
    }

    const LogMessage& messageEquals(const std::string &msg) const {
        CATCH_CHECK(msg_ == msg);
        return *this;
    }

    const LogMessage& levelEquals(LogLevel level) const {
        CATCH_CHECK(level_ == level);
        return *this;
    }

    const LogMessage& categoryEquals(const std::string &category) const {
        CATCH_CHECK(cat_ == category);
        return *this;
    }

    const LogMessage& timeEquals(uint32_t time) const {
        CATCH_CHECK(time_ == time);
        return *this;
    }

    const LogMessage& fileEquals(const std::string &file) const {
        CATCH_CHECK(file_ == file);
        return *this;
    }

    const LogMessage& lineEquals(int line) const {
        CATCH_CHECK(line_ == line);
        return *this;
    }

    const LogMessage& functionEquals(const std::string &func) const {
        CATCH_CHECK(func_ == func);
        return *this;
    }

private:
    std::string msg_, cat_, file_, func_;
    LogLevel level_;
    uint32_t time_;
    int line_;
};

class TestHandler: public LogHandler {
public:
    explicit TestHandler(LogLevel level = LOG_LEVEL_ALL, const Filters &filters = {}):
            LogHandler(level, filters) {
        LogHandler::install(this);
    }

    virtual ~TestHandler() {
        LogHandler::uninstall(this);
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

    const TestHandler& bufferEquals(const std::string &str) const {
        CATCH_CHECK(buf_ == str);
        return *this;
    }

    const TestHandler& bufferEndsWith(const std::string &str) const {
        const std::string s = (buf_.size() >= str.size()) ? buf_.substr(buf_.size() - str.size(), str.size()) : str;
        CATCH_CHECK(str == s);
        return *this;
    }

protected:
    // spark::LogHandler
    virtual void logMessage(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info) override {
        msgs_.push(LogMessage(msg, level, category, time, info));
    }

    virtual void write(const char *data, size_t size) override {
        buf_.append(data, size);
    }

private:
    std::queue<LogMessage> msgs_;
    std::string buf_;
};

// Log handler using compatibility callback
class CompatHandler {
public:
    explicit CompatHandler(LogLevel level = LOG_LEVEL_ALL) {
        instance = this;
        set_logger_output(callback, level);
    }

    ~CompatHandler() {
        set_logger_output(nullptr, LOG_LEVEL_NONE);
        instance = nullptr;
    }

    const std::string& buffer() const {
        return buf_;
    }

    const CompatHandler& bufferEquals(const std::string &str) const {
        CATCH_CHECK(buf_ == str);
        return *this;
    }

    const CompatHandler& bufferEndsWith(const std::string &str) const {
        const std::string s = (buf_.size() >= str.size()) ? buf_.substr(buf_.size() - str.size(), str.size()) : str;
        CATCH_CHECK(str == s);
        return *this;
    }

private:
    std::string buf_;

    static CompatHandler *instance;

    static void callback(const char *str) {
        instance->buf_.append(str);
    }
};

std::string randomString(size_t size = 0) {
    static const std::string chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()`~-_=+[{]}\\|;:'\",<.>/? ");
    std::random_device rd;
    if (!size) {
        size = rd() % LOG_MAX_STRING_LENGTH + 1;
    }
    std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        s += chars[dist(rd)];
    }
    return s;
}

std::string randomBytes(size_t size = 0) {
    std::random_device rd;
    if (!size) {
        size = rd() % (LOG_MAX_STRING_LENGTH / 2) + 1;
    }
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    std::string s;
    s.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        s += (char)dist(rd);
    }
    return s;
}

std::string sourceFileName() {
    const std::string s(__FILE__);
    const size_t pos = s.rfind('/');
    if (pos != std::string::npos) {
        return s.substr(pos + 1);
    }
    return s;
}

std::string toHex(const std::string &str) {
    std::string s(boost::algorithm::hex(str));
    boost::algorithm::to_lower(s); // Logging library uses lower case
    return s;
}

CompatHandler* CompatHandler::instance = nullptr;

const std::string fileName = sourceFileName();

} // namespace

// Global logging category
LOG_SOURCE_CATEGORY("global");

CATCH_TEST_CASE("Message logging") {
    TestHandler log(LOG_LEVEL_ALL);
    CATCH_SECTION("attributes") {
        LOG(TRACE, "trace");
        log.next().messageEquals("trace").levelEquals(LOG_LEVEL_TRACE).fileEquals(fileName);
        LOG(INFO, "info");
        log.next().messageEquals("info").levelEquals(LOG_LEVEL_INFO).fileEquals(fileName);
        LOG(WARN, "warn");
        log.next().messageEquals("warn").levelEquals(LOG_LEVEL_WARN).fileEquals(fileName);
        LOG(ERROR, "error");
        log.next().messageEquals("error").levelEquals(LOG_LEVEL_ERROR).fileEquals(fileName);
    }
    CATCH_SECTION("formatting") {
        std::string s = "";
        LOG(TRACE, "%s", s.c_str());
        log.next().messageEquals("");
        s = randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG(INFO, "%s", s.c_str());
        log.next().messageEquals(s);
        s = randomString(LOG_MAX_STRING_LENGTH);
        LOG(WARN, "%s", s.c_str());
        log.next().messageEquals(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG(ERROR, "%s", s.c_str());
        log.next().messageEquals(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    CATCH_SECTION("compatibility macros") {
        DEBUG("debug"); // Alias for LOG_DEBUG(TRACE, ...)
#ifdef DEBUG_BUILD
        log.next().messageEquals("debug").levelEquals(LOG_LEVEL_TRACE);
#endif
        INFO("info"); // Alias for LOG(INFO, ...)
        log.next().messageEquals("info").levelEquals(LOG_LEVEL_INFO);
        WARN("warn"); // Alias for LOG(WARN, ...)
        log.next().messageEquals("warn").levelEquals(LOG_LEVEL_WARN);
        ERROR("error"); // Alias for LOG(ERROR, ...)
        log.next().messageEquals("error").levelEquals(LOG_LEVEL_ERROR);
    }
}

CATCH_TEST_CASE("Message logging (compatibility callback)") {
    CompatHandler log(LOG_LEVEL_ALL);
    CATCH_SECTION("attributes") {
        LOG(TRACE, "trace");
        log.bufferEndsWith("TRACE: trace\r\n");
        LOG(INFO, "info");
        log.bufferEndsWith("INFO: info\r\n");
        LOG(WARN, "warn");
        log.bufferEndsWith("WARN: warn\r\n");
        LOG(ERROR, "error");
        log.bufferEndsWith("ERROR: error\r\n");
        CATCH_CHECK(log.buffer().find(fileName) != std::string::npos);
    }
    CATCH_SECTION("formatting") {
        std::string s = randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG(INFO, "%s", s.c_str());
        log.bufferEndsWith(s + "\r\n");
        s = randomString(LOG_MAX_STRING_LENGTH);
        LOG(WARN, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + "~\r\n"); // 1 character is reserved for term. null
        s = randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG(ERROR, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + "~\r\n");
    }
}

CATCH_TEST_CASE("Direct logging") {
    TestHandler log(LOG_LEVEL_ALL);
    CATCH_SECTION("write") {
        std::string s = "";
        LOG_WRITE(INFO, s.c_str(), s.size());
        log.bufferEquals("");
        s = randomString();
        LOG_WRITE(WARN, s.c_str(), s.size());
        log.bufferEndsWith(s);
        s = randomString();
        LOG_PRINT(ERROR, s.c_str()); // Alias for LOG_WRITE(level, str, strlen(str))
        log.bufferEndsWith(s);
    }
    CATCH_SECTION("printf") {
        std::string s = "";
        LOG_PRINTF(TRACE, "%s", s.c_str());
        log.bufferEquals("");
        s = randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_PRINTF(INFO, "%s", s.c_str());
        log.bufferEquals(s);
        s = randomString(LOG_MAX_STRING_LENGTH);
        LOG_PRINTF(WARN, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_PRINTF(ERROR, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    CATCH_SECTION("dump") {
        std::string s = "";
        LOG_DUMP(TRACE, s.c_str(), s.size());
        log.bufferEquals("");
        s = randomBytes(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_DUMP(INFO, s.c_str(), s.size());
        log.bufferEquals(toHex(s));
        s = randomBytes(LOG_MAX_STRING_LENGTH);
        LOG_DUMP(WARN, s.c_str(), s.size());
        log.bufferEndsWith(toHex(s));
        s = randomBytes(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_DUMP(ERROR, s.c_str(), s.size());
        log.bufferEndsWith(toHex(s));
    }
    CATCH_SECTION("compatibility macros") {
        std::string s = randomString();
        DEBUG_D("%s", s.c_str()); // Alias for LOG_DEBUG_PRINTF(TRACE, ...)
#ifdef DEBUG_BUILD
        log.bufferEquals(s);
#endif
    }
}

// Copy-pase of above test case with TestHandler replaced with CompatHandler
CATCH_TEST_CASE("Direct logging (compatibility callback)") {
    CompatHandler log(LOG_LEVEL_ALL);
    CATCH_SECTION("write") {
        std::string s = "";
        LOG_WRITE(INFO, s.c_str(), s.size());
        log.bufferEquals("");
        s = randomString();
        LOG_WRITE(WARN, s.c_str(), s.size());
        log.bufferEndsWith(s);
        s = randomString();
        LOG_PRINT(ERROR, s.c_str()); // Alias for LOG_WRITE(level, str, strlen(str))
        log.bufferEndsWith(s);
    }
    CATCH_SECTION("printf") {
        std::string s = "";
        LOG_PRINTF(TRACE, "%s", s.c_str());
        log.bufferEquals("");
        s = randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_PRINTF(INFO, "%s", s.c_str());
        log.bufferEquals(s);
        s = randomString(LOG_MAX_STRING_LENGTH);
        LOG_PRINTF(WARN, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_PRINTF(ERROR, "%s", s.c_str());
        log.bufferEndsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    CATCH_SECTION("dump") {
        std::string s = "";
        LOG_DUMP(TRACE, s.c_str(), s.size());
        log.bufferEquals("");
        s = randomBytes(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_DUMP(INFO, s.c_str(), s.size());
        log.bufferEquals(toHex(s));
        s = randomBytes(LOG_MAX_STRING_LENGTH);
        LOG_DUMP(WARN, s.c_str(), s.size());
        log.bufferEndsWith(toHex(s));
        s = randomBytes(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_DUMP(ERROR, s.c_str(), s.size());
        log.bufferEndsWith(toHex(s));
    }
    CATCH_SECTION("compatibility macros") {
        std::string s = randomString();
        DEBUG_D("%s", s.c_str()); // Alias for LOG_DEBUG_PRINTF(TRACE, ...)
#ifdef DEBUG_BUILD
        log.bufferEquals(s);
#endif
    }
}

CATCH_TEST_CASE("Basic filtering") {
    CATCH_SECTION("warn") {
        TestHandler log(LOG_LEVEL_WARN); // TRACE and INFO should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("cd");
    }
    CATCH_SECTION("none") {
        TestHandler log(LOG_LEVEL_NONE); // All levels should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && !LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("");
    }
}

CATCH_TEST_CASE("Basic filtering (compatibility callback)") {
    CATCH_SECTION("warn") {
        CompatHandler log(LOG_LEVEL_WARN); // TRACE and INFO should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        CATCH_SECTION("trace") {
            LOG(TRACE, "message");
            LOG_PRINT(TRACE, "print,");
            LOG_PRINTF(TRACE, "%s", "printf,");
            LOG_DUMP(TRACE, "\0", 1);
            log.bufferEquals("");
        }
        CATCH_SECTION("info") {
            LOG(INFO, "message");
            LOG_PRINT(INFO, "print,");
            LOG_PRINTF(INFO, "%s", "printf,");
            LOG_DUMP(INFO, "\0", 1);
            log.bufferEquals("");
        }
        CATCH_SECTION("warn") {
            LOG(WARN, "message");
            LOG_PRINT(WARN, "print,");
            LOG_PRINTF(WARN, "%s", "printf,");
            LOG_DUMP(WARN, "\0", 1);
            log.bufferEndsWith("WARN: message\r\nprint,printf,00");
        }
        CATCH_SECTION("error") {
            LOG(ERROR, "message");
            LOG_PRINT(ERROR, "print,");
            LOG_PRINTF(ERROR, "%s", "printf,");
            LOG_DUMP(ERROR, "\0", 1);
            log.bufferEndsWith("ERROR: message\r\nprint,printf,00");
        }
    }
}

CATCH_TEST_CASE("Scoped category") {
    TestHandler log(LOG_LEVEL_ALL);
    LOG(INFO, "");
    log.next().categoryEquals("global");
    {
        LOG_CATEGORY("local");
        LOG(INFO, "");
        log.next().categoryEquals("local");
    }
    LOG(INFO, "");
    log.next().categoryEquals("global");
}

CATCH_TEST_CASE("Category filtering") {
    TestHandler log(LOG_LEVEL_ERROR, {
        { "b.b", LOG_LEVEL_INFO },
        { "a", LOG_LEVEL_WARN },
        { "a.a.a", LOG_LEVEL_TRACE },
        { "a.a", LOG_LEVEL_INFO }
    });
    CATCH_SECTION("a") {
        LOG_CATEGORY("a"); // TRACE and INFO should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("cd");
    }
    CATCH_SECTION("a.a") {
        LOG_CATEGORY("a.a"); // TRACE should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("bcd");
    }
    CATCH_SECTION("a.a.a") {
        LOG_CATEGORY("a.a.a"); // No messages should be filtered out
        CATCH_CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_TRACE);
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("abcd");
    }
    CATCH_SECTION("a.x") {
        LOG_CATEGORY("a.x"); // TRACE and INFO should be filtered out (according to filter set for "a" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("cd");
    }
    CATCH_SECTION("a.a.x") {
        LOG_CATEGORY("a.a.x"); // TRACE should be filtered out (according to filter set for "a.a" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("bcd");
    }
    CATCH_SECTION("a.a.a.x") {
        LOG_CATEGORY("a.a.a.x"); // No messages should be filtered out (according to filter set for "a.a.a" category)
        CATCH_CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_TRACE);
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("abcd");
    }
    CATCH_SECTION("b") {
        LOG_CATEGORY("b"); // All levels except ERROR should be filtered out (default level)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("d");
    }
    CATCH_SECTION("b.x") {
        LOG_CATEGORY("b.x"); // All levels except ERROR should be filtered out (default level)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("d");
    }
    CATCH_SECTION("b.b") {
        LOG_CATEGORY("b.b"); // TRACE should be filtered out
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("bcd");
    }
    CATCH_SECTION("b.b.x") {
        LOG_CATEGORY("b.b.x"); // TRACE should be filtered out (according to filter set for "b.b" category)
        CATCH_CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.next().levelEquals(LOG_LEVEL_INFO);
        log.next().levelEquals(LOG_LEVEL_WARN);
        log.next().levelEquals(LOG_LEVEL_ERROR);
        CATCH_CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        log.bufferEquals("bcd");
    }
}

CATCH_TEST_CASE("Malformed category name") {
    TestHandler log(LOG_LEVEL_ERROR, {
        { "a", LOG_LEVEL_WARN },
        { "a.a", LOG_LEVEL_INFO }
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
        LOG_CATEGORY("a..a"); // ditto
        CATCH_CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
}

CATCH_TEST_CASE("Miscellaneous") {
    CATCH_SECTION("exact category match") {
        TestHandler log(LOG_LEVEL_ERROR, {
            { "aaa", LOG_LEVEL_TRACE },
            { "aa", LOG_LEVEL_INFO },
            { "a", LOG_LEVEL_WARN }
        });
        CATCH_CHECK(LOG_ENABLED_C(WARN, "a"));
        CATCH_CHECK(LOG_ENABLED_C(INFO, "aa"));
        CATCH_CHECK(LOG_ENABLED_C(TRACE, "aaa"));
        CATCH_CHECK(LOG_ENABLED_C(ERROR, "x"));
    }
}
