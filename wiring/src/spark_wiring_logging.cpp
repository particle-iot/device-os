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

#include <algorithm>
#include <cstdio>

#include "spark_wiring_logging.h"

namespace {

using spark::Logger;

class LogHandler {
public:
    LogHandler() {
        log_set_callbacks(logMessage, logWrite, logEnabled, nullptr);
    }

    void addLogger(Logger *logger) {
        const auto it = std::find(loggers_.cbegin(), loggers_.cend(), logger);
        if (it == loggers_.end()) {
            loggers_.push_back(logger);
        }
    }

    void removeLogger(Logger *logger) {
        const auto it = std::find(loggers_.begin(), loggers_.end(), logger);
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

    static void logMessage(const char *msg, int level, const char *category, uint32_t time,
            const char *file, int line, const char *func, void *reserved) {
        const auto &loggers = instance()->loggers_;
        for (size_t i = 0; i < loggers.size(); ++i) {
            loggers[i]->logMessage(msg, (LogLevel)level, category, time, file, line, func);
        }
    }

    static void logWrite(const char *data, size_t size, int level, const char *category, void *reserved) {
        const auto &loggers = instance()->loggers_;
        for (size_t i = 0; i < loggers.size(); ++i) {
            loggers[i]->write(data, size, (LogLevel)level, category);
        }
    }

    static int logEnabled(int level, const char *category, void *reserved) {
        int minLevel = NO_LOG_LEVEL;
        const auto &loggers = instance()->loggers_;
        for (size_t i = 0; i < loggers.size(); ++i) {
            const int level = loggers[i]->categoryLevel(category);
            if (level < minLevel) {
                minLevel = level;
            }
        }
        return (level >= minLevel);
    }
};

inline const char* strchrend(const char* s, char c) {
    const char* result = strchr(s, c);
    return result ? result : s + strlen(s);
}

} // namespace

// spark::Logger
spark::Logger::Logger(LogLevel level, const Filters &filters) :
        level_(level) {
    for (auto it = filters.begin(); it != filters.end(); ++it) {
        const char* const category = it->first;
        const LogLevel level = it->second;
        std::vector<FilterData> *filters = &filters_; // Root categories
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
                    [&found](const FilterData &filter, const std::pair<const char*, size_t> &value) {
                const int cmp = std::strncmp(filter.name, value.first, std::min(filter.size, value.second));
                if (cmp == 0) {
                    if (filter.size == value.second) {
                        found = true;
                    }
                    return filter.size < value.second;
                }
                return cmp < 0;
            });
            if (!found) {
                it = filters->insert(it, FilterData(name, size)); // Add subcategory
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

LogLevel spark::Logger::categoryLevel(const char *category) const {
    if (!category || filters_.empty()) {
        return level_; // Default level
    }
    LogLevel level = level_;
    const std::vector<FilterData> *filters = &filters_; // Root categories
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
                [&found](const FilterData &filter, const std::pair<const char*, size_t> &value) {
            const int cmp = std::strncmp(filter.name, value.first, std::min(filter.size, value.second));
            if (cmp == 0) {
                if (filter.size == value.second) {
                    found = true;
                }
                return filter.size < value.second;
            }
            return cmp < 0;
        });
        if (!found) {
            break;
        }
        if (it->level >= 0) {
            level = (LogLevel)it->level;
        }
        if (!category[i]) {
            break;
        }
        filters = &it->filters;
        pos = i + 1;
    }
    return level;
}

void spark::Logger::formatMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
        const char *file, int line, const char *func) {
    // Timestamp
    char buf[16];
    snprintf(buf, sizeof(buf), "%010u ", (unsigned)time);
    write(buf);
    // Category (optional)
    if (category && category[0]) {
        write(category);
        write(": ");
    }
    // Source info (optional)
    if (file && func) {
        write(file);
        write(":");
        snprintf(buf, sizeof(buf), "%d", line);
        write(buf);
        write(", ");
        // Strip argument and return types for better readability
        int n = 0;
        const char *p = strchrend(func, ' ');
        if (*p) {
            p += 1;
            n = strchrend(p, '(') - p;
        } else {
            n = p - func;
            p = func;
        }
        write(p, n);
        write("(): ");
    }
    // Level
    write(levelName(level));
    write(": ");
    // Message
    write(msg);
    write("\r\n");
}

void spark::Logger::install(Logger *logger) {
    LogHandler::instance()->addLogger(logger);
}

void spark::Logger::uninstall(Logger *logger) {
    LogHandler::instance()->removeLogger(logger);
}
