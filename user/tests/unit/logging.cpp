#define LOG_MODULE_CATEGORY "module"
#define LOG_INCLUDE_SOURCE_INFO 1

#include "spark_wiring_logging.h"
#include "service_debug.h"

#include "mocks/control.h"

#include "tools/catch.h"
#include "tools/string.h"
#include "tools/stream.h"
#include "tools/random.h"

#include "hippomocks.h"

#include <boost/optional/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <queue>
#include <map>

#define CHECK_LOG_ATTR_FLAG(flag, value) \
        do { \
            LogAttributes attr = { 0 }; \
            attr.flag = 1; \
            CHECK(attr.flags == value); \
        } while (false)

// Source file's logging category
LOG_SOURCE_CATEGORY("source")

namespace {

using namespace spark;

std::string fileName(const std::string &path);

class TestLogHandler;

class LogMessage {
public:
    LogMessage(TestLogHandler &handler, const char *msg, LogLevel level, const char *category, const LogAttributes &attr) :
            handler_(handler),
            level_(level) {
        if (msg) {
            msg_ = msg;
        }
        if (category) {
            cat_ = category;
        }
        if (attr.has_file) {
            file_ = fileName(attr.file); // Strip directory path
        }
        if (attr.has_line) {
            line_ = attr.line;
        }
        if (attr.has_function) {
            func_ = attr.function;
        }
        if (attr.has_time) {
            time_ = attr.time;
        }
        if (attr.has_code) {
            code_ = attr.code;
        }
        if (attr.has_details) {
            detail_ = attr.details;
        }
    }

    const LogMessage& messageEquals(const std::string &msg) const {
        CHECK(msg_ == msg);
        return *this;
    }

    const LogMessage& levelEquals(LogLevel level) const {
        CHECK(level_ == level);
        return *this;
    }

    const LogMessage& categoryEquals(const std::string &category) const {
        CHECK(cat_ == category);
        return *this;
    }

    // Default attributes
    const LogMessage& fileEquals(const std::string &file) const {
        CHECK(file_ == file);
        return *this;
    }

    const LogMessage& lineEquals(int line) const {
        CHECK(line_ == line);
        return *this;
    }

    const LogMessage& functionEquals(const std::string &func) const {
        CHECK(func_ == func);
        return *this;
    }

    const LogMessage& timeEquals(uint32_t time) const {
        CHECK(time_ == time);
        return *this;
    }

    // Additional attributes
    const LogMessage& codeEquals(intptr_t code) const {
        CHECK(code_ == code);
        return *this;
    }

    const LogMessage& hasCode(bool yes = true) const {
        CHECK((bool)code_ == yes);
        return *this;
    }

    const LogMessage& detailsEquals(const std::string &str) const {
        CHECK(detail_ == str);
        return *this;
    }

    const LogMessage& hasDetails(bool yes = true) const {
        CHECK((bool)detail_ == yes);
        return *this;
    }

    LogMessage checkNext() const;
    void checkAtEnd() const;

private:
    TestLogHandler &handler_;
    boost::optional<std::string> msg_, cat_, file_, func_, detail_;
    boost::optional<intptr_t> code_;
    boost::optional<uint32_t> time_;
    boost::optional<int> line_;
    LogLevel level_;
};

// Base class for test log handlers
class TestLogHandler: public LogHandler {
public:
    TestLogHandler(LogLevel level, LogCategoryFilters filters, Print *stream) :
            LogHandler(level, filters),
            strm_(stream) {
    }

    LogMessage checkNext() {
        REQUIRE(!msgs_.empty());
        const LogMessage m = msgs_.front();
        msgs_.pop();
        return m;
    }

    void checkAtEnd() const {
        REQUIRE(msgs_.empty());
    }

    bool hasNext() const {
        return !msgs_.empty();
    }

    Print* stream() const {
        return strm_;
    }

protected:
    // spark::LogHandler
    virtual void logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr) override {
        msgs_.push(LogMessage(*this, msg, level, category, attr));
    }

    virtual void write(const char *data, size_t size) override {
        if (strm_) { // Output stream is optional
            strm_->write((const uint8_t*)data, size);
        }
    }

private:
    std::queue<LogMessage> msgs_;
    Print *strm_;
};

inline LogMessage LogMessage::checkNext() const {
    return handler_.checkNext();
}

inline void LogMessage::checkAtEnd() const {
    handler_.checkAtEnd();
}

// Log handler using its own output stream and registering itself automatically
class DefaultLogHandler: public TestLogHandler {
public:
    explicit DefaultLogHandler(LogLevel level = LOG_LEVEL_ALL, LogCategoryFilters filters = {}) :
            TestLogHandler(level, filters, &strm_) {
        LogManager::instance()->addHandler(this);
    }

    virtual ~DefaultLogHandler() {
        LogManager::instance()->removeHandler(this);
    }

    const test::OutputStream& stream() const {
        return strm_;
    }

private:
    test::OutputStream strm_;
};

// Log handler with additional parameters
class NamedLogHandler: public TestLogHandler {
public:
    NamedLogHandler(std::string name, LogLevel level, LogCategoryFilters filters, Print *stream) :
            TestLogHandler(level, filters, stream),
            name_(name) {
        ++s_count;
    }

    virtual ~NamedLogHandler() {
        --s_count;
    }

    const std::string& name() const {
        return name_;
    }

    static size_t instanceCount() {
        return s_count;
    }

