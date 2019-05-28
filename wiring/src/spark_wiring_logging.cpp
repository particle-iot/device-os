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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for strchrnul()
#endif
#include <cstring>

#include "spark_wiring_logging.h"
#include "spark_wiring_network.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_json.h"

#include "c_string.h"
#include "scope_guard.h"
#include "check.h"

#include <algorithm>
#include <cinttypes>

// Uncomment to enable logging in interrupt handlers
// #define LOG_FROM_ISR

#if defined(LOG_FROM_ISR) && PLATFORM_ID != 3
// When compiled with LOG_FROM_ISR defined use ATOMIC_BLOCK
#define LOG_WITH_LOCK(x) ATOMIC_BLOCK()
#else
// Otherwise use mutex
#define LOG_WITH_LOCK(x) WITH_LOCK(x)
#endif

namespace {

using namespace particle;
using namespace spark;

#if PLATFORM_ID == 3
// GCC on some platforms doesn't provide strchrnul()
inline const char* strchrnul(const char *s, char c) {
    while (*s && *s != c) {
        ++s;
    }
    return s;
}
#endif

// Iterates over subcategory names separated by '.' character
const char* nextSubcategoryName(const char* &category, size_t &size) {
    const char *s = strchrnul(category, '.');
    size = s - category;
    if (size) {
        if (*s) {
            ++s;
        }
        std::swap(s, category);
        return s;
    }
    return nullptr;
}

const char* extractFileName(const char *s) {
    const char *s1 = strrchr(s, '/');
    if (s1) {
        return s1 + 1;
    }
    return s;
}

const char* extractFuncName(const char *s, size_t *size) {
    const char *s1 = s;
    for (; *s; ++s) {
        if (*s == ' ') {
            s1 = s + 1; // Skip return type
        } else if (*s == '(') {
            break; // Skip argument types
        }
    }
    *size = s - s1;
    return s1;
}

#if Wiring_LogConfig

int checkStreamSettings(const log_stream* stream) {
    CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_ARGUMENT);
    switch (stream->type) {
    case LOG_USB_SERIAL_STREAM: {
        const auto serial = (const log_serial_stream*)stream;
        switch (serial->index) {
        case 0: // Serial
#if Wiring_USBSerial1
        case 1: // USBSerial1
#endif
            return 0; // OK
        }
        break;
    }
    case LOG_HW_SERIAL_STREAM: {
        const auto serial = (const log_serial_stream*)stream;
        switch (serial->index) {
        case 1: // Serial1
            return 0;
            // TODO: Serial2, Serial3, etc.
        }
        break;
    }
    default:
        break;
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int checkHandlerSettings(const log_handler* handler, const log_stream* stream) {
    CHECK_TRUE(handler, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(handler->id && *handler->id, SYSTEM_ERROR_INVALID_ARGUMENT);
    for (auto f = handler->filters; f; f = f->next) {
        CHECK_TRUE(f->category && *f->category, SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    switch (handler->type) {
    case LOG_DEFAULT_STREAM_HANDLER:
    case LOG_JSON_STREAM_HANDLER: {
        CHECK(checkStreamSettings(stream));
        return 0; // OK
    }
    default:
        break;
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

inline bool isSerialStreamType(log_stream_type type) {
    return (type == LOG_USB_SERIAL_STREAM || type == LOG_HW_SERIAL_STREAM);
}

int addLogHandler(const log_add_handler_command* cmd, log_command_result** result) {
    CHECK(checkHandlerSettings(cmd->handler, cmd->stream)); // Validate handler settings
    struct Result: log_command_result {
        LogCategoryFilters filters;
        CString handlerId;
        log_handler_type handlerType;
        LogLevel level;
        log_stream_type streamType;
        unsigned streamIndex;
        unsigned baudRate;
    };
    std::unique_ptr<Result> r(new(std::nothrow) Result());
    CHECK_TRUE(r, SYSTEM_ERROR_NO_MEMORY);
    r->version = LOG_SYSTEM_API_VERSION; // API version
    r->handlerId = cmd->handler->id;
    CHECK_TRUE(r->handlerId, SYSTEM_ERROR_NO_MEMORY);
    r->handlerType = (log_handler_type)cmd->handler->type;
    r->level = (LogLevel)cmd->handler->level;
    if (cmd->stream) {
        r->streamType = (log_stream_type)cmd->stream->type;
        if (isSerialStreamType(r->streamType)) {
            const auto serial = (const log_serial_stream*)cmd->stream;
            r->streamIndex = serial->index;
            r->baudRate = serial->baud_rate;
        }
    }
    r->filters.reserve(cmd->handler->filter_count);
    for (auto f = cmd->handler->filters; f; f = f->next) {
        CHECK_TRUE(r->filters.append(LogCategoryFilter(f->category, (LogLevel)f->level)), SYSTEM_ERROR_NO_MEMORY);
    }
    r->deleter_fn = [](log_command_result* result) { // Deleter function
        delete (Result*)result;
    };
    // Initialization of a log handler that uses a USB serial as a destination stream may cause the
    // device to be detached from the host, rendering the client application running on the host
    // unable to receive a reply for the previously sent configuration request. As a workaround,
    // the log handler is initialized asynchronously, after the system reports that the host has
    // received the reply successfully
    r->completion_fn = [](int error, log_command_result* result) { // Completion callback
        if (error == 0) {
            const auto r = (Result*)result;
            const auto mgr = LogManager::instance();
            mgr->addFactoryHandler(r->handlerId, r->handlerType, r->level, std::move(r->filters), r->streamType,
                    r->streamIndex, r->baudRate);
        }
    };
    *result = r.release(); // Transfer ownership over the result data
    return 0;
}

int removeLogHandler(const log_remove_handler_command* cmd, log_command_result** result) {
    const auto mgr = LogManager::instance();
    CHECK_TRUE(cmd->id && *cmd->id, SYSTEM_ERROR_INVALID_ARGUMENT);
    mgr->removeFactoryHandler(cmd->id);
    return 0;
}

int getLogHandlers(const log_command* cmd, log_get_handlers_result** result) {
    struct Handler: log_handler_list_item {
        CString handlerId;
    };
    struct Result: log_get_handlers_result {
        Vector<Handler> handlerList;
    };
    std::unique_ptr<Result> r(new(std::nothrow) Result());
    CHECK_TRUE(r, SYSTEM_ERROR_NO_MEMORY);
    r->result.version = LOG_SYSTEM_API_VERSION; // API version
    const auto mgr = LogManager::instance();
    const bool ok = mgr->enumFactoryHandlers([](const char* id, void* data) {
        const auto r = (Result*)data;
        Handler h = {};
        h.handlerId = id;
        if (!h.handlerId) {
            return false; // Memory allocation error
        }
        h.id = h.handlerId; // Handler ID
        if (!r->handlerList.append(std::move(h))) {
            return false;
        }
        if (r->handlerList.size() > 1) {
            r->handlerList.at(r->handlerList.size() - 2).next = &r->handlerList.last();
        }
        ++r->handler_count; // Number of active handlers
        return true;
    }, r.get());
    if (!ok) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    r->handlers = r->handlerList.data(); // List of active handlers
    r->result.deleter_fn = [](log_command_result* result) { // Deleter function
        delete (Result*)result;
    };
    *result = r.release(); // Transfer ownership over the result data
    return 0;
}

#endif // Wiring_LogConfig

} // unnamed

// Default logger instance. This code is compiled as part of the wiring library which has its own
// category name specified at module level, so here we use "app" category name explicitly
const spark::Logger spark::Log("app");

/*
    LogFilter instance maintains a prefix tree based on a list of category filter strings. Every
    node of the tree contains a subcategory name and, optionally, a logging level - if node matches
    complete filter string. For example, given the following filters:

    a (error)
    a.b.c (trace)
    a.b.x (trace)
    aa (error)
    aa.b (warn)

    LogFilter builds the following prefix tree:

    |
    |- a (error) -- b - c (trace)
    |               |
    |               `-- x (trace)
    |
    `- aa (error) - b (warn)
*/

// spark::detail::LogFilter
struct spark::detail::LogFilter::Node {
    const char *name; // Subcategory name
    uint16_t size; // Name length
    int16_t level; // Logging level (-1 if not specified for this node)
    Vector<Node> nodes; // Children nodes

    Node(const char *name, uint16_t size) :
            name(name),
            size(size),
            level(-1) {
    }
};

spark::detail::LogFilter::LogFilter(LogLevel level) :
        level_(level) {
}

spark::detail::LogFilter::LogFilter(LogLevel level, LogCategoryFilters filters) :
        level_(LOG_LEVEL_NONE) { // Fallback level that will be used in case of construction errors
    // Store category names
    Vector<String> cats;
    if (!cats.reserve(filters.size())) {
        return;
    }
    for (LogCategoryFilter &filter: filters) {
        cats.append(std::move(filter.cat_));
    }
    // Process category filters
    Vector<Node> nodes;
    for (int i = 0; i < cats.size(); ++i) {
        const char *category = cats.at(i).c_str();
        if (!category) {
            continue; // Invalid usage or string allocation error
        }
        Vector<Node> *pNodes = &nodes; // Root nodes
        const char *name = nullptr; // Subcategory name
        size_t size = 0; // Name length
        while ((name = nextSubcategoryName(category, size))) {
            bool found = false;
            const int index = nodeIndex(*pNodes, name, size, found);
            if (!found && !pNodes->insert(index, Node(name, size))) { // Add node
                return;
            }
            Node &node = pNodes->at(index);
            if (!*category) { // Check if it's last subcategory
                node.level = filters.at(i).level_;
            }
            pNodes = &node.nodes;
        }
    }
    using std::swap;
    swap(cats_, cats);
    swap(nodes_, nodes);
    level_ = level;
}

spark::detail::LogFilter::~LogFilter() {
}

LogLevel spark::detail::LogFilter::level(const char *category) const {
    LogLevel level = level_; // Default level
    if (!nodes_.isEmpty() && category) {
        const Vector<Node> *pNodes = &nodes_; // Root nodes
        const char *name = nullptr; // Subcategory name
        size_t size = 0; // Name length
        while ((name = nextSubcategoryName(category, size))) {
            bool found = false;
            const int index = nodeIndex(*pNodes, name, size, found);
            if (!found) {
                break;
            }
            const Node &node = pNodes->at(index);
            if (node.level >= 0) {
                level = (LogLevel)node.level;
            }
            pNodes = &node.nodes;
        }
    }
    return level;
}

int spark::detail::LogFilter::nodeIndex(const Vector<Node> &nodes, const char *name, size_t size, bool &found) {
    // Using binary search to find existent node or suitable position for new node
    return std::distance(nodes.begin(), std::lower_bound(nodes.begin(), nodes.end(), std::make_pair(name, size),
            [&found](const Node &node, const std::pair<const char*, size_t> &value) {
                const int cmp = strncmp(node.name, value.first, std::min<size_t>(node.size, value.second));
                if (cmp == 0) {
                    if (node.size == value.second) { // Lengths are equal
                        found = true; // Allows caller code to avoid extra call to strncmp()
                        return false;
                    }
                    return node.size < value.second;
                }
                return cmp < 0;
            }));
}

// spark::StreamLogHandler
void spark::StreamLogHandler::logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr) {
    // TODO: Move this check to a base class (see also JSONStreamLogHandler::logMessage())
#if PLATFORM_ID != PLATFORM_GCC
    if (stream_ == &Serial && Network.listening()) {
        return; // Do not mix logging and serial console output
    }
#endif
    const char *s = nullptr;
    // Timestamp
    if (attr.has_time) {
        printf("%010u ", (unsigned)attr.time);
    }
    // Category
    if (category) {
        write('[');
        write(category);
        write("] ", 2);
    }
    // Source file
    if (attr.has_file) {
        s = extractFileName(attr.file); // Strip directory path
        write(s); // File name
        if (attr.has_line) {
            write(':');
            printf("%d", (int)attr.line); // Line number
        }
        if (attr.has_function) {
            write(", ", 2);
        } else {
            write(": ", 2);
        }
    }
    // Function name
    if (attr.has_function) {
        size_t n = 0;
        s = extractFuncName(attr.function, &n); // Strip argument and return types
        write(s, n);
        write("(): ", 4);
    }
    // Level
    s = levelName(level);
    write(s);
    write(": ", 2);
    // Message
    if (msg) {
        write(msg);
    }
    // Additional attributes
    if (attr.has_code || attr.has_details) {
        write(" [", 2);
        // Code
        if (attr.has_code) {
            write("code = ", 7);
            printf("%" PRIiPTR, (intptr_t)attr.code);
        }
        // Details
        if (attr.has_details) {
            if (attr.has_code) {
                write(", ", 2);
            }
            write("details = ", 10);
            write(attr.details);
        }
        write(']');
    }
    write("\r\n", 2);
}

// spark::JSONStreamLogHandler
void spark::JSONStreamLogHandler::logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr) {
    // TODO: Move this check to a base class (see also StreamLogHandler::logMessage())
#if PLATFORM_ID != PLATFORM_GCC
    if (this->stream() == &Serial && Network.listening()) {
        return; // Do not mix logging and serial console output
    }
#endif
    JSONStreamWriter json(*this->stream());
    json.beginObject();
    // Level
    const char *s = jsonLevelName(level);
    json.name("l", 1).value(s);
    // Message
    if (msg) {
        json.name("m", 1).value(msg);
    }
    // Category
    if (category) {
        json.name("c", 1).value(category);
    }
    // File name
    if (attr.has_file) {
        s = extractFileName(attr.file); // Strip directory path
        json.name("f", 1).value(s);
    }
    // Line number
    if (attr.has_line) {
        json.name("ln", 2).value(attr.line);
    }
    // Function name
    if (attr.has_function) {
        size_t n = 0;
        s = extractFuncName(attr.function, &n); // Strip argument and return types
        json.name("fn", 2).value(s, n);
    }
    // Timestamp
    if (attr.has_time) {
        json.name("t", 1).value((unsigned)attr.time);
    }
    // Code (additional attribute)
    if (attr.has_code) {
        json.name("code", 4).value((int)attr.code);
    }
    // Details (additional attribute)
    if (attr.has_details) {
        json.name("detail", 6).value(attr.details);
    }
    json.endObject();
    this->stream()->write((const uint8_t*)"\r\n", 2);
}

const char* spark::JSONStreamLogHandler::jsonLevelName(int level) {
    if (level >= LOG_LEVEL_ERROR) {
        return "e";
    } else if (level >= LOG_LEVEL_WARN) {
        return "w";
    } else if (level >= LOG_LEVEL_INFO) {
        return "i";
    } else {
        return "t";
    }
}

#if Wiring_LogConfig

// spark::DefaultLogHandlerFactory
LogHandler* spark::DefaultLogHandlerFactory::createHandler(log_handler_type type, LogLevel level,
        LogCategoryFilters filters, Print* stream) {
    switch (type) {
    case LOG_DEFAULT_STREAM_HANDLER:
        if (!stream) {
            return nullptr; // Output stream is not specified
        }
        return new(std::nothrow) StreamLogHandler(*stream, level, std::move(filters));
    case LOG_JSON_STREAM_HANDLER:
        if (!stream) {
            return nullptr;
        }
        return new(std::nothrow) JSONStreamLogHandler(*stream, level, std::move(filters));
    }
    return nullptr;
}

spark::DefaultLogHandlerFactory* spark::DefaultLogHandlerFactory::instance() {
    static DefaultLogHandlerFactory factory;
    return &factory;
}

// spark::DefaultOutputStreamFactory
Print* spark::DefaultOutputStreamFactory::createStream(log_stream_type type, unsigned index, unsigned baudRate) {
#if PLATFORM_ID != 3
    switch (type) {
    case LOG_USB_SERIAL_STREAM: {
        switch (index) {
        case 0:
            Serial.begin();
            return &Serial;
#if Wiring_USBSerial1
        case 1:
            USBSerial1.begin();
            return &USBSerial1;
#endif
        }
        break;
    }
    case LOG_HW_SERIAL_STREAM: {
        if (!baudRate) {
            baudRate = 115200; // Default baud rate
        }
        switch (index) {
        case 1:
            Serial1.begin(baudRate);
            return &Serial1;
            // TODO: Serial2, Serial3, etc.
        }
        break;
    }
    default:
        break;
    }
#endif // PLATFORM_ID != 3
    return nullptr;
}

void spark::DefaultOutputStreamFactory::destroyStream(Print *stream) {
#if PLATFORM_ID != 3
    if (stream == &Serial) {
        // FIXME: Uninitializing the primary USB serial detaches a Gen 3 device from the host
        // Serial.end();
        return;
    }
#if Wiring_USBSerial1
    if (stream == &USBSerial1) {
        USBSerial1.end();
        return;
    }
#endif
    if (stream == &Serial1) {
        Serial1.end();
        return;
    }
#endif // PLATFORM_ID != 3
    OutputStreamFactory::destroyStream(stream);
}

spark::DefaultOutputStreamFactory* spark::DefaultOutputStreamFactory::instance() {
    static DefaultOutputStreamFactory factory;
    return &factory;
}

// spark::LogManager
struct spark::LogManager::FactoryHandler {
    CString id;
    LogHandler *handler;
    Print *stream;
};

#endif // Wiring_LogConfig

spark::LogManager::LogManager() :
        outputActive_(false) {
#if Wiring_LogConfig
    handlerFactory_ = DefaultLogHandlerFactory::instance();
    streamFactory_ = DefaultOutputStreamFactory::instance();
#endif
}

spark::LogManager::~LogManager() {
    resetSystemCallbacks();
#if Wiring_LogConfig
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandlers();
    }
#endif
}

bool spark::LogManager::addHandler(LogHandler *handler) {
    LOG_WITH_LOCK(mutex_) {
        if (activeHandlers_.contains(handler) || !activeHandlers_.append(handler)) {
            return false;
        }
        if (activeHandlers_.size() == 1) {
            setSystemCallbacks();
        }
    }
    return true;
}

void spark::LogManager::removeHandler(LogHandler *handler) {
    LOG_WITH_LOCK(mutex_) {
        if (activeHandlers_.removeOne(handler) && activeHandlers_.isEmpty()) {
            resetSystemCallbacks();
        }
    }
}

spark::LogManager* spark::LogManager::instance() {
    static LogManager mgr;
    return &mgr;
}

#if Wiring_LogConfig

bool spark::LogManager::addFactoryHandler(const char *id, log_handler_type handlerType, LogLevel level,
        LogCategoryFilters filters, log_stream_type streamType, unsigned streamIndex, unsigned baudRate) {
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandler(id); // Destroy an existing handler with the same ID
        FactoryHandler h = {};
        h.id = id;
        if (!h.id || !*h.id) {
            return false; // Empty handler ID or memory allocation error
        }
        // Create output stream (optional)
        Print* stream = streamFactory_->createStream(streamType, streamIndex, baudRate);
        NAMED_SCOPE_GUARD(streamGuard, {
            streamFactory_->destroyStream(stream);
        });
        // Create log handler
        LogHandler* handler = handlerFactory_->createHandler(handlerType, level, std::move(filters), stream);
        if (!handler) {
            return false;
        }
        NAMED_SCOPE_GUARD(handlerGuard, {
            handlerFactory_->destroyHandler(handler);
        });
        h.handler = handler;
        h.stream = stream;
        if (!factoryHandlers_.append(std::move(h))) {
            return false;
        }
        NAMED_SCOPE_GUARD(handlerListGuard, {
            factoryHandlers_.takeLast(); // Revert factoryHandlers_.append()
        });
        if (!activeHandlers_.append(handler)) {
            return false;
        }
        if (activeHandlers_.size() == 1) {
            setSystemCallbacks();
        }
        streamGuard.dismiss();
        handlerGuard.dismiss();
        handlerListGuard.dismiss();
    }
    return true;
}

void spark::LogManager::removeFactoryHandler(const char *id) {
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandler(id);
    }
}

bool spark::LogManager::enumFactoryHandlers(bool(*callback)(const char *id, void *data), void *data) {
    LOG_WITH_LOCK(mutex_) {
        for (const FactoryHandler &h: factoryHandlers_) {
            if (!callback(h.id, data)) {
                return false;
            }
        }
    }
    return true;
}

void spark::LogManager::setHandlerFactory(LogHandlerFactory *factory) {
    LOG_WITH_LOCK(mutex_) {
        if (handlerFactory_ != factory) {
            destroyFactoryHandlers();
            handlerFactory_ = factory;
        }
    }
}

void spark::LogManager::setStreamFactory(OutputStreamFactory *factory) {
    LOG_WITH_LOCK(mutex_) {
        if (streamFactory_ != factory) {
            destroyFactoryHandlers();
            streamFactory_ = factory;
        }
    }
}

void spark::LogManager::destroyFactoryHandler(const char *id) {
    for (int i = 0; i < factoryHandlers_.size(); ++i) {
        const FactoryHandler &h = factoryHandlers_.at(i);
        if (strcmp(h.id, id) == 0) {
            activeHandlers_.removeOne(h.handler);
            if (activeHandlers_.isEmpty()) {
                resetSystemCallbacks();
            }
            handlerFactory_->destroyHandler(h.handler);
            if (h.stream) {
                streamFactory_->destroyStream(h.stream);
            }
            factoryHandlers_.removeAt(i);
            break;
        }
    }
}

void spark::LogManager::destroyFactoryHandlers() {
    for (const FactoryHandler &h: factoryHandlers_) {
        activeHandlers_.removeOne(h.handler);
        if (activeHandlers_.isEmpty()) {
            resetSystemCallbacks();
        }
        handlerFactory_->destroyHandler(h.handler);
        if (h.stream) {
            streamFactory_->destroyStream(h.stream);
        }
    }
    factoryHandlers_.clear();
}

#endif // Wiring_LogConfig

void spark::LogManager::setSystemCallbacks() {
    log_set_callbacks(logMessage, logWrite, logEnabled, nullptr);
}

void spark::LogManager::resetSystemCallbacks() {
    log_set_callbacks(nullptr, nullptr, nullptr, nullptr);
}

void spark::LogManager::logMessage(const char *msg, int level, const char *category, const LogAttributes *attr, void *reserved) {
#ifndef LOG_FROM_ISR
    if (HAL_IsISR()) {
        return;
    }
#endif
    LogManager *that = instance();
    LOG_WITH_LOCK(that->mutex_) {
        // prevent re-entry
        if (that->isActive()) {
            return;
        }
        that->setActive(true);
        for (LogHandler *handler: that->activeHandlers_) {
            handler->message(msg, (LogLevel)level, category, *attr);
        }
        that->setActive(false);
    }
}

void spark::LogManager::logWrite(const char *data, size_t size, int level, const char *category, void *reserved) {
#ifndef LOG_FROM_ISR
    if (HAL_IsISR()) {
        return;
    }
#endif
    LogManager *that = instance();
    LOG_WITH_LOCK(that->mutex_) {
        // prevent re-entry
        if (that->isActive()) {
            return;
        }
        that->setActive(true);
        for (LogHandler *handler: that->activeHandlers_) {
            handler->write(data, size, (LogLevel)level, category);
        }
        that->setActive(false);
    }
}

int spark::LogManager::logEnabled(int level, const char *category, void *reserved) {
#ifndef LOG_FROM_ISR
    if (HAL_IsISR()) {
        return 0;
    }
#endif
    LogManager *that = instance();
    int minLevel = LOG_LEVEL_NONE;
    LOG_WITH_LOCK(that->mutex_) {
        for (LogHandler *handler: that->activeHandlers_) {
            const int level = handler->level(category);
            if (level < minLevel) {
                minLevel = level;
            }
        }
    }
    return (level >= minLevel);
}

inline bool spark::LogManager::isActive() const {
    return outputActive_;
}

inline void spark::LogManager::setActive(bool outputActive) {
    outputActive_ = outputActive;
}

#if Wiring_LogConfig

// spark::
int spark::logCommand(const log_command* cmd, log_command_result** result, void* userData) {
    switch (cmd->type) {
    case LOG_ADD_HANDLER_COMMAND:
        return addLogHandler((const log_add_handler_command*)cmd, result);
    case LOG_REMOVE_HANDLER_COMMAND:
        return removeLogHandler((const log_remove_handler_command*)cmd, result);
    case LOG_GET_HANDLERS_COMMAND:
        return getLogHandlers(cmd, (log_get_handlers_result**)result);
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

#endif // Wiring_LogConfig
