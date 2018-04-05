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

#include <algorithm>
#include <cinttypes>
#include <memory>

#include "spark_wiring_usbserial.h"
#include "spark_wiring_usartserial.h"

#include "spark_wiring_interrupts.h"

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

#if Wiring_LogConfig

/*
    This class performs processing of configuration requests in JSON format.

    Adding log handler:

    {
      "cmd": "addHandler", // Command name
      "id": "handler1", // Handler ID
      "hnd": { // Handler settings
        "type": "JSONLogHandler", // Handler type
        "param": { // Additional handler parameters
          ...
        }
      },
      "strm": { // Stream settings
        "type": "Serial1", // Stream type
        "param": { // Additional stream parameters
          ...
        }
      }
      "filt": [ // Category filters
        {
          "app": "all" // Category name and logging level
        }
      ],
      "lvl": "warn" // Default logging level
    }

    Removing log handler:

    {
      "cmd": "removeHandler", // Command name
      "id": "handler1" // Handler ID
    }

    Enumerating active log handlers:

    {
      "cmd": "enumHandlers" // Command name
    }

    Reply example:

    [
      "handler1", "handler2"
    ]
*/
class JSONRequestHandler {
public:
    static void process(ctrl_request* ctrlReq) {
        // TODO: Refactor this code once we introduce a high-level API for the control requests
        const JSONValue jsonReq = JSONValue::parse(ctrlReq->request_data, ctrlReq->request_size);
        if (!jsonReq.isValid()) {
            system_ctrl_set_result(ctrlReq, SYSTEM_ERROR_BAD_DATA, nullptr, nullptr, nullptr); // Parsing error
            return;
        }
        Request req;
        if (!parseRequest(jsonReq, &req)) {
            system_ctrl_set_result(ctrlReq, SYSTEM_ERROR_BAD_DATA, nullptr, nullptr, nullptr);
            return;
        }
        const size_t replyBufSize = 256;
        if (system_ctrl_alloc_reply_data(ctrlReq, replyBufSize, nullptr) != 0) {
            system_ctrl_set_result(ctrlReq, SYSTEM_ERROR_NO_MEMORY, nullptr, nullptr, nullptr);
            return;
        }
        JSONBufferWriter writer(ctrlReq->reply_data, ctrlReq->reply_size);
        if (!processRequest(req, writer)) {
            system_ctrl_set_result(ctrlReq, SYSTEM_ERROR_UNKNOWN, nullptr, nullptr, nullptr); // FIXME
            return;
        }
        ctrlReq->reply_size = writer.dataSize();
        system_ctrl_set_result(ctrlReq, SYSTEM_ERROR_NONE, nullptr, nullptr, nullptr);
    }

private:
    struct Object {
        JSONString type;
        JSONValue params;
    };

    struct Request {
        Object handler, stream;
        LogCategoryFilters filters;
        JSONString cmd, id;
        LogLevel level;

        Request() :
                level(LOG_LEVEL_INFO) { // Default level
        }
    };

    static bool parseRequest(const JSONValue &value, Request *req) {
        JSONObjectIterator it(value);
        while (it.next()) {
            if (it.name() == "cmd") { // Command name
                req->cmd = it.value().toString();
            } else if (it.name() == "id") { // Handler ID
                req->id = it.value().toString();
            } else if (it.name() == "hnd") { // Handler settings
                if (!parseObject(it.value(), &req->handler)) {
                    return false;
                }
            } else if (it.name() == "strm") { // Stream settings
                if (!parseObject(it.value(), &req->stream)) {
                    return false;
                }
            } else if (it.name() == "filt") { // Category filters
                if (!parseFilters(it.value(), &req->filters)) {
                    return false;
                }
            } else if (it.name() == "lvl") { // Default level
                if (!parseLevel(it.value(), &req->level)) {
                    return false;
                }
            }
        }
        return true;
    }

