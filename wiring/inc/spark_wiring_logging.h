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

/*!
    \brief Abstract logger.

    This class can be subclassed to implement custom loggers. The library also provides several built-in
    loggers, such as \ref spark::SerialLogger and \ref spark::Serial1Logger.

    Typically, custom logger classes only need to implement \ref write(const char*, size_t) method that will
    write data to some output stream. Optionally, \ref formatMessage() method can be reimplemented to customize
    format of the generated messages.
*/
class Logger {
public:
    /*!
        \brief Category filter.
    */
    typedef std::pair<const char*, LogLevel> Filter;
    /*!
        \brief List of category filters.
    */
    typedef std::initializer_list<Filter> Filters;
    /*!
        \brief Constructor.
        \param level Default logging level.
        \param filters Category filters.
    */
    explicit Logger(LogLevel level = ALL_LEVEL, const Filters &filters = {});
    /*!
        \brief Destructor.
    */
    virtual ~Logger() = default;
    /*!
        \brief Returns default logging level.
    */
    LogLevel defaultLevel() const;
    /*!
        \brief Returns logging level for specified category.
        \param category Category name.
    */
    LogLevel categoryLevel(const char *category) const;
    /*!
        \brief Returns level name.
        \param level Logging level.
    */
    static const char* levelName(LogLevel level);
    /*!
        \brief Registers logger globally.
        \param logger Logger instance.
        \note This method is not thread-safe.
    */
    static void install(Logger *logger);
    /*!
        \brief Unregisters previously registered logger.
        \param logger Logger instance.
        \note This method is not thread-safe.
    */
    static void uninstall(Logger *logger);

    // These methods are called by the system modules
    void logMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func);
    void write(const char *data, size_t size, LogLevel level, const char *category);

protected:
    /*!
        \brief Formats message and writes it to output stream.
        \param msg Text message.
        \param level Logging level.
        \param category Category name (can be null).
        \param time Timestamp (milliseconds since startup).
        \param file Source file name (can be null).
        \param line Line number.
        \param func Function name (can be null).

        Default implementation generates messages in the following format:
        `<timestamp>: [category]: [file]:[line], [function]: <level>: <message>`
    */
    virtual void formatMessage(const char *msg, LogLevel level, const char *category, uint32_t time,
            const char *file, int line, const char *func);
    /*!
        \brief Writes data to output stream.
        \param data Data buffer.
        \param size Data size.

        This method should be implemented by all subclasses.
    */
    virtual void write(const char *data, size_t size) = 0;
    /*!
        \brief Writes string to output stream.

        This method is equivalent to `write(str, strlen(str))`.
    */
    void write(const char *str);

private:
    struct FilterData {
        const char *name; // Category name
        size_t size; // Name length
        int level; // Logging level (-1 if not specified for this category)
        std::vector<FilterData> filters; // Subcategories

        FilterData(const char *name, size_t size) :
                name(name),
                size(size),
                level(-1) {
        }
    };

    std::vector<FilterData> filters_;
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
