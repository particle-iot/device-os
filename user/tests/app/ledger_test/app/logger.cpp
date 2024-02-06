#include <memory>
#include <cstring>

#include <spark_wiring_usbserial.h>
#include <spark_wiring_logging.h>
#include <spark_wiring_error.h>

#include "logger.h"
#include "config.h"

namespace particle::test {

namespace {

const auto LEDGER_CATEGORY = "system.ledger";
const auto APP_CATEGORY = "app";

class AnsiLogHandler: public StreamLogHandler {
public:
    using StreamLogHandler::StreamLogHandler;

protected:
    using StreamLogHandler::write;

    void logMessage(const char* msg, LogLevel level, const char* category, const LogAttributes& attr) override {
        if (level >= LOG_LEVEL_ERROR) {
            stream()->write("\033[31;1m"); // Red, bold
        } else if (level >= LOG_LEVEL_WARN) {
            stream()->write("\033[33;1m"); // Yellow, bold
        } else if (category && std::strcmp(category, APP_CATEGORY) == 0) {
            stream()->write("\033[32m"); // Green
        } else if (category && std::strcmp(category, LEDGER_CATEGORY) == 0) {
            stream()->write("\033[37m"); // White
        } else {
            stream()->write("\033[90m"); // Gray
        }
        writingMsg_ = true;
        StreamLogHandler::logMessage(msg, level, category, attr);
        writingMsg_ = false;
        stream()->write("\033[0m"); // Reset
    }

    void write(const char* data, size_t size) override {
        if (!writingMsg_) {
            stream()->write("\033[90m"); // Gray
        }
        StreamLogHandler::write(data, size);
        if (!writingMsg_) {
            stream()->write("\033[0m"); // Reset
        }
    }

private:
    bool writingMsg_ = false;
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
        { LEDGER_CATEGORY, appLevel },
        { APP_CATEGORY, appLevel }
    }));
    if (!handler || !LogManager::instance()->addHandler(handler.get())) {
        return Error::NO_MEMORY;
    }
    g_logHandler = std::move(handler);
    return 0;
}

} // namespace particle::test
