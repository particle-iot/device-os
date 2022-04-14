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

#include "thread_runner.h"

#include "runnable.h"

#include "system_error.h"
#include "debug.h"

namespace particle {

const char* const ThreadRunnerOptions::THREAD_NAME = "ThreadRunner";

int ThreadRunner::init(Runnable* run, const ThreadRunnerOptions& opts) {
    run_ = run;
    eventHandler_ = opts.eventHandler();
    stop_ = false;
    if (os_thread_create(&thread_, opts.threadName(), opts.priority(), ThreadRunner::run, this, opts.stackSize()) != 0) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return 0;
}

void ThreadRunner::destroy() {
    if (thread_ == OS_THREAD_INVALID_HANDLE) {
        return;
    }
    stop_ = true;
    SPARK_ASSERT(os_thread_join(thread_) == 0);
    SPARK_ASSERT(os_thread_cleanup(thread_) == 0);
    thread_ = OS_THREAD_INVALID_HANDLE;
}

void ThreadRunner::run(void* data) {
    const auto r = static_cast<ThreadRunner*>(data);
    if (r->eventHandler_) {
        ThreadRunnerEvent ev = {};
        ev.type = ThreadRunnerEventType::STARTED;
        r->eventHandler_(ev);
    }
    for (;;) {
        if (r->stop_) {
            break;
        }
        const int ret = r->run_->run();
        if (ret < 0) {
            LOG_DEBUG(ERROR, "Runnable failed: %d", ret);
            if (r->eventHandler_) {
                ThreadRunnerErrorEvent ev = {};
                ev.type = ThreadRunnerEventType::ERROR;
                ev.error = ret;
                r->eventHandler_(ev);
            }
            break;
        }
    }
    if (r->eventHandler_) {
        ThreadRunnerEvent ev = {};
        ev.type = ThreadRunnerEventType::STOPPED;
        r->eventHandler_(ev);
    }
    SPARK_ASSERT(os_thread_exit(nullptr) == 0);
}

} // particle
