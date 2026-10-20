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

#pragma once

#include <algorithm>
#include <climits>
#include <cstdint>
#include <limits>

#include "system_tick_hal.h"

namespace particle {

class ConnectionBackoff {
public:
    static const unsigned INTERVAL_COUNT = 3;

    struct Config {
        unsigned immediateStages = 0;
        // The registration timeout. Cooldown is the interval minus activeWindow.
        system_tick_t activeWindow = 0;
        // Start of one active window to the start of the next. The last one repeats forever.
        system_tick_t intervals[INTERVAL_COUNT] = {};
        system_tick_t heartbeatInterval = 0;
        unsigned jitterPercent = 0;
    };

    static unsigned maxStage(const Config& conf) {
        return conf.immediateStages + INTERVAL_COUNT;
    }

    unsigned maxStage() const {
        return maxStage(conf_);
    }

    explicit ConnectionBackoff(const Config& conf)
            : conf_(conf) {
    }

    // Keep the active window in sync with the ncpclient's registration timeout
    void setActiveWindow(system_tick_t window) {
        conf_.activeWindow = window;
    }

    // Replace the whole schedule, for integration tests that need minutes rather than hours.
    // Reachable from an application only through PARTICLE_USE_UNSTABLE_API.
    void setSchedule(const Config& conf) {
        conf_ = conf;
    }

    void reset() {
        failures_ = 0;
        failed_ = false;
        cooling_ = false;
    }

    // Resume a stage read out of retained memory, at init only
    void restoreStage(unsigned stage) {
        if (stage < 1) {
            stage = 1;
        } else if (stage > maxStage()) {
            stage = maxStage();
        }
        failures_ = stage - 1;
        failed_ = false;
        cooling_ = false;
    }

    // Called when an active window ended without registering
    void attemptFailed(system_tick_t now, uint32_t random = 0) {
        if (failed_ && now - lastFailure_ < conf_.activeWindow) {
            return;
        }
        const unsigned failedStage = failures_ + 1;
        failed_ = true;
        lastFailure_ = now;
        if (failures_ < UINT_MAX) {
            failures_++;
        }
        if (failedStage <= conf_.immediateStages) {
            return; // The first hour, unchanged
        }
        unsigned i = failedStage - conf_.immediateStages - 1;
        if (i >= INTERVAL_COUNT) {
            i = INTERVAL_COUNT - 1; // The last interval repeats forever
        }
        cooldownStart_ = now;
        lastHeartbeat_ = now;
        // An interval shorter than its window retries immediately
        const system_tick_t interval = conf_.intervals[i];
        const uint64_t cooldown = interval > conf_.activeWindow ? interval - conf_.activeWindow : 0;
        const uint64_t jitter = cooldown * std::min(conf_.jitterPercent, 100u) / 100;
        cooldownLength_ = (system_tick_t)std::min<uint64_t>(cooldown - jitter + random % (2 * jitter + 1),
                std::numeric_limits<system_tick_t>::max());
        cooling_ = true;
    }

    bool inCooldown(system_tick_t now) const {
        return cooling_ && now - cooldownStart_ < cooldownLength_;
    }

    system_tick_t cooldownLength() const {
        return cooling_ ? cooldownLength_ : 0;
    }

    system_tick_t cooldownRemaining(system_tick_t now) const {
        if (!inCooldown(now)) {
            return 0;
        }
        return cooldownLength_ - (now - cooldownStart_);
    }

    // Used for logging we are in cooldown
    bool heartbeatDue(system_tick_t now) const {
        return inCooldown(now) && now - lastHeartbeat_ >= conf_.heartbeatInterval;
    }

    void heartbeatSent(system_tick_t now) {
        lastHeartbeat_ = now;
    }

    // 1 based. The active window we are in, or the one the current cooldown is waiting to run.
    unsigned stage() const {
        return failures_ + 1;
    }

    unsigned retainedStage() const {
        const unsigned s = stage();
        return s > maxStage() ? maxStage() : s;
    }

    const Config& config() const {
        return conf_;
    }

private:
    Config conf_;
    unsigned failures_ = 0;
    bool failed_ = false;
    bool cooling_ = false;
    system_tick_t lastFailure_ = 0;
    system_tick_t cooldownStart_ = 0;
    system_tick_t cooldownLength_ = 0;
    system_tick_t lastHeartbeat_ = 0;
};

} // namespace particle
