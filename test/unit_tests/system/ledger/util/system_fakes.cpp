/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

// Deterministic replacements for the clock, RTC and system timer used by the ledger code

#include "util/system_fakes.h"
#include "util/system_timer.h"

#include "timer_hal.h"
#include "delay_hal.h"
#include "rtc_hal.h"
#include "system_cloud.h"

#include <map>
#include <stdexcept>

namespace {

const int64_t EPOCH_TIME_SEC = 1700000000; // Unix time reported by the fake RTC at startup

struct TimerEntry {
    particle::system::SystemTimer::Callback callback;
    void* arg;
    uint64_t dueTime;
};

uint64_t g_now = 0;

// Timers are keyed by their address. Only started timers are stored
std::map<const void*, TimerEntry>& timers() {
    static auto t = new std::map<const void*, TimerEntry>(); // Leaked, see CoapFake::instance()
    return *t;
}

// Runs the earliest timer that is due at the specified time. Returns false if there are no such timers
bool runNextTimer(uint64_t time) {
    auto& t = timers();
    auto next = t.end();
    for (auto it = t.begin(); it != t.end(); ++it) {
        if (it->second.dueTime <= time && (next == t.end() || it->second.dueTime < next->second.dueTime)) {
            next = it;
        }
    }
    if (next == t.end()) {
        return false;
    }
    auto e = next->second;
    t.erase(next); // Timers are one-shot
    if (e.dueTime > g_now) {
        g_now = e.dueTime;
    }
    e.callback(e.arg);
    return true;
}

} // namespace

namespace particle::test {

uint64_t currentTime() {
    return g_now;
}

void runTimers() {
    // Guard against a timer that keeps restarting itself with a zero timeout
    for (int i = 0; i < 1000; ++i) {
        if (!runNextTimer(g_now)) {
            return;
        }
    }
    throw std::runtime_error("Timers keep running");
}

void advanceTime(uint64_t ms) {
    auto endTime = g_now + ms;
    for (int i = 0; i < 1000; ++i) {
        if (!runNextTimer(endTime)) {
            g_now = endTime;
            return;
        }
    }
    throw std::runtime_error("Timers keep running");
}

} // namespace particle::test

namespace particle::system {

SystemTimer::~SystemTimer() {
    stop();
}

int SystemTimer::start(unsigned timeout) {
    timers()[this] = { callback_, arg_, g_now + timeout };
    return 0;
}

void SystemTimer::stop() {
    timers().erase(this);
}

void SystemTimer::taskCallback(ISRTaskQueue::Task* task) {
}

void SystemTimer::timerCallback(os_timer_t timer) {
}

} // namespace particle::system

uint64_t hal_timer_millis(void* reserved) {
    return g_now;
}

system_tick_t HAL_Timer_Get_Milli_Seconds() {
    return g_now;
}

void HAL_Delay_Milliseconds(uint32_t millis) {
    // The tests are single-threaded so there's nothing to wait for
}

bool hal_rtc_time_is_valid(hal_rtc_option_t* opt) {
    return true;
}

int hal_rtc_get_time(struct timeval* tv, hal_rtc_option_t* opt) {
    auto ms = EPOCH_TIME_SEC * 1000 + (int64_t)g_now;
    tv->tv_sec = ms / 1000;
    tv->tv_usec = (ms % 1000) * 1000;
    return 0;
}

// Not using test/unit_tests/stub/system_cloud.cpp as it depends on a large part of Wiring
bool spark_cloud_flag_connected() {
    return false; // LedgerManager::init() requires the device to be disconnected
}
