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

#ifndef SERVICES_COMPLETION_HANDLER_H
#define SERVICES_COMPLETION_HANDLER_H

#include "system_error.h"

#ifdef __cplusplus
#include "spark_wiring_vector.h"
#include "system_tick_hal.h"

#include <limits>

extern "C" {
#endif // defined(__cplusplus)

// Callback invoked when an asynchronous operation completes. For a successfully completed operation
// `error` argument should be set to SYSTEM_ERROR_NONE
typedef void (*completion_callback)(int error, const void* data, void* callback_data, void* reserved);

#ifdef __cplusplus
} // extern "C"

namespace particle {

// C++ wrapper for a completion callback. Instances of this class can only be moved and not copied
// (similarly to std::unique_ptr). If no result or error is passed to a CompletionHandler instance
// during its lifetime, underlying completion callback will be invoked with SYSTEM_ERROR_INTERNAL
// error by CompletionHandler's destructor
class CompletionHandler {
public:
    explicit CompletionHandler(completion_callback callback = nullptr, void* data = nullptr) :
            callback_(callback),
            data_(data) {
    }

    CompletionHandler(CompletionHandler&& handler) :
            callback_(handler.callback_),
            data_(handler.data_) {
        handler.callback_ = nullptr; // Reset source handler
    }

    ~CompletionHandler() {
        // It's an internal error if a completion handler wasn't invoked during its lifetime
        setError(SYSTEM_ERROR_INTERNAL);
    }

    template<typename T>
    void setResult(const T& result) {
        setResult((const void*)&result);
    }

    void setResult() {
        setResult(nullptr);
    }

    // TODO: Message formatting
    void setError(int error, const char* msg = nullptr) {
        if (callback_) {
            callback_(error, msg, data_, nullptr);
            callback_ = nullptr;
        }
    }

    CompletionHandler& operator=(CompletionHandler&& handler) {
        setError(SYSTEM_ERROR_INTERNAL); // Invoke current callback
        callback_ = handler.callback_;
        data_ = handler.data_;
        handler.callback_ = nullptr; // Reset source handler
        return *this;
    }

    void operator()(void* data = nullptr) {
        setResult(data);
    }

    operator bool() const {
        return callback_;
    }

private:
    completion_callback callback_;
    void* data_;

    void setResult(const void* result) {
        if (callback_) {
            callback_(SYSTEM_ERROR_NONE, result, data_, nullptr);
            callback_ = nullptr;
        }
    }
};

// Container class storing a list of CompletionHandler instances. This class manages handler timeouts,
// see update() method for details
class CompletionHandlerList {
public:
    static const system_tick_t MAX_TIMEOUT = std::numeric_limits<system_tick_t>::max();

    explicit CompletionHandlerList(system_tick_t defaultTimeout = 60000) :
            defaultTimeout_(defaultTimeout),
            timeoutTicks_(MAX_TIMEOUT),
            ticks_(0) {
    }

    bool addHandler(CompletionHandler&& handler, system_tick_t timeout) {
        if (handler) {
            const system_tick_t t = ticks_ + timeout; // Handler expiration time
            if (handlers_.append(Handler(std::move(handler), t))) {
                if (t < timeoutTicks_) {
                    timeoutTicks_ = t; // Update nearest expiration time
                }
                return true;
            }
        }
        return false;
    }

    bool addHandler(CompletionHandler&& handler) {
        return addHandler(std::move(handler), defaultTimeout_);
    }

    void clear() {
        setError(SYSTEM_ERROR_ABORTED);
    }

    int size() const {
        return handlers_.size();
    }

    bool isEmpty() const {
        return handlers_.isEmpty();
    }

    template<typename T>
    void setResult(const T& result) {
        for (Handler& h: handlers_) {
            h.handler.setResult(result);
        }
        reset();
    }

    void setResult() {
        for (Handler& h: handlers_) {
            h.handler.setResult();
        }
        reset();
    }

    void setError(int error, const char* msg = nullptr) {
        for (Handler& h: handlers_) {
            h.handler.setError(error, msg);
        }
        reset();
    }

    // This method needs to be called periodically in order to invoke expired handlers.
    // `ticks` argument specifies a number of milliseconds passed since previous update
    int update(system_tick_t ticks) {
        if (!handlers_.isEmpty()) {
            ticks_ += ticks;
            if (ticks_ >= timeoutTicks_) {
                timeoutTicks_ = MAX_TIMEOUT;
                int count = 0; // Number of expired handlers
                int i = 0;
                do {
                    Handler& h = handlers_.at(i);
                    if (ticks_ >= h.ticks) {
                        // Remove expired handler
                        CompletionHandler handler = handlers_.takeAt(i).handler;
                        handler.setError(SYSTEM_ERROR_TIMEOUT);
                        ++count;
                    } else {
                        // Update handler expiration time
                        h.ticks -= ticks_;
                        if (h.ticks < timeoutTicks_) {
                            timeoutTicks_ = h.ticks;
                        }
                        ++i;
                    }
                } while (i < handlers_.size());
                ticks_ = 0;
                return count;
            }
        }
        return 0;
    }