    static bool parseObject(const JSONValue &value, Object *object) {
        JSONObjectIterator it(value);
        while (it.next()) {
            if (it.name() == "type") { // Object type
                object->type = it.value().toString();
            } else if (it.name() == "param") { // Additional parameters
                object->params = it.value();
            }
        }
        return true;
    }

    static bool parseFilters(const JSONValue &value, LogCategoryFilters *filters) {
        JSONArrayIterator it(value);
        if (!filters->reserve(it.count())) {
            return false; // Memory allocation error
        }
        while (it.next()) {
            JSONString cat;
            LogLevel level = LOG_LEVEL_INFO; // Default level
            JSONObjectIterator it2(it.value());
            if (it2.next()) {
                cat = it2.name();
                if (cat.isEmpty()) {
                    return false;
                }
                if (!parseLevel(it2.value(), &level)) {
                    return false;
                }
            }
            filters->append(LogCategoryFilter((String)cat, level));
        }
        return true;
    }

    static bool parseLevel(const JSONValue &value, LogLevel *level) {
        const JSONString name = value.toString();
        static struct {
            const char *name;
            LogLevel level;
        } levels[] = {
                { "none", LOG_LEVEL_NONE },
                { "trace", LOG_LEVEL_TRACE },
                { "info", LOG_LEVEL_INFO },
                { "warn", LOG_LEVEL_WARN },
                { "error", LOG_LEVEL_ERROR },
                { "panic", LOG_LEVEL_PANIC },
                { "all", LOG_LEVEL_ALL }
            };
        static size_t n = sizeof(levels) / sizeof(levels[0]);
        size_t i = 0;
        for (; i < n; ++i) {
            if (name == levels[i].name) {
                break;
            }
        }
        if (i == n) {
            return false; // Invalid level name
        }
        *level = levels[i].level;
        return true;
    }

    static bool processRequest(Request &req, JSONWriter &writer) {
        if (req.cmd == "addHandler") {
            return addHandler(req, writer);
        } else if (req.cmd == "removeHandler") {
            return removeHandler(req, writer);
        } else if (req.cmd == "enumHandlers") {
            return enumHandlers(req, writer);
        } else {
            return false; // Unsupported request
        }
    }

    static bool addHandler(Request &req, JSONWriter&) {
        if (req.id.isEmpty() || req.handler.type.isEmpty()) {
            return false;
        }
        const char* const strmType = !req.stream.type.isEmpty() ? (const char*)req.stream.type : nullptr;
        return logManager()->addFactoryHandler((const char*)req.id, (const char*)req.handler.type, req.level, req.filters,
                req.handler.params, strmType, req.stream.params);
    }

    static bool removeHandler(Request &req, JSONWriter&) {
        if (req.id.isEmpty()) {
            return false;
        }
        logManager()->removeFactoryHandler((const char*)req.id);
        return true;
    }

    static bool enumHandlers(Request &req, JSONWriter &writer) {
        writer.beginArray();
        logManager()->enumFactoryHandlers(enumHandlersCallback, &writer);
        writer.endArray();
        return true;
    }

    static void enumHandlersCallback(const char *id, void *data) {
        JSONWriter *writer = static_cast<JSONWriter*>(data);
        writer->value(id);
    }

    static LogManager* logManager() {
        return LogManager::instance();
    }
};

// Custom deleter for std::unique_ptr
template<typename T, typename FactoryT, void(FactoryT::*destroy)(T*)>
struct FactoryDeleter {
    FactoryDeleter() : factory(nullptr) {
    }

    FactoryDeleter(FactoryT *factory) : factory(factory) {
    }

    void operator()(T *ptr) {
        if (ptr) {
            (factory->*destroy)(ptr);
        }
    }

    FactoryT *factory;
};

typedef FactoryDeleter<LogHandler, LogHandlerFactory, &LogHandlerFactory::destroyHandler> LogHandlerFactoryDeleter;
typedef FactoryDeleter<Print, OutputStreamFactory, &OutputStreamFactory::destroyStream> OutputStreamFactoryDeleter;

