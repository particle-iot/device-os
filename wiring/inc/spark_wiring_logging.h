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

#include "spark_wiring_print.h"

namespace spark {

/*!
    \brief Log handler.
*/
class LogHandler {
public:
    /*!
        \brief Source file info.
    */
    struct SourceInfo {
        const char* file; //!< File name (can be null).
        const char* function; //!< Function name (can be null).
        int line; //!< Line number.
    };
    /*!
        \brief Category filter.
    */
    typedef std::pair<const char*, LogLevel> Filter;
    /*!
        \brief List of category filters.
    */
    typedef std::initializer_list<Filter> Filters;
    /*!
        \brief Output stream type.
    */
    typedef Print Stream;
    /*!
        \brief Constructor.
        \param level Default logging level.
        \param filters Category filters.
    */
    explicit LogHandler(LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {});
    /*!
        \brief Constructor.
        \param stream Output stream.
        \param level Default logging level.
        \param filters Category filters.
    */
    explicit LogHandler(Stream &stream, LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {});
    /*!
        \brief Destructor.
    */
    virtual ~LogHandler();
    /*!
        \brief Returns output stream (can be null).
    */
    Stream* stream() const;
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
        \brief Registers log handler globally.
        \param logger Handler instance.

        Note that the library doesn't take ownership over handler objects.
    */
    static void install(LogHandler *handler);
    /*!
        \brief Unregisters log handler.
        \param logger Handler instance.
    */
    static void uninstall(LogHandler *handler);

    // These methods are called by the system modules
    void message(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info);
    void write(const char *data, size_t size, LogLevel level, const char *category);

protected:
    /*!
        \brief Processes log message.
        \param msg Text message.
        \param level Logging level.
        \param category Category name (can be null).
        \param time Timestamp (milliseconds since startup).
        \param info Source file info.

        Default implementation generates messages in the following format:
        `<timestamp>: [category]: [file]:[line], [function]: <level>: <message>`.

        Resulting string is then written to the output stream.
    */
    virtual void logMessage(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info);
    /*!
        \brief Writes buffer to the output stream.
        \param data Characters buffer.
        \param size Buffer size.

        This method is equivalent to `stream()->write((const uint8_t*)data, size)`.
    */
    void write(const char *data, size_t size);
    /*!
        \brief Writes string to the output stream.

        This method is equivalent to `write(str, strlen(str))`.
    */
    void write(const char *str);

private:
    struct FilterData;

    std::vector<FilterData> filters_;
    Stream *stream_;
    LogLevel level_;

    LogHandler(Stream *stream, LogLevel level, const Filters &filters);
};

} // namespace spark

// spark::LogHandler
inline spark::LogHandler::LogHandler(LogLevel level, const Filters &filters) :
        LogHandler(nullptr, level, filters) {
}

inline spark::LogHandler::LogHandler(Stream &stream, LogLevel level, const Filters &filters) :
        LogHandler(&stream, level, filters) {
}

inline void spark::LogHandler::message(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info) {
    if (level >= categoryLevel(category)) {
        logMessage(msg, level, category, time, info);
    }
}

inline void spark::LogHandler::write(const char *data, size_t size, LogLevel level, const char *category) {
    if (stream_ && level >= categoryLevel(category)) {
        stream_->write((const uint8_t*)data, size);
    }
}

inline void spark::LogHandler::write(const char *data, size_t size) {
    if (stream_) {
        stream_->write((const uint8_t*)data, size);
    }
}

inline void spark::LogHandler::write(const char *str) {
    write(str, strlen(str));
}

inline spark::LogHandler::Stream* spark::LogHandler::stream() const {
    return stream_;
}

inline LogLevel spark::LogHandler::defaultLevel() const {
    return level_;
}

inline const char* spark::LogHandler::levelName(LogLevel level) {
    return log_level_name(level, nullptr);
}

#endif // SPARK_WIRING_LOGGING_H