    // This class is non-copyable
    NamedLogHandler(const NamedLogHandler&) = delete;
    NamedLogHandler& operator=(const NamedLogHandler&) = delete;

private:
    std::string name_;

    static size_t s_count;
};

// Log handler using compatibility callback
class CompatLogHandler {
public:
    explicit CompatLogHandler(LogLevel level = LOG_LEVEL_ALL) {
        s_instance = this;
        set_logger_output(callback, level);
    }

    ~CompatLogHandler() {
        set_logger_output(nullptr, LOG_LEVEL_NONE);
        s_instance = nullptr;
    }

    const test::OutputStream& stream() const {
        return strm_;
    }

private:
    test::OutputStream strm_;

    static CompatLogHandler *s_instance;

    static void callback(const char *str) {
        s_instance->strm_.write((const uint8_t*)str, strlen(str));
    }
};

// Mixin class registering log handler automatically
template<typename HandlerT>
class ScopedLogHandler: public HandlerT {
public:
    template<typename... ArgsT>
    explicit ScopedLogHandler(ArgsT&&... args) :
            HandlerT(std::forward<ArgsT>(args)...) {
        LogManager::instance()->addHandler(this);
    }

    virtual ~ScopedLogHandler() {
        LogManager::instance()->removeHandler(this);
    }
};

class NamedLogHandlerFactory: public LogHandlerFactory {
public:
    NamedLogHandlerFactory() {
        LogManager::instance()->setHandlerFactory(this);
    }

    virtual ~NamedLogHandlerFactory() {
        // Restore default factory (destroys all active handlers)
        LogManager::instance()->setHandlerFactory(DefaultLogHandlerFactory::instance());
    }

    virtual LogHandler* createHandler(const char *type, LogLevel level, LogCategoryFilters filters, Print *stream,
            const JSONValue &params) override {
        if (strcmp(type, "NamedLogHandler") != 0) {
            return nullptr; // Unsupported handler type
        }
        // Getting additional parameters
        std::string name;
        JSONObjectIterator it(params);
        while (it.next()) {
            if (it.name() == "name") { // Handler name
                name = (const char*)it.value().toString();
                break;
            }
        }
        if (handlers_.count(name)) {
            return nullptr; // Duplicate handler name
        }
        NamedLogHandler *h = new NamedLogHandler(name, level, filters, stream);
        handlers_.insert(std::make_pair(name, h));
        return h;
    }

    virtual void destroyHandler(LogHandler *handler) override {
        NamedLogHandler *h = dynamic_cast<NamedLogHandler*>(handler);
        if (h) {
            handlers_.erase(h->name());
            delete h;
        } else {
            CATCH_WARN("NamedLogHandlerFactory: Unexpected handler type");
        }
    }

    NamedLogHandler& handler(const std::string &name = std::string()) const {
        const auto it = handlers_.find(name);
        if (it == handlers_.end()) {
            FAIL("NamedLogHandlerFactory: Unknown handler name: " << name);
        }
        return *it->second;
    }

    bool hasHandler(const std::string &name = std::string()) const {
        return handlers_.count(name);
    }

private:
    std::map<std::string, NamedLogHandler*> handlers_;
};

// Output stream with additional parameters
class NamedOutputStream: public test::OutputStream {
public:
    explicit NamedOutputStream(std::string name) :
            name_(name) {
        ++s_count;
    }

    virtual ~NamedOutputStream() {
        --s_count;
    }

    const std::string& name() const {
        return name_;
    }

    static size_t instanceCount() {
        return s_count;
    }

    // This class is non-copyable
    NamedOutputStream(const NamedOutputStream&) = delete;
    NamedOutputStream& operator=(const NamedOutputStream&) = delete;

private:
    std::string name_;

    static size_t s_count;
};

class NamedOutputStreamFactory: public OutputStreamFactory {
public:
    NamedOutputStreamFactory() {
        LogManager::instance()->setStreamFactory(this);
    }

    virtual ~NamedOutputStreamFactory() {
        // Restore default factory (destroys all active handlers)
        LogManager::instance()->setStreamFactory(DefaultOutputStreamFactory::instance());
    }

    virtual Print* createStream(const char *type, const JSONValue &params) override {
        if (strcmp(type, "NamedOutputStream") != 0) {
            return nullptr; // Unsupported stream type
        }
        // Getting additional parameters
        std::string name;
        JSONObjectIterator it(params);
        while (it.next()) {
            if (it.name() == "name") { // Stream name
                name = (const char*)it.value().toString();
                break;
            }
        }
        if (streams_.count(name)) {
            return nullptr; // Duplicate stream name
        }
        NamedOutputStream *s = new NamedOutputStream(name);
        streams_.insert(std::make_pair(name, s));
        return s;
    }

    virtual void destroyStream(Print *stream) override {
        NamedOutputStream *s = dynamic_cast<NamedOutputStream*>(stream);
        if (s) {
            streams_.erase(s->name());
            delete s;
        } else {
            CATCH_WARN("NamedOutputStreamFactory: Unexpected stream type");
        }
    }

    NamedOutputStream& stream(const std::string &name = std::string()) const {
        const auto it = streams_.find(name);
        if (it == streams_.end()) {
            FAIL("NamedOutputStreamFactory: Unknown stream name: " << name);
        }
        return *it->second;
    }

