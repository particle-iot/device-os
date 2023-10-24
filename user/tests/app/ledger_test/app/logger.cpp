#include <memory>

#include <spark_wiring_usbserial.h>
#include <spark_wiring_logging.h>
#include <spark_wiring_error.h>

#include "logger.h"
#include "config.h"

namespace particle::test {

namespace {

class AnsiLogHandler: public StreamLogHandler {
public:
    using StreamLogHandler::StreamLogHandler;

protected:
    using StreamLogHandler::write;

    void logMessage(const char* msg, LogLevel level, const char* category, const LogAttributes& attr) override {
        logMsg_ = true;
        switch (level) {
        case LOG_LEVEL_TRACE:
            stream()->write("\033[90m");
            break;
        case LOG_LEVEL_INFO:
            break;
        case LOG_LEVEL_WARN:
            stream()->write("\033[33;1m");
            break;
        case LOG_LEVEL_ERROR:
        case LOG_LEVEL_PANIC:
            stream()->write("\033[31;1m");
            break;
        }
        StreamLogHandler::logMessage(msg, level, category, attr);
        stream()->write("\033[0m");
        logMsg_ = false;
    }

    void write(const char* data, size_t size) override {
        if (!logMsg_) {
            stream()->write("\033[90m");
        }
        StreamLogHandler::write(data, size);
        if (!logMsg_) {
            stream()->write("\033[0m");
        }
    }

private:
    bool logMsg_ = false;
};

std::unique_ptr<LogHandler> g_logHandler;

} // namespace

int initLogger() {
    if (g_logHandler) {
        LogManager::instance()->removeHandler(g_logHandler.get());
        g_logHandler.reset();
    }
    auto& conf = Config::get();
    auto appLevel = conf.debugEnabled ? LOG_LEVEL_ALL : LOG_LEVEL_INFO;
    auto defaultLevel = conf.debugEnabled ? LOG_LEVEL_ALL : LOG_LEVEL_WARN;
    std::unique_ptr<LogHandler> handler(new(std::nothrow) AnsiLogHandler(Serial, defaultLevel, {
        { "system.ledger", appLevel },
        { "app", appLevel }
    }));
    if (!handler || !LogManager::instance()->addHandler(handler.get())) {
        return Error::NO_MEMORY;
    }
    g_logHandler = std::move(handler);
    return 0;
}

} // namespace particle::test
