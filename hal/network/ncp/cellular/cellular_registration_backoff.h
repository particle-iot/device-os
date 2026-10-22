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

#include "network/ncp/connection_backoff.h"

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
class CellularRegistrationBackoff: public ConnectionBackoff {
public:
    struct Config: ConnectionBackoff::Config {
        Config() {
            immediateStages = CELLULAR_BACKOFF_FIRST_STAGES;
            activeWindow = CELLULAR_BACKOFF_ACTIVE_WINDOW;
            const system_tick_t defaultIntervals[INTERVAL_COUNT] = CELLULAR_BACKOFF_INTERVALS;
            std::copy(defaultIntervals, defaultIntervals + INTERVAL_COUNT, intervals);
            heartbeatInterval = CELLULAR_BACKOFF_LOG_INTERVAL;
        }
    };

    CellularRegistrationBackoff()
            : ConnectionBackoff(Config()) {
    }

    explicit CellularRegistrationBackoff(const ConnectionBackoff::Config& conf)
            : ConnectionBackoff(conf) {
    }
};

} // namespace particle
