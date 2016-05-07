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
    \brief Abstract log handler.

    This class can be subclassed to implement custom log handlers. The library also provides several
    built-in handlers, such as \ref spark::SerialLogHandler and \ref spark::Serial1LogHandler.
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
        \brief Constructor.
        \param level Default logging level.
        \param filters Category filters.
    */
    explicit LogHandler(LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {});
    /*!
        \brief Destructor.
    */
    virtual ~LogHandler();
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

    // These methods are called by the LogManager instance
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

        Resulting string is then written to output stream via \ref write(const char*, size_t) method.
    */
    virtual void logMessage(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info);
    /*!
        \brief Writes buffer to output stream.
        \param data Characters buffer.
        \param size Buffer size.

        This method should be implemented by all subclasses.
    */
    virtual void write(const char *data, size_t size) = 0;
    /*!
        \brief Writes string to output stream.

        This method is equivalent to `write(str, strlen(str))`.
    */
    void write(const char *str);

private:
    struct FilterData;

    std::vector<FilterData> filters_;
    LogLevel level_;
};

/*!
    \brief Stream-based log handler.

    Adapter class allowing to use existent stream objects as destination for logging output.
*/
class StreamLogHandler: public LogHandler {
public:
    /*!
        \brief Output stream type.
    */
    typedef Print Stream;

    /*!
        \brief Constructor.
        \param stream Output stream.
        \param level Default logging level.
        \param filters Category filters.
    */
    explicit StreamLogHandler(Stream &stream, LogLevel level = LOG_LEVEL_INFO, const Filters &filters = {});
    /*!
        \brief Returns output stream.
    */
    Stream* stream() const;

protected:
    /*!
        \brief Writes buffer to output stream.
        \param data Characters buffer.
        \param size Buffer size.

        This method is equivalent to `stream()->write((const uint8_t*)data, size)`.
    */
    virtual void write(const char *data, size_t size) override;

private:
    Stream *stream_;
};

} // namespace spark

// spark::LogHandler
inline void spark::LogHandler::message(const char *msg, LogLevel level, const char *category, uint32_t time, const SourceInfo &info) {
    if (level >= categoryLevel(category)) {
        logMessage(msg, level, category, time, info);
    }
}

inline void spark::LogHandler::write(const char *data, size_t size, LogLevel level, const char *category) {
    if (level >= categoryLevel(category)) {
        write(data, size);
    }
}

inline void spark::LogHandler::write(const char *str) {
    write(str, strlen(str));
}

inline LogLevel spark::LogHandler::defaultLevel() const {
    return level_;
}

inline const char* spark::LogHandler::levelName(LogLevel level) {
    return log_level_name(level, nullptr);
}

// spark::StreamLogHandler
inline spark::StreamLogHandler::StreamLogHandler(Stream &stream, LogLevel level, const Filters &filters) :
        LogHandler(level, filters),
        stream_(&stream) {
}

inline void spark::StreamLogHandler::write(const char *data, size_t size) {
    stream_->write((const uint8_t*)data, size);
}

inline spark::StreamLogHandler::Stream* spark::StreamLogHandler::stream() const {
    return stream_;
}

#endif // SPARK_WIRING_LOGGING_H