    bool hasStream(const std::string &name = std::string()) const {
        return streams_.count(name);
    }

private:
    std::map<std::string, NamedOutputStream*> streams_;
};

// Convenience wrapper for spark::logProcessControlRequest()
class LogControl {
public:
    LogControl() :
            ctrl_(&mocks_) {
    }

    bool config(const std::string &req, std::string* rep = nullptr) {
        const auto r = ctrl_.makeRequest(CTRL_REQUEST_LOG_CONFIG, req);
        logProcessControlRequest(r.get());
        REQUIRE(r->hasResult());
        if (rep) {
            *rep = r->replyData();
        }
        return (r->result() == SYSTEM_ERROR_NONE);
    }

private:
    MockRepository mocks_;
    test::SystemControl ctrl_;
};

std::string fileName(const std::string &path) {
    const size_t pos = path.rfind('/');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

size_t NamedLogHandler::s_count = 0;
size_t NamedOutputStream::s_count = 0;

CompatLogHandler* CompatLogHandler::s_instance = nullptr;

const std::string SOURCE_FILE = fileName(__FILE__);
const std::string SOURCE_CATEGORY = LOG_THIS_CATEGORY();

} // namespace

TEST_CASE("Message logging") {
    DefaultLogHandler log(LOG_LEVEL_ALL);
    SECTION("default attributes") {
        LOG(TRACE, "trace");
        log.checkNext().messageEquals("trace").levelEquals(LOG_LEVEL_TRACE).categoryEquals(LOG_THIS_CATEGORY()).fileEquals(SOURCE_FILE);
        LOG(INFO, "info");
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO).categoryEquals(LOG_THIS_CATEGORY()).fileEquals(SOURCE_FILE);
        LOG(WARN, "warn");
        log.checkNext().messageEquals("warn").levelEquals(LOG_LEVEL_WARN).categoryEquals(LOG_THIS_CATEGORY()).fileEquals(SOURCE_FILE);
        LOG(ERROR, "error");
        log.checkNext().messageEquals("error").levelEquals(LOG_LEVEL_ERROR).categoryEquals(LOG_THIS_CATEGORY()).fileEquals(SOURCE_FILE);
    }
    SECTION("additional attributes") {
        LOG(INFO, "info");
        log.checkNext().hasCode(false).hasDetails(false); // No additional attributes
        LOG_ATTR(INFO, (code = -1, details = "details"), "info");
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO).categoryEquals(LOG_THIS_CATEGORY()).fileEquals(SOURCE_FILE)
                .codeEquals(-1).detailsEquals("details");
    }
    SECTION("message formatting") {
        std::string s = "";
        LOG(TRACE, "%s", s.c_str());
        log.checkNext().messageEquals("");
        s = test::randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG(INFO, "%s", s.c_str());
        log.checkNext().messageEquals(s);
        s = test::randomString(LOG_MAX_STRING_LENGTH);
        LOG(WARN, "%s", s.c_str());
        log.checkNext().messageEquals(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = test::randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG(ERROR, "%s", s.c_str());
        log.checkNext().messageEquals(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    SECTION("compatibility macros") {
        DEBUG("debug"); // Alias for LOG_DEBUG(TRACE, ...)
#ifdef DEBUG_BUILD
        log.checkNext().messageEquals("debug").levelEquals(LOG_LEVEL_TRACE);
#endif
        INFO("info"); // Alias for LOG(INFO, ...)
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO);
        WARN("warn"); // Alias for LOG(WARN, ...)
        log.checkNext().messageEquals("warn").levelEquals(LOG_LEVEL_WARN);
        ERROR("error"); // Alias for LOG(ERROR, ...)
        log.checkNext().messageEquals("error").levelEquals(LOG_LEVEL_ERROR);
    }
}
/*
TEST_CASE("Message logging (compatibility callback)") {
    CompatLogHandler log(LOG_LEVEL_ALL);
    SECTION("default attributes") {
        LOG(TRACE, "trace");
        check(log.stream()).endsWith("TRACE: trace\r\n");
        LOG(INFO, "info");
        check(log.stream()).endsWith("INFO: info\r\n");
        LOG(WARN, "warn");
        check(log.stream()).endsWith("WARN: warn\r\n");
        LOG(ERROR, "error");
        check(log.stream()).endsWith("ERROR: error\r\n");
        check(log.stream()).contains(SOURCE_FILE);
    }
    SECTION("message formatting") {
        std::string s = test::randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG(INFO, "%s", s.c_str());
        check(log.stream()).endsWith(s + "\r\n");
        s = test::randomString(LOG_MAX_STRING_LENGTH);
        LOG(WARN, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + "~\r\n"); // 1 character is reserved for term. null
        s = test::randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG(ERROR, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + "~\r\n");
    }
}
*/
TEST_CASE("Direct logging") {
    DefaultLogHandler log(LOG_LEVEL_ALL);
    SECTION("write") {
        std::string s = "";
        LOG_WRITE(INFO, s.c_str(), s.size());
        check(log.stream()).isEmpty();
        s = test::randomString(1, 100);
        LOG_WRITE(WARN, s.c_str(), s.size());
        check(log.stream()).endsWith(s);
        s = test::randomString(1, 100);
        LOG_PRINT(ERROR, s.c_str()); // Alias for LOG_WRITE(level, str, strlen(str))
        check(log.stream()).endsWith(s);
    }
    SECTION("printf") {
        std::string s = "";
        LOG_PRINTF(TRACE, "%s", s.c_str());
        check(log.stream()).isEmpty();
        s = test::randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_PRINTF(INFO, "%s", s.c_str());
        check(log.stream()).equals(s);
        s = test::randomString(LOG_MAX_STRING_LENGTH);
        LOG_PRINTF(WARN, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = test::randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_PRINTF(ERROR, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    SECTION("dump") {
        std::string s = "";
        LOG_DUMP(TRACE, s.c_str(), s.size());
        check(log.stream()).isEmpty();
        s = test::randomBytes(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_DUMP(INFO, s.c_str(), s.size());
        check(log.stream()).unhex().equals(s);
        s = test::randomBytes(LOG_MAX_STRING_LENGTH);
        LOG_DUMP(WARN, s.c_str(), s.size());
        check(log.stream()).unhex().endsWith(s);
        s = test::randomBytes(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_DUMP(ERROR, s.c_str(), s.size());
        check(log.stream()).unhex().endsWith(s);
    }
    SECTION("compatibility macros") {
        std::string s = test::randomString(LOG_MAX_STRING_LENGTH / 2);
        DEBUG_D("%s", s.c_str()); // Alias for LOG_DEBUG_PRINTF(TRACE, ...)
#ifdef DEBUG_BUILD
        check(log.stream()).equals(s);
#endif
    }
}
/*
// Copy-pase of above test case with DefaultLogHandler replaced with CompatLogHandler
TEST_CASE("Direct logging (compatibility callback)") {
    CompatLogHandler log(LOG_LEVEL_ALL);
    SECTION("write") {
        std::string s = "";
        LOG_WRITE(INFO, s.c_str(), s.size());
        check(log.stream()).isEmpty();
        s = test::randomString(1, 100);
        LOG_WRITE(WARN, s.c_str(), s.size());
        check(log.stream()).endsWith(s);
        s = test::randomString(1, 100);
        LOG_PRINT(ERROR, s.c_str()); // Alias for LOG_WRITE(level, str, strlen(str))
        check(log.stream()).endsWith(s);
    }
    SECTION("printf") {
        std::string s = "";
        LOG_PRINTF(TRACE, "%s", s.c_str());
        check(log.stream()).isEmpty();
        s = test::randomString(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_PRINTF(INFO, "%s", s.c_str());
        check(log.stream()).equals(s);
        s = test::randomString(LOG_MAX_STRING_LENGTH);
        LOG_PRINTF(WARN, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~'); // 1 character is reserved for term. null
        s = test::randomString(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_PRINTF(ERROR, "%s", s.c_str());
        check(log.stream()).endsWith(s.substr(0, LOG_MAX_STRING_LENGTH - 2) + '~');
    }
    SECTION("dump") {
        std::string s = "";
        LOG_DUMP(TRACE, s.c_str(), s.size());
        check(log.stream()).isEmpty();
        s = test::randomBytes(LOG_MAX_STRING_LENGTH / 2); // Smaller than the internal buffer
        LOG_DUMP(INFO, s.c_str(), s.size());
        check(log.stream()).unhex().equals(s);
        s = test::randomBytes(LOG_MAX_STRING_LENGTH);
        LOG_DUMP(WARN, s.c_str(), s.size());
        check(log.stream()).unhex().endsWith(s);
        s = test::randomBytes(LOG_MAX_STRING_LENGTH * 3 / 2); // Larger than the internal buffer
        LOG_DUMP(ERROR, s.c_str(), s.size());
        check(log.stream()).unhex().endsWith(s);
    }
    SECTION("compatibility macros") {
        std::string s = test::randomString(LOG_MAX_STRING_LENGTH / 2);
        DEBUG_D("%s", s.c_str()); // Alias for LOG_DEBUG_PRINTF(TRACE, ...)
#ifdef DEBUG_BUILD
        check(log.stream()).equals(s);
#endif
    }
}
*/
TEST_CASE("Basic filtering") {
    SECTION("warn") {
        DefaultLogHandler log(LOG_LEVEL_WARN); // TRACE and INFO should be filtered out
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("cd");
    }
    SECTION("none") {
        DefaultLogHandler log(LOG_LEVEL_NONE); // All levels should be filtered out
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && !LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).isEmpty();
    }
}
/*
TEST_CASE("Basic filtering (compatibility callback)") {
    CompatLogHandler log(LOG_LEVEL_WARN); // TRACE and INFO should be filtered out
    CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    SECTION("trace") {
        LOG(TRACE, "message");
        LOG_PRINT(TRACE, "print,");
        LOG_PRINTF(TRACE, "%s", "printf,");
        LOG_DUMP(TRACE, "\0", 1);
        check(log.stream()).isEmpty();
    }
    SECTION("info") {
        LOG(INFO, "message");
        LOG_PRINT(INFO, "print,");
        LOG_PRINTF(INFO, "%s", "printf,");
        LOG_DUMP(INFO, "\0", 1);
        check(log.stream()).isEmpty();
    }
    SECTION("warn") {
        LOG(WARN, "message");
        LOG_PRINT(WARN, "print,");
        LOG_PRINTF(WARN, "%s", "printf,");
        LOG_DUMP(WARN, "\0", 1);
        check(log.stream()).endsWith("WARN: message\r\nprint,printf,00");
    }
    SECTION("error") {
        LOG(ERROR, "message");
        LOG_PRINT(ERROR, "print,");
        LOG_PRINTF(ERROR, "%s", "printf,");
        LOG_DUMP(ERROR, "\0", 1);
        check(log.stream()).endsWith("ERROR: message\r\nprint,printf,00");
    }
}
*/
TEST_CASE("Scoped category") {
    DefaultLogHandler log(LOG_LEVEL_ALL);
    CHECK(LOG_THIS_CATEGORY() == SOURCE_CATEGORY);
    LOG(INFO, "");
    log.checkNext().categoryEquals(SOURCE_CATEGORY);
    {
        LOG_CATEGORY("scope");
        CHECK(LOG_THIS_CATEGORY() == std::string("scope"));
        LOG(INFO, "");
        log.checkNext().categoryEquals("scope");
    }
    CHECK(LOG_THIS_CATEGORY() == SOURCE_CATEGORY);
    LOG(INFO, "");
    log.checkNext().categoryEquals(SOURCE_CATEGORY);
}

TEST_CASE("Category filtering") {
    DefaultLogHandler log(LOG_LEVEL_ERROR, {
        { "b.b", LOG_LEVEL_INFO },
        { "a", LOG_LEVEL_WARN },
        { "a.a.a", LOG_LEVEL_TRACE },
        { "a.a", LOG_LEVEL_INFO }
    });
    SECTION("a") {
        LOG_CATEGORY("a"); // TRACE and INFO should be filtered out
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("cd");
    }
    SECTION("a.a") {
        LOG_CATEGORY("a.a"); // TRACE should be filtered out
        CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("bcd");
    }
    SECTION("a.a.a") {
        LOG_CATEGORY("a.a.a"); // No messages should be filtered out
        CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_TRACE);
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("abcd");
    }
    SECTION("a.x") {
        LOG_CATEGORY("a.x"); // TRACE and INFO should be filtered out (according to filter set for "a" category)
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("cd");
    }
    SECTION("a.a.x") {
        LOG_CATEGORY("a.a.x"); // TRACE should be filtered out (according to filter set for "a.a" category)
        CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("bcd");
    }
    SECTION("a.a.a.x") {
        LOG_CATEGORY("a.a.a.x"); // No messages should be filtered out (according to filter set for "a.a.a" category)
        CHECK((LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_TRACE);
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("abcd");
    }
    SECTION("b") {
        LOG_CATEGORY("b"); // All levels except ERROR should be filtered out (default level)
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("d");
    }
    SECTION("b.x") {
        LOG_CATEGORY("b.x"); // All levels except ERROR should be filtered out (default level)
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("d");
    }
    SECTION("b.b") {
        LOG_CATEGORY("b.b"); // TRACE should be filtered out
        CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("bcd");
    }
    SECTION("b.b.x") {
        LOG_CATEGORY("b.b.x"); // TRACE should be filtered out (according to filter set for "b.b" category)
        CHECK((!LOG_ENABLED(TRACE) && LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
        LOG(TRACE, ""); LOG(INFO, ""); LOG(WARN, ""); LOG(ERROR, "");
        log.checkNext().levelEquals(LOG_LEVEL_INFO);
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        LOG_PRINT(TRACE, "a"); LOG_PRINT(INFO, "b"); LOG_PRINT(WARN, "c"); LOG_PRINT(ERROR, "d");
        check(log.stream()).equals("bcd");
    }
}

TEST_CASE("Malformed category name") {
    DefaultLogHandler log(LOG_LEVEL_ERROR, {
        { "a", LOG_LEVEL_WARN },
        { "a.a", LOG_LEVEL_INFO }
    });
    SECTION("empty") {
        LOG_CATEGORY(""); // All levels except ERROR should be filtered out (default level)
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    SECTION(".") {
        LOG_CATEGORY("."); // ditto
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    SECTION(".a") {
        LOG_CATEGORY(".a"); // ditto
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && !LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    SECTION("a.") {
        LOG_CATEGORY("a."); // TRACE and INFO should be filtered out (according to filter set for "a" category)
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
    SECTION("a..a") {
        LOG_CATEGORY("a..a"); // ditto
        CHECK((!LOG_ENABLED(TRACE) && !LOG_ENABLED(INFO) && LOG_ENABLED(WARN) && LOG_ENABLED(ERROR)));
    }
}

TEST_CASE("Miscellaneous") {
    SECTION("exact category match") {
        DefaultLogHandler log(LOG_LEVEL_ERROR, {
            { "aaa", LOG_LEVEL_TRACE },
            { "aa", LOG_LEVEL_INFO },
            { "a", LOG_LEVEL_WARN }
        });
        CHECK(LOG_ENABLED_C(WARN, "a"));
        CHECK(LOG_ENABLED_C(INFO, "aa"));
        CHECK(LOG_ENABLED_C(TRACE, "aaa"));
        CHECK(LOG_ENABLED_C(ERROR, "x"));
    }
    SECTION("attribute flag values") {
        CHECK_LOG_ATTR_FLAG(has_file, 0x01);
        CHECK_LOG_ATTR_FLAG(has_line, 0x02);
        CHECK_LOG_ATTR_FLAG(has_function, 0x04);
        CHECK_LOG_ATTR_FLAG(has_time, 0x08);
        CHECK_LOG_ATTR_FLAG(has_code, 0x10);
        CHECK_LOG_ATTR_FLAG(has_details, 0x20);
        CHECK_LOG_ATTR_FLAG(has_end, 0x40);
    }
}

TEST_CASE("Logger API") {
    SECTION("message logging") {
        DefaultLogHandler log(LOG_LEVEL_ALL);
        Logger logger; // Uses module's category by default
        logger.trace("%s", "trace");
        log.checkNext().messageEquals("trace").levelEquals(LOG_LEVEL_TRACE).categoryEquals(LOG_MODULE_CATEGORY); // No file info available
        logger.info("%s", "info");
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO).categoryEquals(LOG_MODULE_CATEGORY);
        logger.warn("%s", "warn");
        log.checkNext().messageEquals("warn").levelEquals(LOG_LEVEL_WARN).categoryEquals(LOG_MODULE_CATEGORY);
        logger.error("%s", "error");
        log.checkNext().messageEquals("error").levelEquals(LOG_LEVEL_ERROR).categoryEquals(LOG_MODULE_CATEGORY);
        logger.log(LOG_LEVEL_INFO, "%s", "info");
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO).categoryEquals(LOG_MODULE_CATEGORY);
        logger.log("%s", "default"); // Uses default level
        log.checkNext().messageEquals("default").levelEquals(Logger::DEFAULT_LEVEL).categoryEquals(LOG_MODULE_CATEGORY);
        logger(LOG_LEVEL_INFO, "%s", "info"); // Alias for Logger::log(LogLevel, const char *fmt)
        log.checkNext().messageEquals("info").levelEquals(LOG_LEVEL_INFO).categoryEquals(LOG_MODULE_CATEGORY);
        logger("%s", "default"); // Alias for Logger::log(const char *fmt)
        log.checkNext().messageEquals("default").levelEquals(Logger::DEFAULT_LEVEL).categoryEquals(LOG_MODULE_CATEGORY);
    }
    SECTION("additional attributes") {
        DefaultLogHandler log(LOG_LEVEL_ALL);
        Logger logger;
        logger.log("%s", "");
        log.checkNext().hasCode(false).hasDetails(false); // No additional attributes
        // LogAttributes::code
        logger.code(-1).log("%s", "");
        log.checkNext().codeEquals(-1);
        // LogAttributes::details
        logger.details("details").info("%s", "");
        log.checkNext().detailsEquals("details");
    }
    SECTION("direct logging") {
        DefaultLogHandler log(LOG_LEVEL_ALL);
        Logger logger;
        logger.write(LOG_LEVEL_TRACE, "a", 1);
        logger.write("b", 1); // Uses default level
        logger.print(LOG_LEVEL_INFO, "c");
        logger.print("d");
        logger.printf(LOG_LEVEL_WARN, "%s", "e");
        logger.printf("%s", "f");
        logger.dump(LOG_LEVEL_ERROR, "\x01", 1);
        logger.dump("\x02", 1);
        check(log.stream()).equals("abcdef0102");
    }
    SECTION("basic filtering") {
        DefaultLogHandler log(LOG_LEVEL_WARN); // TRACE and INFO should be filtered out
        Logger logger;
        CHECK((!logger.isTraceEnabled() && !logger.isInfoEnabled() && logger.isWarnEnabled() && logger.isErrorEnabled()));
        logger.trace("trace"); logger.info("info"); logger.warn("warn"); logger.error("error");
        log.checkNext().levelEquals(LOG_LEVEL_WARN);
        log.checkNext().levelEquals(LOG_LEVEL_ERROR);
        CHECK(!log.hasNext());
        logger.print(LOG_LEVEL_TRACE, "a"); logger.print(LOG_LEVEL_INFO, "b"); logger.print(LOG_LEVEL_WARN, "c"); logger.print(LOG_LEVEL_ERROR, "d");
        check(log.stream()).equals("cd");
    }
    SECTION("category filtering") {
        // Only basic checks here - category filtering is tested in above test cases
        DefaultLogHandler log(LOG_LEVEL_ERROR, {
            { "a", LOG_LEVEL_WARN },
            { "a.b", LOG_LEVEL_INFO }
        });
        SECTION("a") {
            Logger logger("a"); // TRACE and INFO should be filtered out
            CHECK((!logger.isTraceEnabled() && !logger.isInfoEnabled() && logger.isWarnEnabled() && logger.isErrorEnabled()));
        }
        SECTION("a.b") {
            Logger logger("a.b"); // TRACE should be filtered out
            CHECK((!logger.isTraceEnabled() && logger.isInfoEnabled() && logger.isWarnEnabled() && logger.isErrorEnabled()));
        }
        SECTION("a.x") {
            Logger logger("a.x"); // TRACE and INFO should be filtered out (according to filter set for "a" category)
            CHECK((!logger.isTraceEnabled() && !logger.isInfoEnabled() && logger.isWarnEnabled() && logger.isErrorEnabled()));
        }
        SECTION("x") {
            Logger logger("x"); // All levels except ERROR should be filtered out (default level)
            CHECK((!logger.isTraceEnabled() && !logger.isInfoEnabled() && !logger.isWarnEnabled() && logger.isErrorEnabled()));
        }
    }
}

TEST_CASE("Message formatting") {
    SECTION("level names") {
        CHECK(LogHandler::levelName(LOG_LEVEL_TRACE) == std::string("TRACE"));
        CHECK(LogHandler::levelName(LOG_LEVEL_INFO) == std::string("INFO"));
        CHECK(LogHandler::levelName(LOG_LEVEL_WARN) == std::string("WARN"));
        CHECK(LogHandler::levelName(LOG_LEVEL_ERROR) == std::string("ERROR"));
        CHECK(LogHandler::levelName(LOG_LEVEL_PANIC) == std::string("PANIC"));
    }

    SECTION("default formatting") {
        test::OutputStream stream;
        ScopedLogHandler<StreamLogHandler> handler(stream);
        LOG_ATTR(INFO, (code = -1, details = "details"), "\"message\"");
        // timestamp [category] file:line, function(): level: message [code = ..., details = ...]
        check(stream).matches("\\d{10} \\[(.+)\\] (.+):\\d+, .+\\(\\): (.+): (.+) \\[code = (.+), details = (.+)\\]\r\n")
                .at(0).equals(LOG_THIS_CATEGORY())
                .at(1).equals(SOURCE_FILE)
                .at(2).equals("INFO")
                .at(3).equals("\"message\"")
                .at(4).equals("-1")
                .at(5).equals("details");
    }

    SECTION("JSON formatting") {
        test::OutputStream stream;
        ScopedLogHandler<JSONStreamLogHandler> handler(stream);
        LOG_ATTR(INFO, (code = -1, details = "details"), "\"message\"");
        // {"l": level, "m": message, "c": category, "f": file, "ln": line, "fn": function, "t": timestamp, "code": code, "detail": details}
        check(stream).matches("{\"l\":\"(.+)\",\"m\":\"(.+)\",\"c\":\"(.+)\",\"f\":\"(.+)\",\"ln\":\\d+,\"fn\":\".+\",\"t\":\\d+,\"code\":(.+),\"detail\":\"(.+)\"}\r\n")
                .at(0).equals("INFO")
                .at(1).equals("\\\"message\\\"") // Special characters are escaped
                .at(2).equals(LOG_THIS_CATEGORY())
                .at(3).equals(SOURCE_FILE)
                .at(4).equals("-1")
                .at(5).equals("details");
    }
}

TEST_CASE("Configuration requests") {
    LogControl logControl;
    NamedOutputStreamFactory streamFactory;
    NamedLogHandlerFactory handlerFactory;

    SECTION("adding and removing message-based handler") {
        // Add handler
        std::string rep;
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\"," // Handler ID
                "\"hnd\": {\"type\": \"NamedLogHandler\"}" // Handler settings
                "}", &rep));
        CHECK(rep.empty()); // No reply data expected
        CHECK(handlerFactory.handler().stream() == nullptr); // No output stream
        CHECK(!streamFactory.hasStream());
        // Enumerate handlers
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[\"1\"]");
        // Do some logging
        LOG(TRACE, ""); // Ignored by default
        LOG(INFO, "");
        handlerFactory.handler().checkNext().levelEquals(LOG_LEVEL_INFO);
        // Remove handler
        REQUIRE(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"1\"}", &rep));
        CHECK(rep.empty()); // No reply data expected
        CHECK(!handlerFactory.hasHandler());
        // Enumerate handlers
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[]"); // No active handlers
    }

    SECTION("adding and removing stream-based handler") {
        // Add handler
        std::string rep;
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\"," // Handler ID
                "\"hnd\": {\"type\": \"NamedLogHandler\"}," // Handler settings
                "\"strm\": {\"type\": \"NamedOutputStream\"}" // Stream settings
                "}", &rep));
        CHECK(rep.empty()); // No reply data expected
        CHECK(handlerFactory.hasHandler());
        CHECK(streamFactory.hasStream());
        // Enumerate handlers
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[\"1\"]");
        // Do some logging
        LOG(TRACE, ""); LOG_PRINT(TRACE, "trace"); // Ignored by default
        LOG(INFO, ""); LOG_PRINT(INFO, "info");
        handlerFactory.handler().checkNext().levelEquals(LOG_LEVEL_INFO);
        streamFactory.stream().check().equals("info");
        // Remove handler
        REQUIRE(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"1\"}", &rep));
        CHECK(rep.empty()); // No reply data expected
        CHECK(!handlerFactory.hasHandler());
        CHECK(!streamFactory.hasStream());
        // Enumerate handlers
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[]"); // No active handlers
    }

    SECTION("handler- and stream-specific parameters") {
        // Add handler
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"The Handler\"}}," // Handler name
                "\"strm\": {\"type\": \"NamedOutputStream\", \"param\": {\"name\": \"The Stream\"}}" // Stream name
                "}"));
        CHECK(handlerFactory.hasHandler("The Handler"));
        CHECK(streamFactory.hasStream("The Stream"));
        // Remove handler
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"1\"}"));
    }

    SECTION("adding multiple handlers with different settings") {
        // Add handlers
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"a\"}},"
                "\"lvl\": \"all\"" // LOG_LEVEL_ALL
                "}"));
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"2\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"b\"}},"
                "\"lvl\": \"info\"" // LOG_LEVEL_INFO
                "}"));
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"3\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"c\"}},"
                "\"lvl\": \"warn\"" // LOG_LEVEL_WARN
                "}"));
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"4\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"d\"}},"
                "\"lvl\": \"error\"," // LOG_LEVEL_ERROR (default level)
                "\"filt\": ["
                "{\"test\": \"trace\"}" // LOG_LEVEL_TRACE
                "]}"));
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"5\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"e\"}},"
                "\"lvl\": \"none\"" // LOG_LEVEL_NONE
                "}"));
        std::string rep;
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[\"1\",\"2\",\"3\",\"4\",\"5\"]");
        CHECK(handlerFactory.hasHandler("a"));
        CHECK(handlerFactory.hasHandler("b"));
        CHECK(handlerFactory.hasHandler("c"));
        CHECK(handlerFactory.hasHandler("d"));
        CHECK(handlerFactory.hasHandler("e"));
        // Do some logging
        LOG_C(TRACE, "test", ""); // Specifying category name explicitly
        LOG(TRACE, "");
        LOG(INFO, "");
        LOG(WARN, "");
        LOG(ERROR, "");
        handlerFactory.handler("a")
                .checkNext().levelEquals(LOG_LEVEL_TRACE).categoryEquals("test")
                .checkNext().levelEquals(LOG_LEVEL_TRACE)
                .checkNext().levelEquals(LOG_LEVEL_INFO)
                .checkNext().levelEquals(LOG_LEVEL_WARN)
                .checkNext().levelEquals(LOG_LEVEL_ERROR)
                .checkAtEnd();
        handlerFactory.handler("b")
                .checkNext().levelEquals(LOG_LEVEL_INFO)
                .checkNext().levelEquals(LOG_LEVEL_WARN)
                .checkNext().levelEquals(LOG_LEVEL_ERROR)
                .checkAtEnd();
        handlerFactory.handler("c")
                .checkNext().levelEquals(LOG_LEVEL_WARN)
                .checkNext().levelEquals(LOG_LEVEL_ERROR)
                .checkAtEnd();
        handlerFactory.handler("d")
                .checkNext().levelEquals(LOG_LEVEL_TRACE).categoryEquals("test")
                .checkNext().levelEquals(LOG_LEVEL_ERROR)
                .checkAtEnd();
        handlerFactory.handler("e").checkAtEnd();
        // Remove handlers
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"1\"}"));
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"2\"}"));
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"3\"}"));
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"4\"}"));
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"5\"}"));
    }

    SECTION("replacing handler") {
        // Add handler
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"a\"}}"
                "}"));
        std::string rep;
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[\"1\"]");
        CHECK(handlerFactory.hasHandler("a"));
        // Add another handler with the same ID
        REQUIRE(logControl.config("{"
                "\"cmd\": \"addHandler\","
                "\"id\": \"1\","
                "\"hnd\": {\"type\": \"NamedLogHandler\", \"param\": {\"name\": \"b\"}}" // a -> b
                "}"));
        REQUIRE(logControl.config("{\"cmd\": \"enumHandlers\"}", &rep));
        CHECK(rep == "[\"1\"]");
        CHECK(!handlerFactory.hasHandler("a"));
        CHECK(handlerFactory.hasHandler("b")); // a -> b
        // Remove handler
        CHECK(logControl.config("{\"cmd\": \"removeHandler\", \"id\": \"1\"}"));
    }

    SECTION("error handling and other checks") {
        SECTION("unsupported handler type") {
            CHECK(!logControl.config("{"
                    "\"cmd\": \"addHandler\","
                    "\"id\": \"1\","
                    "\"hnd\": {\"type\": \"DummyLogHandler\"},"
                    "\"strm\": {\"type\": \"NamedOutputStream\"}"
                    "}"));
        }
        SECTION("unsupported stream type") {
            CHECK(!logControl.config("{"
                    "\"cmd\": \"addHandler\","
                    "\"id\": \"1\","
                    "\"hnd\": {\"type\": \"NamedLogHandler\"},"
                    "\"strm\": {\"type\": \"DummyOutputStream\"}"
                    "}"));;
        }
        SECTION("missing or invalid parameters") {
            SECTION("cmd") {
                CHECK(!logControl.config("{"
                        "\"id\": \"1\""
                        "}")); // Missing command name
                CHECK(!logControl.config("{"
                        "\"cmd\": \"resetHandler\"" // Unsupported command
                        "\"id\": \"1\""
                        "}"));
            }
            SECTION("addHandler") {
                CHECK(!logControl.config("{"
                        "\"cmd\": \"addHandler\","
                        "\"hnd\": {\"type\": \"NamedLogHandler\"},"
                        "}")); // Missing handler ID
                CHECK(!logControl.config("{"
                        "\"cmd\": \"addHandler\","
                        "\"id\": \"1\","
                        "\"hnd\": {\"param\": {\"name\": \"a\"}}"
                        "}")); // Missing handler type
                CHECK(!logControl.config("{"
                        "\"cmd\": \"addHandler\","
                        "\"id\": \"1\","
                        "\"hnd\": {\"type\": \"NamedLogHandler\"},"
                        "\"lvl\": \"debug\"" // Invalid level name
                        "}"));
                CHECK(!logControl.config("{"
                        "\"cmd\": \"addHandler\","
                        "\"id\": \"1\","
                        "\"hnd\": {\"type\": \"NamedLogHandler\"},"
                        "\"filt\": ["
                        "{\"\": \"trace\"}" // Empty category name
                        "]}"));
            }
            SECTION("removeHandler") {
                CHECK(!logControl.config("{"
                        "\"cmd\": \"removeHandler\","
                        "}")); // Missing handler ID
            }
        }
    }

    CHECK(NamedOutputStream::instanceCount() == 0);
    CHECK(NamedLogHandler::instanceCount() == 0);
}