    system_tick_t nearestTimeout() const {
        return timeoutTicks_ - ticks_;
    }

    int size() {
        return handlers_.size();
    }

private:
    struct Handler {
        CompletionHandler handler;
        system_tick_t ticks; // Expiration time

        Handler(CompletionHandler handler, system_tick_t ticks) :
                handler(std::move(handler)),
                ticks(ticks) {
        }
    };

    const system_tick_t defaultTimeout_;

    spark::Vector<Handler> handlers_;
    system_tick_t timeoutTicks_; // Nearest handler expiration time
    system_tick_t ticks_;

    void reset() {
        handlers_.clear();
        timeoutTicks_ = MAX_TIMEOUT;
        ticks_ = 0;
    }
};

// Container class storing CompletionHandler instances arranged by key. This class manages handler
// timeouts, see update() method for details
template<typename KeyT>
class CompletionHandlerMap {
public:
    static const system_tick_t MAX_TIMEOUT = std::numeric_limits<system_tick_t>::max();

    explicit CompletionHandlerMap(system_tick_t defaultTimeout = 60000) :
            defaultTimeout_(defaultTimeout),
            timeoutTicks_(MAX_TIMEOUT),
            ticks_(0) {
    }

    bool addHandler(const KeyT& key, CompletionHandler&& handler, system_tick_t timeout) {
        if (handler) {
            const system_tick_t t = ticks_ + timeout; // Handler expiration time
            if (handlers_.append(Handler(key, std::move(handler), t))) {
                if (t < timeoutTicks_) {
                    timeoutTicks_ = t; // Update nearest expiration time
                }
                return true;
            }
        }
        return false;
    }

    bool addHandler(const KeyT& key, CompletionHandler&& handler) {
        return addHandler(key, std::move(handler), defaultTimeout_);
    }

    CompletionHandler takeHandler(const KeyT& key) {
        CompletionHandler handler;
        timeoutTicks_ = MAX_TIMEOUT;
        int i = 0;
        while (i < handlers_.size()) {
            const Handler& h = handlers_.at(i);
            if (h.key == key) {
                handler = handlers_.takeAt(i).handler;
                if (handlers_.isEmpty()) {
                    ticks_ = 0;
                }
            } else {
                if (h.ticks < timeoutTicks_) {
                    timeoutTicks_ = h.ticks;
                }
                ++i;
            }
        }
        return handler;
    }

    bool hasHandler(const KeyT& key) const {
        for (const Handler& h: handlers_) {
            if (h.key == key) {
                return true;
            }
        }
        return false;
    }

    void clear() {
        for (Handler& h: handlers_) {
            h.handler.setError(SYSTEM_ERROR_ABORTED);
        }
        handlers_.clear();
        timeoutTicks_ = MAX_TIMEOUT;
        ticks_ = 0;
    }

    int size() const {
        return handlers_.size();
    }

    bool isEmpty() const {
        return handlers_.isEmpty();
    }

    template<typename T>
    void setResult(const KeyT& key, const T& result) {
        takeHandler(key).setResult(result);
    }

    void setResult(const KeyT& key) {
        takeHandler(key).setResult();
    }

    void setError(const KeyT& key, int error, const char* msg = nullptr) {
        takeHandler(key).setError(error, msg);
    }

    // This method needs to be called periodically in order to invoke expired handlers.
    // `ticks` argument specifies a number of milliseconds passed since previous update
    int update(system_tick_t ticks) {
        if (!handlers_.isEmpty()) {
            ticks_ += ticks;
            if (ticks_ >= timeoutTicks_) {
                timeoutTicks_ = MAX_TIMEOUT;
                int count = 0; // Number of expired handlers
                int i = 0;
                do {
                    Handler& h = handlers_.at(i);
                    if (ticks_ >= h.ticks) {
                        // Remove expired handler
                        CompletionHandler handler = handlers_.takeAt(i).handler;
                        handler.setError(SYSTEM_ERROR_TIMEOUT);
                        ++count;
                    } else {
                        // Update handler expiration time
                        h.ticks -= ticks_;
                        if (h.ticks < timeoutTicks_) {
                            timeoutTicks_ = h.ticks;
                        }
                        ++i;
                    }
                } while (i < handlers_.size());
                ticks_ = 0;
                return count;
            }
        }
        return 0;
    }

    system_tick_t nearestTimeout() const {
        return timeoutTicks_ - ticks_;
    }

private:
    struct Handler {
        KeyT key;
        CompletionHandler handler;
        system_tick_t ticks; // Expiration time

        Handler(KeyT key, CompletionHandler handler, system_tick_t ticks) :
                key(std::move(key)),
                handler(std::move(handler)),
                ticks(ticks) {
        }
    };

    const system_tick_t defaultTimeout_;

    spark::Vector<Handler> handlers_;
    system_tick_t timeoutTicks_; // Nearest handler expiration time
    system_tick_t ticks_;
};

template<typename KeyT>
const system_tick_t CompletionHandlerMap<KeyT>::MAX_TIMEOUT;

} // namespace particle

#endif // defined(__cplusplus)

#endif // SERVICES_COMPLETION_HANDLER_H
