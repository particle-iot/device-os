/*
 ******************************************************************************
 *  Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#pragma once

#include <climits>

#include "system_tick_hal.h"

// Backoff schedule
// Accelerated testing goes through setSchedule() at runtime
#define CELLULAR_BACKOFF_ACTIVE_WINDOW (10 * 60 * 1000)
#define CELLULAR_BACKOFF_FIRST_STAGES  (6)
#define CELLULAR_BACKOFF_INTERVALS     { 1 * 60 * 60 * 1000, 2 * 60 * 60 * 1000, 4 * 60 * 60 * 1000 }
#define CELLULAR_BACKOFF_LOG_INTERVAL  (10 * 60 * 1000)

namespace particle {

/**
 * Registration backoff for a SIM that cannot register. Spaces retries out to 1, 2 then every 4
 * hours once the first hour has gone by. Based on AT&T's TRENDI guidance.
 *
 * The first hour is deliberately unchanged, so a device that recovers inside it is unaffected.
 *
 * millis() is passed in so this can be accelerated in tests.
 */
class CellularRegistrationBackoff {
public:
    static const unsigned INTERVAL_COUNT = 3;

    struct Config {
        unsigned firstHourStages = CELLULAR_BACKOFF_FIRST_STAGES;
        // The registration timeout. Cooldown is the interval minus activeWindow.
        system_tick_t activeWindow = CELLULAR_BACKOFF_ACTIVE_WINDOW;
        // Start of one active window to the start of the next. The last one repeats forever.
        system_tick_t intervals[INTERVAL_COUNT] = CELLULAR_BACKOFF_INTERVALS;
        system_tick_t heartbeatInterval = CELLULAR_BACKOFF_LOG_INTERVAL;
    };

    static unsigned maxStage(const Config& conf) {
        return conf.firstHourStages + INTERVAL_COUNT;
    }

    unsigned maxStage() const {
        return maxStage(conf_);
    }

    CellularRegistrationBackoff() = default;

    explicit CellularRegistrationBackoff(const Config& conf)
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

    void reset(system_tick_t now) {
        (void)now;
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
    void attemptFailed(system_tick_t now) {
        if (failed_ && now - lastFailure_ < conf_.activeWindow) {
            return;
        }
        const unsigned failedStage = failures_ + 1;
        failed_ = true;
        lastFailure_ = now;
        if (failures_ < UINT_MAX) {
            failures_++;
        }
        if (failedStage <= conf_.firstHourStages) {
            return; // The first hour, unchanged
        }
        unsigned i = failedStage - conf_.firstHourStages - 1;
        if (i >= INTERVAL_COUNT) {
            i = INTERVAL_COUNT - 1; // The last interval repeats forever
        }
        cooldownStart_ = now;
        lastHeartbeat_ = now;
        // An interval shorter than its window retries immediately
        const system_tick_t interval = conf_.intervals[i];
        cooldownLength_ = interval > conf_.activeWindow ? interval - conf_.activeWindow : 0;
        cooling_ = true;
    }

    bool inCooldown(system_tick_t now) const {
        return cooling_ && now - cooldownStart_ < cooldownLength_;
    }

    bool cooldownExpired(system_tick_t now) const {
        return cooling_ && now - cooldownStart_ >= cooldownLength_;
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

    void breakCooldown(system_tick_t now) {
        (void)now;
        cooling_ = false;
    }

    // 1 based. The active window we are in, or the one the current cooldown is waiting to run.
    unsigned stage() const {
        return failures_ + 1;
    }

    unsigned retainedStage() const {
        const unsigned s = stage();
        return s > maxStage() ? maxStage() : s;
    }

    // How many registration interventions this stage is allowed. Currently uncapped.
    unsigned interventionBudget() const {
        return UINT_MAX;
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
