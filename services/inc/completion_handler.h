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
#include "timer_hal.h"

#include <limits>

extern "C" {
#endif // defined(__cplusplus)

// Callback invoked when an asynchronous operation completes. 'error' is set to SYSTEM_ERROR_NONE
// if operation has completed successfully
typedef void (*completion_callback)(int error, void* result, void* data, void* reserved);

#ifdef __cplusplus
} // extern "C"

namespace particle {

// C++ wrapper for completion callback. Instances of this class can only be moved and not copied
// (similarly to std::unique_ptr). If no result or error is passed to a CompletionHandler instance
// during its lifetime, underlying completion callback will be invoked with SYSTEM_ERROR_INTERNAL error
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
        // It's an error if a completion handler wasn't invoked during its lifetime
        setError(SYSTEM_ERROR_INTERNAL);
    }

    void setResult(void* data = nullptr) {
        if (callback_) {
            callback_(SYSTEM_ERROR_NONE, data, data_, nullptr);
            callback_ = nullptr;
        }
    }

    void setError(system_error error) {
        if (callback_) {
            callback_(error, nullptr, data_, nullptr);
            callback_ = nullptr;
        }
    }

    void operator()(void* data = nullptr) {
        setResult(data);
    }

    operator bool() const {
        return callback_;
    }

    CompletionHandler& operator=(CompletionHandler&& handler) {
        setError(SYSTEM_ERROR_INTERNAL); // Invoke current callback
        callback_ = handler.callback_;
        data_ = handler.data_;
        handler.callback_ = nullptr; // Reset source handler
        return *this;
    }

private:
    completion_callback callback_;
    void* data_;
};

// Container class storing CompletionHandler instances arranged by key. processTimeouts() method
// needs to be called periodically in order to invoke expired handlers
template<typename KeyT, unsigned defaultTimeoutMillis = 30000>
class CompletionHandlerMap {
public:
    CompletionHandlerMap() :
            nearestTimeout_(std::numeric_limits<system_tick_t>::max()) {
    }

    bool add(const KeyT& key, CompletionHandler&& handler, unsigned timeout = defaultTimeoutMillis) {
        if (handler) {
            // FIXME: Handle timer overflow
            const system_tick_t t = HAL_Timer_Get_Milli_Seconds() + timeout; // Absolute expiration time
            if (!handlers_.append(Handler(key, std::move(handler), t))) {
                return false;
            }
            if (t < nearestTimeout_) {
                nearestTimeout_ = t;
            }
        }
        return true;
    }

    CompletionHandler remove(const KeyT& key) {
        return takeHandler(key);
    }

    void setResult(const KeyT& key, void* data = nullptr) {
        takeHandler(key).setResult(data);
    }

    void setError(const KeyT& key, system_error error) {
        takeHandler(key).setError(error);
    }

    void processTimeouts() {
        const system_tick_t now = HAL_Timer_Get_Milli_Seconds();
        if (now >= nearestTimeout_) {
            nearestTimeout_ = std::numeric_limits<system_tick_t>::max();
            int i = 0;
            while (i < handlers_.size()) {
                const system_tick_t t = handlers_.at(i).time;
                if (t >= now) {
                    Handler h = handlers_.takeAt(i);
                    h.handler.setError(SYSTEM_ERROR_TIMEOUT);
                } else {
                    if (t < nearestTimeout_) {
                        nearestTimeout_ = t;
                    }
                    ++i;
                }
            }
        }
    }

    void clear() {
        handlers_.clear();
        nearestTimeout_ = std::numeric_limits<system_tick_t>::max();
    }

private:
    struct Handler {
        KeyT key;
        CompletionHandler handler;
        system_tick_t time; // Absolute expiration time

        Handler(KeyT key, CompletionHandler handler, system_tick_t time) :
                key(std::move(key)),
                handler(std::move(handler)),
                time(time) {
        }
    };

    CompletionHandler takeHandler(const KeyT& key) {
        int index = -1;
        system_tick_t t = std::numeric_limits<system_tick_t>::max();
        for (int i = 0; i < handlers_.size(); ++i) {
            const Handler& h = handlers_.at(i);
            if (h.key == key) {
                index = i;
            } else if (h.time < t) {
                t = h.time;
            }
        }
        if (index < 0) {
            return CompletionHandler();
        }
        nearestTimeout_ = t;
        return handlers_.takeAt(index).handler;
    }

    spark::Vector<Handler> handlers_;
    system_tick_t nearestTimeout_;
};

} // namespace particle

#endif // defined(__cplusplus)

#endif // SERVICES_COMPLETION_HANDLER_H
