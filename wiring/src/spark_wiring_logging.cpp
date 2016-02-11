/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "spark_wiring_logging.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace {

using spark::Logger;

class LogHandler {
public:
    LogHandler() {
        log_set_handler(logMessage, logString);
    }

    void addLogger(Logger *logger) {
        auto it = std::find(loggers_.begin(), loggers_.end(), logger);
        if (it == loggers_.end()) {
            loggers_.push_back(logger);
        }
    }

    void removeLogger(Logger *logger) {
        auto it = std::find(loggers_.begin(), loggers_.end(), logger);
        if (it != loggers_.end()) {
            loggers_.erase(it);
        }
    }

    static LogHandler* instance() {
        static LogHandler handler;
        return &handler;
    }

private:
    std::vector<Logger*> loggers_;

    static void logMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func) {
        const auto &loggers = instance()->loggers_;
        for (size_t i = 0; i < loggers.size(); ++i) {
            loggers[i]->logMessage(msg, level, time, category, file, line, func);
        }
    }

    static void logString(const char *str, LoggerOutputLevel level) {
        const auto &loggers = instance()->loggers_;
        for (size_t i = 0; i < loggers.size(); ++i) {
            loggers[i]->logString(str, level);
        }
    }
};

} // namespace

// spark::Logger
spark::Logger::Logger(LoggerOutputLevel level, const std::initializer_list<CategoryFilter> &filters) :
        level_(level) {
    for (auto it = filters.begin(); it != filters.end(); ++it) {
        const char* const category = it->first;
        const LoggerOutputLevel level = it->second;
        std::vector<Filter> *filters = &filters_; // Root categories
        size_t pos = 0;
        for (size_t i = 0;; ++i) {
            if (category[i] && category[i] != '.') { // Category separator
                continue;
            }
            const size_t size = i - pos;
            if (!size) {
                break; // Invalid category name
            }
            const char* const name = category + pos;
            bool found = false;
            auto it = std::lower_bound(filters->begin(), filters->end(), std::make_pair(name, size),
                    [&found](const Filter &filter, const std::pair<const char*, size_t> &value) {
                const int cmp = std::strncmp(filter.name, value.first, std::min(filter.size, value.second));
                if (cmp == 0) {
                    found = true;
                }
                return cmp < 0;
            });
            if (!found) {
                it = filters->insert(it, Filter(name, size)); // Add subcategory
            }
            if (!category[i]) {
                it->level = level;
                break;
            }
            filters = &it->filters;
            pos = i + 1;
        }
    }
}

LoggerOutputLevel spark::Logger::categoryLevel(const char *category) {
    if (!category || filters_.empty()) {
        return level_; // Default level
    }
    LoggerOutputLevel level = level_;
    const std::vector<Filter> *filters = &filters_; // Root categories
    size_t pos = 0;
    for (size_t i = 0;; ++i) {
        if (category[i] && category[i] != '.') { // Category separator
            continue;
        }
        const size_t size = i - pos;
        if (!size) {
            break; // Invalid category name
        }
        const char* const name = category + pos;
        bool found = false;
        auto it = std::lower_bound(filters->begin(), filters->end(), std::make_pair(name, size),
                [&found](const Filter &filter, const std::pair<const char*, size_t> &value) {
            const int cmp = std::strncmp(filter.name, value.first, std::min(filter.size, value.second));
            if (cmp == 0) {
                found = true;
            }
            return cmp < 0;
        });
        if (!found) {
            break;
        }
        if (it->level >= 0) {
            level = (LoggerOutputLevel)it->level;
        }
        if (!category[i]) {
            break;
        }
        filters = &it->filters;
        pos = i + 1;
    }
    return level;
}

void spark::Logger::install(Logger *logger) {
    LogHandler::instance()->addLogger(logger);
}

void spark::Logger::uninstall(Logger *logger) {
    LogHandler::instance()->removeLogger(logger);
}

// spark::FormattingLogger
void spark::FormattingLogger::writeMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
        const char *file, int line, const char *func) {
    const size_t size = 80;
    char buf[size];
    size_t pos = 0;
    do {
        // Timestamp
        pos += snprintf(buf + pos, size - pos, "%010u", (unsigned)time);
        if (pos >= size) {
            break;
        }
        // Category (optional)
        if (category) {
            pos += snprintf(buf + pos, size - pos, ": %s", category);
            if (pos >= size) {
                break;
            }
        }
        // File, line number (optional)
        if (file) {
            pos += snprintf(buf + pos, size - pos, ": %s:%d", file, line);
            if (pos >= size) {
                break;
            }
        }
        // Function name (optional)
        if (func) {
            pos += snprintf(buf + pos, size - pos, ": %s", func);
            if (pos >= size) {
                break;
            }
        }
        // Level
        pos += snprintf(buf + pos, size - pos, ": %s", levelName(level));
        if (pos >= size) {
            break;
        }
    } while (false);
    if (pos >= size) {
        buf[size - 2] = '~';
    }
    writeString(buf, level);
    writeString(": ", level);
    writeString(msg, level);
    writeString("\r\n", level);
}

const char* spark::FormattingLogger::levelName(LoggerOutputLevel level) {
    switch (level) {
    case TRACE_LEVEL:
        return "TRACE";
    case LOG_LEVEL:
        return "LOG";
    case DEBUG_LEVEL:
        return "DEBUG";
    case INFO_LEVEL:
        return "INFO";
    case WARN_LEVEL:
        return "WARN";
    case ERROR_LEVEL:
        return "ERROR";
    case PANIC_LEVEL:
        return "PANIC";
    default:
        return "";
    }
}
