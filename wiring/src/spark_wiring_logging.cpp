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
#include "spark_wiring_usbserial.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_json.h"

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

int addLogHandler(const log_config_add_handler_command* cmd) {
    if (cmd->version != LOG_CONFIG_API_VERSION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    LogCategoryFilters filters;
    for (size_t i = 0; i < cmd->filter_count; ++i) {
        const auto& f = cmd->filters[i];
        CHECK_TRUE(filters.append(LogCategoryFilter(f.category, (LogLevel)f.level)), SYSTEM_ERROR_NO_MEMORY);
    }
    const auto params = (const log_config_serial_params*)cmd->params;
    const auto mgr = LogManager::instance();
    CHECK_TRUE(mgr->addFactoryHandler(cmd->id, (log_config_handler_type)cmd->type, (LogLevel)cmd->level,
            std::move(filters), params), SYSTEM_ERROR_UNKNOWN);
    return 0;
}

int removeLogHandler(const log_config_remove_handler_command* cmd) {
    if (cmd->version != LOG_CONFIG_API_VERSION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    const auto mgr = LogManager::instance();
    mgr->removeFactoryHandler(cmd->id);
    return 0;
}

int getLogHandlers(const log_config_get_handlers_command* cmd, log_config_get_handlers_result* result) {
    if (cmd->version != LOG_CONFIG_API_VERSION) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    result->handlers = nullptr;
    result->handler_count = 0;
    NAMED_SCOPE_GUARD(resultGuard, {
        for (size_t i = 0; i < result->handler_count; ++i) {
            free(result->handlers[i].id);
        }
        free(result->handlers);
    });
    const auto mgr = LogManager::instance();
    const bool ok = mgr->enumFactoryHandlers([](const char* id, void* data) {
        const auto result = (log_config_get_handlers_result*)data;
        ++result->handler_count;
        const auto handlers = (log_config_handler_list_item*)realloc(result->handlers,
                sizeof(log_config_handler_list_item) * result->handler_count);
        if (!handlers) {
            return false;
        }
        result->handlers = handlers;
        result->handlers[result->handler_count - 1].id = strdup(id);
        if (!result->handlers[result->handler_count - 1].id) {
            return false;
        }
        return true;
    }, result);
    if (!ok) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    resultGuard.dismiss();
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
    JSONStreamWriter json(*this->stream());
    json.beginObject();
    // Level
    const char *s = levelName(level);
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

#if Wiring_LogConfig

// spark::DefaultLogHandlerFactory
LogHandler* spark::DefaultLogHandlerFactory::createHandler(LogLevel level, LogCategoryFilters filters, Print *stream) {
    return new(std::nothrow) JSONStreamLogHandler(*stream, level, std::move(filters)); // FIXME
}

spark::DefaultLogHandlerFactory* spark::DefaultLogHandlerFactory::instance() {
    static DefaultLogHandlerFactory factory;
    return &factory;
}

// spark::DefaultOutputStreamFactory
Print* spark::DefaultOutputStreamFactory::createStream(log_config_handler_type type, const log_config_serial_params* params) {
#if PLATFORM_ID != 3
    if (!params) {
        return nullptr;
    }
    switch (type) {
        case LOG_CONFIG_USB_SERIAL: {
            switch (params->index) {
                case 0: {
                    Serial.begin();
                    return &Serial;
                }
#if Wiring_USBSerial1
                case 1: {
                    USBSerial1.begin();
                    return &USBSerial1;
                }
#endif // Wiring_USBSerial1
            }
        }
        case LOG_CONFIG_HW_SERIAL: {
            switch (params->index) {
                case 1: {
                    Serial1.begin(params->baud_rate ? params->baud_rate : 115200);
                    return &Serial1;
                }
            }
        }
    }
#endif // PLATFORM_ID != 3
    return nullptr;
}

void spark::DefaultOutputStreamFactory::destroyStream(Print *stream) {
#if PLATFORM_ID != 3
    if (stream == &Serial) {
        Serial.end();
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
    String id;
    LogHandler *handler;
    Print *stream;
};

#endif // Wiring_LogConfig

spark::LogManager::LogManager() {
#if Wiring_LogConfig
    handlerFactory_ = DefaultLogHandlerFactory::instance();
    streamFactory_ = DefaultOutputStreamFactory::instance();
#endif
    outputActive_ = false;
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

bool spark::LogManager::addFactoryHandler(const char *id, log_config_handler_type handlerType, LogLevel level,
        LogCategoryFilters filters, const log_config_serial_params* streamParams) {
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandler(id); // Destroy an existing handler with the same ID
        FactoryHandler h;
        h.id = id;
        if (!h.id.length()) {
            return false; // Empty handler ID or memory allocation error
        }
        // Create output stream (optional)
        Print* stream = streamFactory_->createStream(handlerType, streamParams); // FIXME
        if (!stream) {
            return false;
        }
        NAMED_SCOPE_GUARD(streamGuard, {
            streamFactory_->destroyStream(stream);
        });
        // Create log handler
        LogHandler* handler = handlerFactory_->createHandler(level, std::move(filters), stream);
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
            if (!callback(h.id.c_str(), data)) {
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
        if (h.id == id) {
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
int spark::logConfig(int cmd, const void* data, void* result, void* userData) {
    switch (cmd) {
    case LOG_CONFIG_ADD_HANDLER:
        return addLogHandler((const log_config_add_handler_command*)data);
    case LOG_CONFIG_REMOVE_HANDLER:
        return removeLogHandler((const log_config_remove_handler_command*)data);
    case LOG_CONFIG_GET_HANDLERS:
        return getLogHandlers((const log_config_get_handlers_command*)data, (log_config_get_handlers_result*)result);
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

#endif // Wiring_LogConfig
