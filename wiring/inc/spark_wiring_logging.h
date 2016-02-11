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

#include "service_debug.h"

namespace spark {

class Logger {
public:
    typedef std::pair<const char*, LoggerOutputLevel> CategoryFilter;
    typedef std::initializer_list<CategoryFilter> CategoryFilters;

    explicit Logger(LoggerOutputLevel level = WARN_LEVEL, const CategoryFilters &filters = {});
    virtual ~Logger() = default;

    void logMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func);
    void logString(const char *str, LoggerOutputLevel level);

    static void install(Logger *logger);
    static void uninstall(Logger *logger);

protected:
    virtual void writeMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func) = 0;
    virtual void writeString(const char *str, LoggerOutputLevel level) = 0;

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

    LoggerOutputLevel categoryLevel(const char *category);
};

class FormattingLogger: public Logger {
public:
    using Logger::Logger;

    static const char* levelName(LoggerOutputLevel level);

protected:
    // spark::Logger
    virtual void writeMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func) override;
};

} // namespace spark

// spark::Logger
inline void spark::Logger::logMessage(const char *msg, LoggerOutputLevel level, uint32_t time, const char *category,
            const char *file, int line, const char *func) {
    if (level >= categoryLevel(category)) {
        writeMessage(msg, level, time, category, file, line, func);
    }
}

inline void spark::Logger::logString(const char *str, LoggerOutputLevel level) {
    if (level >= level_) {
        writeString(str, level);
    }
}

#endif // SPARK_WIRING_LOGGING_H