#endif // Wiring_LogConfig

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

} // namespace

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
LogHandler* spark::DefaultLogHandlerFactory::createHandler(const char *type, LogLevel level, LogCategoryFilters filters,
            Print *stream, const JSONValue &params) {
    if (strcmp(type, "JSONStreamLogHandler") == 0) {
        if (!stream) {
            return nullptr; // Output stream is not specified
        }
        return new(std::nothrow) JSONStreamLogHandler(*stream, level, std::move(filters));
    } else if (strcmp(type, "StreamLogHandler") == 0) {
        if (!stream) {
            return nullptr;
        }
        return new(std::nothrow) StreamLogHandler(*stream, level, std::move(filters));
    }
    return nullptr; // Unknown handler type
}

spark::DefaultLogHandlerFactory* spark::DefaultLogHandlerFactory::instance() {
    static DefaultLogHandlerFactory factory;
    return &factory;
}

// spark::DefaultOutputStreamFactory
Print* spark::DefaultOutputStreamFactory::createStream(const char *type, const JSONValue &params) {
#if PLATFORM_ID != 3
    if (strcmp(type, "Serial") == 0) {
        Serial.begin();
        return &Serial;
    }
#if Wiring_USBSerial1
    if (strcmp(type, "USBSerial1") == 0) {
        USBSerial1.begin();
        return &USBSerial1;
    }
#endif
    if (strcmp(type, "Serial1") == 0) {
        int baud = 9600;
        getParams(params, &baud);
        Serial1.begin(baud);
        return &Serial1;
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

void spark::DefaultOutputStreamFactory::getParams(const JSONValue &params, int *baudRate) {
    JSONObjectIterator it(params);
    while (it.next()) {
        if (it.name() == "baud" && baudRate) {
            *baudRate = it.value().toInt();
        }
    }
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

bool spark::LogManager::addFactoryHandler(const char *id, const char *handlerType, LogLevel level, LogCategoryFilters filters,
        const JSONValue &handlerParams, const char *streamType, const JSONValue &streamParams) {
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandler(id); // Destroy existent handler with the same ID
        FactoryHandler h;
        h.id = id;
        if (!h.id.length()) {
            return false; // Empty handler ID or memory allocation error
        }
        // Create output stream (optional)
        std::unique_ptr<Print, OutputStreamFactoryDeleter> stream(nullptr, streamFactory_);
        if (streamType) {
            if (streamFactory_) {
                stream.reset(streamFactory_->createStream(streamType, streamParams));
            }
            if (!stream) {
                return false; // Unsupported stream type
            }
        }
        // Create log handler
        std::unique_ptr<LogHandler, LogHandlerFactoryDeleter> handler(nullptr, handlerFactory_);
        if (handlerType && handlerFactory_) {
            handler.reset(handlerFactory_->createHandler(handlerType, level, std::move(filters), stream.get(), handlerParams));
        }
        if (!handler) {
            return false; // Unsupported handler type
        }
        h.handler = handler.get();
        h.stream = stream.get();
        if (!factoryHandlers_.append(std::move(h))) {
            return false;
        }
        if (!activeHandlers_.append(h.handler)) {
            factoryHandlers_.takeLast(); // Revert factoryHandlers_.append()
            return false;
        }
        if (activeHandlers_.size() == 1) {
            setSystemCallbacks();
        }
        handler.release(); // Release scope guard pointers
        stream.release();
    }
    return true;
}

void spark::LogManager::removeFactoryHandler(const char *id) {
    LOG_WITH_LOCK(mutex_) {
        destroyFactoryHandler(id);
    }
}

void spark::LogManager::enumFactoryHandlers(void(*callback)(const char *id, void *data), void *data) {
    LOG_WITH_LOCK(mutex_) {
        for (const FactoryHandler &h: factoryHandlers_) {
            callback(h.id.c_str(), data);
        }
    }
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
void spark::logProcessControlRequest(ctrl_request* req) {
    JSONRequestHandler::process(req);
}

#endif // Wiring_LogConfig
