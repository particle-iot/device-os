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
    typedef std::pair<const char*, LogLevel> CategoryFilter;
    typedef std::initializer_list<CategoryFilter> CategoryFilters;

    explicit Logger(LogLevel level = ALL_LEVEL, const CategoryFilters &filters = {});
    virtual ~Logger() = default;

    void logMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func);
    void write(const char *data, size_t size, LogLevel level, const char *category);

    LogLevel defaultLevel() const;
    LogLevel categoryLevel(const char *category) const;

    static const char* levelName(LogLevel level);

    static void install(Logger *logger);
    static void uninstall(Logger *logger);

protected:
    virtual void formatMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func);
    virtual void write(const char *data, size_t size) = 0;

    void write(const char *data);

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
    LogLevel level_;
};

} // namespace spark

// spark::Logger
inline void spark::Logger::logMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
        const char *file, int line, const char *func) {
    if (level >= categoryLevel(category)) {
        formatMessage(msg, level, category, time, file, line, func);
    }
}

inline void spark::Logger::write(const char *data, size_t size, LogLevel level, const char *category) {
    if (level >= categoryLevel(category)) {
        write(data, size);
    }
}

inline void spark::Logger::write(const char *str) {
    write(str, strlen(str));
}

inline LogLevel spark::Logger::defaultLevel() const {
    return level_;
}

inline const char* spark::Logger::levelName(LogLevel level) {
    return log_level_name(level, nullptr);
}

#endif // SPARK_WIRING_LOGGING_H
