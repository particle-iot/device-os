/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "concurrent_hal.h"

#include <cstdint>

namespace particle {

class Runnable;

enum class ThreadRunnerEventType: uint8_t {
    STARTED = 1,
    STOPPED = 2,
    ERROR = 3
};

struct ThreadRunnerEvent {
    ThreadRunnerEventType type;
};

struct ThreadRunnerErrorEvent: ThreadRunnerEvent {
    int error;
};

typedef void(*ThreadRunnerEventHandler)(const ThreadRunnerEvent& event);

class ThreadRunnerOptions {
public:
    static const char* const DEFAULT_THREAD_NAME = "ThreadRunner";
    static const size_t DEFAULT_STACK_SIZE = 2048;

    ThreadRunnerOptions();

    ThreadRunnerOptions& eventHandler(ThreadRunnerEventHandler handler);
    ThreadRunnerEventHandler eventHandler() const;

    ThreadRunnerOptions& threadName(const char* name);
    const char* threadName() const;

    ThreadRunnerOptions& stackSize(size_t size);
    size_t stackSize() const;

    ThreadRunnerOptions& priority(os_thread_prio_t prio);
    os_thread_prio_t priority() const;

private:
    ThreadRunnerEventHandler handler_;
    const char* name_;
    os_thread_prio_t prio_;
    size_t stackSize_;
};

class ThreadRunner {
public:
    ThreadRunner();
    ~ThreadRunner();

    // Note: The runner doesn't take ownership over the runnable object
    int init(Runnable* run, const ThreadRunnerOptions& opts = ThreadRunnerOptions());
    void destroy();

private:
    Runnable* run_;
    EventHandler* eventHandler_;
    os_thread_t thread_;
    volatile bool stop_;

    static void run(void* data);
};

inline ThreadRunnerOptions::ThreadRunnerOptions() :
        handler_(nullptr),
        name_(DEFAULT_THREAD_NAME),
        prio_(OS_THREAD_PRIORITY_DEFAULT),
        stackSize_(DEFAULT_STACK_SIZE) {
}

inline ThreadRunnerOptions& ThreadRunnerOptions::eventHandler(ThreadRunnerEventHandler handler) {
    handler_ = handler;
    return *this;
}

inline ThreadRunnerEventHandler ThreadRunnerOptions::eventHandler() const {
    return handler_
}

inline ThreadRunnerOptions& ThreadRunnerOptions::threadName(const char* name) {
    name_ = name;
    return *this;
}

inline const char* ThreadRunnerOptions::threadName() const {
    return name_;
}

inline ThreadRunnerOptions& ThreadRunnerOptions::stackSize(size_t size) {
    stackSize_ = size;
    return *this;
}

inline size_t ThreadRunnerOptions::stackSize() const {
    return stackSize_;
}

inline ThreadRunnerOptions& ThreadRunnerOptions::priority(os_thread_prio_t prio) {
    prio_ = prio;
    return *this;
}

inline os_thread_prio_t ThreadRunnerOptions::priority() const {
    return prio_;
}

inline ThreadRunner::ThreadRunner() :
        run_(nullptr),
        eventHandler_(nullptr),
        thread_(OS_THREAD_INVALID_HANDLE),
        stop_(false) {
}

inline ThreadRunner::~ThreadRunner() {
    destroy();
}

} // particle
