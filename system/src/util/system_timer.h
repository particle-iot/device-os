/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "system_threading.h"

#include "concurrent_hal.h"

namespace particle::system {

/**
 * A one-shot timer executed in the system thread.
 */
class SystemTimer: private ISRTaskQueue::Task {
public:
    typedef void (*Callback)(void* arg);

    /**
     * Construct a timer.
     *
     * @param callback Callback to invoke when the timer expires.
     * @param arg Argument to pass to the callback.
     */
    explicit SystemTimer(Callback callback, void* arg = nullptr) :
            Task(taskCallback),
            timer_(),
            callback_(callback),
            arg_(arg) {
    }

    /**
     * Destruct the timer.
     */
    ~SystemTimer();

    /**
     * Start the timer.
     *
     * @param timeout Timeout in milliseconds.
     * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
     */
    int start(unsigned timeout);

    /**
     * Stop the timer.
     */
    void stop();

private:
    os_timer_t timer_;
    Callback callback_;
    void* arg_;

    static void taskCallback(ISRTaskQueue::Task* task);
    static void timerCallback(os_timer_t timer);
};

} // namespace particle::system
