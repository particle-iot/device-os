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

#include <chrono>
#include <thread>
#include <mutex>
#include <cassert>

#include <boost/asio.hpp>

#include "concurrent_hal.h"

#include "system_error.h"
#include "logging.h"

namespace {

class IoService {
public:
    void stop() {
        work_.reset(); // Break the event loop
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    boost::asio::io_context& context() {
        return ctx_;
    }

    static IoService* instance() {
        static IoService s;
        return &s;
    }

private:
    boost::asio::io_context ctx_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    std::thread thread_;

    IoService() :
            work_(ctx_.get_executor()),
            thread_([this]() { this->ctx_.run(); }) {
    }

    ~IoService() {
        stop();
    }
};

class Timer {
public:
    typedef void (*Callback)(os_timer_t timer);

    Timer(unsigned period, Callback callback, bool oneShot = true, void* id = nullptr) :
            d_(std::make_shared<Data>(this, period, callback, oneShot, id)) {
        // Requirement for the period to be greater than 0 mimics the behavior of FreeRTOS' xTimerCreate():
        // https://www.freertos.org/FreeRTOS-timers-xTimerCreate.html
        if (!period || !callback) {
            throw std::runtime_error("Invalid argument");
        }
    }

    Timer(const Timer&) = delete;

    ~Timer() {
        stop();
    }

    void start() {
        std::lock_guard lock(d_->mutex);
        cancel();
        startTimer(d_);
        d_->started = true;
    }

    void stop() {
        std::lock_guard lock(d_->mutex);
        cancel();
    }

    void setPeriod(unsigned period) {
        // Requirement for the period to be greater than 0 mimics the behavior of FreeRTOS' xTimerChangePeriod():
        // https://www.freertos.org/FreeRTOS-timers-xTimerChangePeriod.html
        if (!period) {
            throw std::runtime_error("Invalid argument");
        }
        std::lock_guard lock(d_->mutex);
        cancel();
        d_->period = period;
        // xTimerChangePeriod() also starts the timer
        startTimer(d_);
    }

    void setId(void* id) {
        std::lock_guard lock(d_->mutex);
        d_->timerId = id;
    }

    void* id() const {
        std::lock_guard lock(d_->mutex);
        return d_->timerId;
    }

    bool isStarted() {
        std::lock_guard lock(d_->mutex);
        return d_->started;
    }

    Timer& operator=(const Timer&) = delete;

private:
    struct Data {
        boost::asio::steady_timer timer;
        std::recursive_mutex mutex;
        Callback callback;
        os_timer_t halInstance;
        void* timerId;
        unsigned period;
        int stateId;
        bool oneShot;
        bool started;

        Data(os_timer_t halInstance, unsigned period, Callback callback, bool oneShot, void* id) :
                timer(IoService::instance()->context()),
                callback(callback),
                halInstance(halInstance),
                timerId(id),
                period(period),
                stateId(0),
                oneShot(oneShot),
                started(false) {
        }
    };

    std::shared_ptr<Data> d_;

    void cancel() {
        d_->timer.cancel();
        d_->started = false;
        // "Invalidate" any references to the timer's internal state
        d_->stateId += 1;
    }

    static void startTimer(std::shared_ptr<Data> d) {
        auto t = std::chrono::steady_clock::now() + std::chrono::milliseconds(d->period);
        d->timer.expires_at(t);
        d->timer.async_wait([d, stateId = d->stateId](const boost::system::error_code& err) {
            std::lock_guard lock(d->mutex);
            if (err || d->stateId != stateId) {
                return;
            }
            if (d->oneShot) {
                d->started = false;
            }
            assert(d->callback);
            d->callback(d->halInstance);
            if (d->stateId != stateId) {
                return;
            }
            if (!d->oneShot) {
                startTimer(d);
            }
        });
    }
};

} // namespace

int os_timer_create(os_timer_t* timer, unsigned period, void (*callback)(os_timer_t timer), void* timer_id, bool one_shot,
        void* /* reserved */) {
    try {
        assert(timer);
        *timer = new Timer(period, callback, one_shot, timer_id);
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "os_timer_create() failed: %s", e.what());
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int os_timer_destroy(os_timer_t timer, void* /* reserved */) {
    auto t = static_cast<Timer*>(timer);
    delete t;
    return 0;
}

int os_timer_change(os_timer_t timer, os_timer_change_t change, bool /* fromISR */, unsigned period, unsigned /* block */,
        void* /* reserved */) {
    try {
        auto t = static_cast<Timer*>(timer);
        assert(t);
        switch (change) {
        // xTimerStart() and xTimerReset() seem to behave exactly the same :thinking_face:
        // https://www.freertos.org/FreeRTOS-timers-xTimerStart.html
        // https://www.freertos.org/FreeRTOS-timers-xTimerReset.html
        case OS_TIMER_CHANGE_START:
        case OS_TIMER_CHANGE_RESET: {
            t->start();
            break;
        }
        case OS_TIMER_CHANGE_STOP: {
            t->stop();
            break;
        }
        case OS_TIMER_CHANGE_PERIOD: {
            t->setPeriod(period);
            break;
        }
        default:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "os_timer_change() failed: %s", e.what());
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int os_timer_set_id(os_timer_t timer, void* timer_id) {
    try {
        auto t = static_cast<Timer*>(timer);
        assert(t);
        t->setId(timer_id);
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "os_timer_set_id() failed: %s", e.what());
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int os_timer_get_id(os_timer_t timer, void** timer_id) {
    try {
        auto t = static_cast<Timer*>(timer);
        assert(t && timer_id);
        *timer_id = t->id();
        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "os_timer_get_id() failed: %s", e.what());
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int os_timer_is_active(os_timer_t timer, void* /* reserved */) {
    try {
        auto t = static_cast<Timer*>(timer);
        assert(t);
        return t->isStarted();
    } catch (const std::exception& e) {
        LOG(ERROR, "os_timer_is_active() failed: %s", e.what());
        return SYSTEM_ERROR_UNKNOWN;
    }
}
