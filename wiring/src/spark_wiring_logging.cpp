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
#include <cinttypes>
#include <cstdio>

#include "spark_wiring_logging.h"

namespace {

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
// category name specified at module level, so here we have to use "app" category name explicitly
const spark::Logger spark::Log("app");

// spark::LogHandler
struct spark::LogHandler::FilterData {
    const char *name; // Subcategory name
    size_t size; // Name length
    int level; // Logging level (-1 if not specified for this node)
    std::vector<FilterData> filters; // Children nodes

    FilterData(const char *name, size_t size) :
            name(name),
            size(size),
            level(-1) {
    }
};

/*
    This method builds prefix tree based on the list of filter strings. Every node of the tree
    contains subcategory name and, optionally, logging level - if node matches complete filter
    string. For example, given following filters:

    a (error)
    a.b.c (trace)
    a.b.x (trace)
    aa (error)
    aa.b (warn)

    The code builds following prefix tree:

    |
    |- a (error) -- b - c (trace)
    |               |
    |               `-- x (trace)
    |
    `- aa (error) - b (warn)
*/
spark::LogHandler::LogHandler(LogLevel level, const Filters &filters) :
        level_(level) {
    for (auto it = filters.begin(); it != filters.end(); ++it) {
        const char* const category = it->first;
        const LogLevel level = it->second;
        std::vector<FilterData> *filters = &filters_; // Root nodes
        size_t pos = 0;
        for (size_t i = 0;; ++i) {
            if (category[i] && category[i] != '.') { // Category name separator
                continue;
            }
            const size_t size = i - pos;
            if (!size) {
                break; // Invalid category name
            }
            const char* const name = category + pos;
            // Use binary search to find existent node or position for new node
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
                it = filters->insert(it, FilterData(name, size)); // Add node
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

spark::LogHandler::~LogHandler() {
}

LogLevel spark::LogHandler::categoryLevel(const char *category) const {
    if (!category || filters_.empty()) {
        return level_; // Default level
    }
    LogLevel level = level_;
    const std::vector<FilterData> *filters = &filters_; // Root nodes
    size_t pos = 0;
    for (size_t i = 0;; ++i) {
        if (category[i] && category[i] != '.') { // Category name separator
            continue;
        }
        const size_t size = i - pos;
        if (!size) {
            break; // Invalid category name
        }
        const char* const name = category + pos;
        // Use binary search to find node for current subcategory
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

// spark::StreamLogHandler
void spark::StreamLogHandler::logMessage(const char *msg, LogLevel level, const char *category, const LogAttributes &attr) {
    char buf[16];
    // Timestamp
    if (attr.has_time) {
        snprintf(buf, sizeof(buf), "%010u ", (unsigned)attr.time);
        write(buf);
    }
    // Category
    if (category) {
        write("[");
        write(category);
        write("] ");
    }
    // Source file
    if (attr.has_file) {
        // Strip directory path
        const char *s = extractFileName(attr.file);
        write(s); // File name
        if (attr.has_line) {
            write(":");
            snprintf(buf, sizeof(buf), "%d", attr.line); // Line number
            write(buf);
        }
        if (attr.has_function) {
            write(", ");
        } else {
            write(": ");
        }
    }
    // Function name
    if (attr.has_function) {
        // Strip argument and return types for better readability
        size_t n = 0;
        const char *s = extractFuncName(attr.function, &n);
        write(s, n);
        write("(): ");
    }
    // Level
    write(levelName(level));
    write(": ");
    // Message
    if (msg) {
        write(msg);
    }
    // Additional attributes
    if (attr.has_code || attr.has_details) {
        write(" [");
        if (attr.has_code) {
            write("code");
            write(" = ");
            snprintf(buf, sizeof(buf), "%" PRIiPTR, attr.code);
            write(buf);
        }
        if (attr.has_details) {
            if (attr.has_code) {
                write(", ");
            }
            write("details");
            write(" = ");
            write(attr.details);
        }
        write("]");
    }
    write("\r\n");
}

// spark::LogManager
void spark::LogManager::addHandler(LogHandler *handler) {
    const auto it = std::find(handlers_.cbegin(), handlers_.cend(), handler);
    if (it == handlers_.end()) {
        handlers_.push_back(handler);
        if (handlers_.size() == 1) {
            log_set_callbacks(logMessage, logWrite, logEnabled, nullptr); // Set system callbacks
        }
    }
}

void spark::LogManager::removeHandler(LogHandler *handler) {
    const auto it = std::find(handlers_.begin(), handlers_.end(), handler);
    if (it != handlers_.end()) {
        if (handlers_.size() == 1) {
            log_set_callbacks(nullptr, nullptr, nullptr, nullptr); // Reset system callbacks
        }
        handlers_.erase(it);
    }
}

spark::LogManager* spark::LogManager::instance() {
    static LogManager mgr;
    return &mgr;
}

void spark::LogManager::logMessage(const char *msg, int level, const char *category, const LogAttributes *attr, void *reserved) {
    const auto &handlers = instance()->handlers_;
    for (size_t i = 0; i < handlers.size(); ++i) {
        handlers[i]->message(msg, (LogLevel)level, category, *attr);
    }
}

void spark::LogManager::logWrite(const char *data, size_t size, int level, const char *category, void *reserved) {
    const auto &handlers = instance()->handlers_;
    for (size_t i = 0; i < handlers.size(); ++i) {
        handlers[i]->write(data, size, (LogLevel)level, category);
    }
}

int spark::LogManager::logEnabled(int level, const char *category, void *reserved) {
    int minLevel = LOG_LEVEL_NONE;
    const auto &handlers = instance()->handlers_;
    for (size_t i = 0; i < handlers.size(); ++i) {
        const int level = handlers[i]->categoryLevel(category);
        if (level < minLevel) {
            minLevel = level;
        }
    }
    return (level >= minLevel);
}
