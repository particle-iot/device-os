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

#ifndef SPARK_WIRING_LOGGING_H
#define SPARK_WIRING_LOGGING_H

#include <initializer_list>
#include <vector>
#include <cstring>

#include "logging.h"

namespace spark {

class Logger {
public:
    typedef std::pair<const char*, LoggerOutputLevel> CategoryFilter;
    typedef std::initializer_list<CategoryFilter> CategoryFilters;

    explicit Logger(LoggerOutputLevel level = WARN_LEVEL, const CategoryFilters &filters = {});
    virtual ~Logger() = default;

    void logMessage(const char *msg, LoggerOutputLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func);
    void logData(const char *data, size_t size, LoggerOutputLevel level, const char *category);

    LoggerOutputLevel defaultLevel() const;
    LoggerOutputLevel categoryLevel(const char *category) const;

    static void install(Logger *logger);
    static void uninstall(Logger *logger);

protected:
    virtual void writeMessage(const char *msg, LoggerOutputLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func) = 0;
    virtual void writeData(const char *data, size_t size, LoggerOutputLevel level, const char *category) = 0;

private:
    struct Filter {
        const char *name; // Category name
        size_t size; // Name length
        int level; // Logging level (-1 if not specified for this category)
        std::vector<Filter> filters; // Subcategories of this category

        Filter(const char *name, size_t size) :
                name(name),
                size(size),
                level(-1) {
        }
    };

    std::vector<Filter> filters_;
    LoggerOutputLevel level_;
};

class FormattingLogger: public Logger {
public:
    using Logger::Logger;

    static const char* levelName(LoggerOutputLevel level);

protected:
    virtual void write(const char *data, size_t size) = 0;

    // spark::Logger
    virtual void writeMessage(const char *msg, LoggerOutputLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func) override;
    virtual void writeData(const char *data, size_t size, LoggerOutputLevel level, const char *category) override;

    void write(const char *str);
};

} // namespace spark

// spark::Logger
inline void spark::Logger::logMessage(const char *msg, LoggerOutputLevel level, const char *category, uint32_t time,
        const char *file, int line, const char *func) {
    if (level >= categoryLevel(category)) {
        writeMessage(msg, level, category, time, file, line, func);
    }
}

inline void spark::Logger::logData(const char *data, size_t size, LoggerOutputLevel level, const char *category) {
    if (level >= categoryLevel(category)) {
        writeData(data, size, level, category);
    }
}

inline LoggerOutputLevel spark::Logger::defaultLevel() const {
    return level_;
}

// spark::FormattingLogger
inline const char* spark::FormattingLogger::levelName(LoggerOutputLevel level) {
    return log_level_name(level, nullptr);
}

inline void spark::FormattingLogger::writeData(const char *data, size_t size, LoggerOutputLevel, const char*) {
    write(data, size);
}

inline void spark::FormattingLogger::write(const char *str) {
    write(str, strlen(str));
}

#endif // SPARK_WIRING_LOGGING_H
