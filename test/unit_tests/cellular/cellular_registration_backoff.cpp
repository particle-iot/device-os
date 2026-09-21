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

#include "ncp/cellular/cellular_registration_backoff.h"

#undef WARN
#undef INFO
#include "catch2/catch.hpp"

using namespace particle;

namespace {

const system_tick_t MIN = 60 * 1000;
const system_tick_t HR = 60 * MIN;

const system_tick_t WINDOW = 10 * MIN;

// Spelled out rather than taken from the defaults, so a header edit cannot rewrite a case
CellularRegistrationBackoff::Config productionConfig() {
    CellularRegistrationBackoff::Config c;
    c.firstHourStages = 6;
    c.activeWindow = WINDOW;
    c.intervals[0] = 1 * HR;
    c.intervals[1] = 2 * HR;
    c.intervals[2] = 4 * HR;
    c.heartbeatInterval = 10 * MIN;
    return c;
}

// Runs one active window that fails, returns when the window closed
system_tick_t failWindow(CellularRegistrationBackoff& b, system_tick_t now) {
    now += WINDOW;
    b.attemptFailed(now);
    return now;
}

// Same, stepping by a caller supplied window
// attemptFailed() swallows anything sooner than one window, so a fixed step drops most calls
system_tick_t failWindowsOf(CellularRegistrationBackoff& b, system_tick_t now, unsigned count,
        system_tick_t window) {
    for (unsigned i = 0; i < count; i++) {
        now += window;
        b.attemptFailed(now);
    }
    return now;
}

// Runs the six windows of the first hour, all failing
system_tick_t failFirstHour(CellularRegistrationBackoff& b, system_tick_t now) {
    for (unsigned i = 0; i < 6; i++) {
        now = failWindow(b, now);
        REQUIRE_FALSE(b.inCooldown(now));
    }
    return now;
}

} // namespace

TEST_CASE("Registration backoff starts at stage 1 with no cooldown") {
    CellularRegistrationBackoff b(productionConfig());
    b.reset(0);
    REQUIRE(b.stage() == 1);
    REQUIRE_FALSE(b.inCooldown(0));
    REQUIRE_FALSE(b.cooldownExpired(0));
    REQUIRE(b.cooldownRemaining(0) == 0);
}

TEST_CASE("The first hour is unchanged") {
    // The central scope decision of SC-140817, asserted directly
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    for (unsigned i = 0; i < 6; i++) {
        REQUIRE(b.stage() == i + 1);
        t = failWindow(b, t);
        REQUIRE_FALSE(b.inCooldown(t));
        REQUIRE(b.cooldownRemaining(t) == 0);
    }
    REQUIRE(t == 60 * MIN);
    REQUIRE(b.stage() == 7);
    REQUIRE_FALSE(b.inCooldown(t));
}

TEST_CASE("Past the first hour the backoff steps 1, 2, 4 hours") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    t = failFirstHour(b, t);

    SECTION("stage 7 cools down for the hour minus its window") {
        t = failWindow(b, t); // closes at 70 min
        REQUIRE(t == 70 * MIN);
        REQUIRE(b.stage() == 8);
        REQUIRE(b.inCooldown(t));
        REQUIRE(b.cooldownRemaining(t) == 50 * MIN);
        REQUIRE_FALSE(b.cooldownExpired(t));

        SECTION("and the next window opens exactly one hour after the last one did") {
            REQUIRE(b.inCooldown(t + 50 * MIN - 1));
            REQUIRE(b.cooldownExpired(t + 50 * MIN));
            REQUIRE(b.cooldownRemaining(t + 50 * MIN) == 0);
            // Window 7 opened at 60 min, window 8 opens at 120 min
            REQUIRE(t + 50 * MIN == 120 * MIN);
        }

        SECTION("stage 8 cools down for two hours minus its window") {
            t += 50 * MIN;
            t = failWindow(b, t); // closes at 130 min
            REQUIRE(b.stage() == 9);
            REQUIRE(b.inCooldown(t));
            REQUIRE(b.cooldownRemaining(t) == 110 * MIN);
            // Window 9 opens at 240 min
            REQUIRE(t + 110 * MIN == 240 * MIN);

            SECTION("stage 9 cools down for four hours minus its window, and repeats forever") {
                t += 110 * MIN;
                for (unsigned i = 0; i < 12; i++) {
                    const unsigned expected = 9 + i;
                    REQUIRE(b.stage() == expected);
                    t = failWindow(b, t);
                    REQUIRE(b.inCooldown(t));
                    REQUIRE(b.cooldownRemaining(t) == 230 * MIN);
                    t += 230 * MIN;
                }
                // Windows opened at 240, 480, 720 ... every four hours
                REQUIRE(t == 240 * MIN + 12 * 4 * HR);
            }
        }
    }
}

TEST_CASE("Worst case recovery for a reactivated SIM is inside four hours") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    t = failFirstHour(b, t);
    t = failWindow(b, t);
    t += b.cooldownRemaining(t);
    t = failWindow(b, t);
    t += b.cooldownRemaining(t);
    t = failWindow(b, t); // steady state now
    // A SIM reactivated a minute after this window closed waits out the rest of the cooldown
    REQUIRE(b.cooldownRemaining(t + 1 * MIN) == 229 * MIN);
    REQUIRE(b.cooldownRemaining(t + 1 * MIN) < 4 * HR);
}

TEST_CASE("reset() puts the backoff back to stage 1") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    t = failFirstHour(b, t);
    t = failWindow(b, t);
    REQUIRE(b.inCooldown(t));

    SECTION("including from inside a cooldown") {
        b.reset(t);
        REQUIRE(b.stage() == 1);
        REQUIRE_FALSE(b.inCooldown(t));
        REQUIRE_FALSE(b.cooldownExpired(t));
        REQUIRE(b.cooldownRemaining(t) == 0);
    }

    SECTION("and the first hour is free again afterwards") {
        b.reset(t);
        failFirstHour(b, t);
    }
}

TEST_CASE("breakCooldown() leaves the cooldown without touching the stage") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    t = failFirstHour(b, t);
    t = failWindow(b, t);
    REQUIRE(b.inCooldown(t));
    REQUIRE(b.stage() == 8);

    b.breakCooldown(t);
    REQUIRE_FALSE(b.inCooldown(t));
    REQUIRE_FALSE(b.cooldownExpired(t));
    REQUIRE(b.cooldownRemaining(t) == 0);
    REQUIRE(b.stage() == 8);

    // The window it runs next is stage 8's, so failing it cools down for two hours
    t = failWindow(b, t);
    REQUIRE(b.cooldownRemaining(t) == 110 * MIN);
    REQUIRE(b.stage() == 9);
}

TEST_CASE("A second attemptFailed() inside one window is ignored") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);

    SECTION("during the first hour") {
        t = failWindow(b, t);
        REQUIRE(b.stage() == 2);
        b.attemptFailed(t);
        b.attemptFailed(t + 1);
        b.attemptFailed(t + WINDOW - 1);
        REQUIRE(b.stage() == 2);
        // A failure a full window later is a real one
        b.attemptFailed(t + WINDOW);
        REQUIRE(b.stage() == 3);
    }

    SECTION("and during a cooldown, where it would otherwise skip a stage") {
        t = failFirstHour(b, t);
        t = failWindow(b, t);
        REQUIRE(b.stage() == 8);
        REQUIRE(b.cooldownRemaining(t) == 50 * MIN);
        b.attemptFailed(t + 1 * MIN);
        REQUIRE(b.stage() == 8);
        REQUIRE(b.cooldownRemaining(t + 1 * MIN) == 49 * MIN);
    }
}

TEST_CASE("restoreStage() resumes a stage read out of retained memory") {
    CellularRegistrationBackoff b(productionConfig());

    SECTION("a restored stage runs an active window, not a cooldown") {
        b.restoreStage(9);
        REQUIRE(b.stage() == 9);
        REQUIRE_FALSE(b.inCooldown(0));
        REQUIRE_FALSE(b.cooldownExpired(0));
        REQUIRE(b.cooldownRemaining(0) == 0);
    }

    SECTION("and the next failure advances from there, not from stage 1") {
        b.restoreStage(9);
        const system_tick_t t = failWindow(b, 0);
        REQUIRE(b.stage() == 10);
        REQUIRE(b.inCooldown(t));
        REQUIRE(b.cooldownRemaining(t) == 230 * MIN);
    }

    SECTION("restoring stage 7 gives the one hour interval back") {
        b.restoreStage(7);
        const system_tick_t t = failWindow(b, 0);
        REQUIRE(b.cooldownRemaining(t) == 50 * MIN);
    }

    SECTION("restoring a first hour stage keeps the first hour free") {
        b.restoreStage(3);
        system_tick_t t = 0;
        for (unsigned i = 3; i <= 6; i++) {
            REQUIRE(b.stage() == i);
            t = failWindow(b, t);
            REQUIRE_FALSE(b.inCooldown(t));
        }
        REQUIRE(b.stage() == 7);
    }

    SECTION("out of range values are clamped rather than rejected") {
        b.restoreStage(0);
        REQUIRE(b.stage() == 1);
        b.restoreStage(1000);
        REQUIRE(b.stage() == b.maxStage());
    }

    SECTION("a cooldown in progress does not survive a restore") {
        system_tick_t t = 0;
        b.reset(t);
        t = failFirstHour(b, t);
        t = failWindow(b, t);
        REQUIRE(b.inCooldown(t));
        b.restoreStage(8);
        REQUIRE_FALSE(b.inCooldown(t));
    }
}

TEST_CASE("retainedStage() clamps to the highest stage worth remembering") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    REQUIRE(b.retainedStage() == 1);

    t = failFirstHour(b, t);
    REQUIRE(b.stage() == 7);
    REQUIRE(b.retainedStage() == 7);

    // Everything past maxStage() behaves identically, so it stores as maxStage()
    for (unsigned i = 0; i < 10; i++) {
        t = failWindow(b, t);
        t += b.cooldownRemaining(t);
    }
    REQUIRE(b.stage() > b.maxStage());
    REQUIRE(b.retainedStage() == b.maxStage());

    // And a round trip through retained memory lands on the same behaviour
    CellularRegistrationBackoff resumed(productionConfig());
    resumed.restoreStage(b.retainedStage());
    const system_tick_t t2 = failWindow(resumed, 0);
    REQUIRE(resumed.cooldownRemaining(t2) == 230 * MIN);
}

TEST_CASE("The schedule survives a system tick rollover") {
    // system_tick_t is 32 bits and wraps every 49.7 days, so every comparison is now - stamp
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0xFFFFFFFF - 5 * MIN;
    b.reset(t);

    t = failFirstHour(b, t); // wraps during the first hour
    REQUIRE(b.stage() == 7);

    t = failWindow(b, t);
    REQUIRE(b.inCooldown(t));
    REQUIRE(b.cooldownRemaining(t) == 50 * MIN);
    REQUIRE(b.inCooldown(t + 50 * MIN - 1));
    REQUIRE(b.cooldownExpired(t + 50 * MIN));

    SECTION("including a cooldown that spans the wrap") {
        // Start early enough that the cooldown itself begins 10 minutes before 0xFFFFFFFF
        CellularRegistrationBackoff c(productionConfig());
        system_tick_t u = 0xFFFFFFFF - 80 * MIN;
        c.reset(u);
        u = failFirstHour(c, u);
        u = failWindow(c, u);
        REQUIRE(c.inCooldown(u));
        REQUIRE(c.cooldownRemaining(u) == 50 * MIN);
        REQUIRE(c.inCooldown(u + 10 * MIN)); // exactly at the wrap
        REQUIRE(c.cooldownRemaining(u + 10 * MIN) == 40 * MIN);
        REQUIRE(c.inCooldown(u + 49 * MIN)); // past it
        REQUIRE(c.cooldownExpired(u + 50 * MIN));
    }
}

TEST_CASE("The intervention budget is uncapped in v1") {
    // Hours of silence follow each window, so even an uncapped burst stays inside carrier limits
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    REQUIRE(b.interventionBudget() == UINT_MAX);
    t = failFirstHour(b, t);
    t = failWindow(b, t);
    REQUIRE(b.interventionBudget() == UINT_MAX);
}

TEST_CASE("The schedule is configurable so the integration tests can run in minutes") {
    CellularRegistrationBackoff::Config conf;
    conf.firstHourStages = 2;
    conf.activeWindow = 10 * 1000;
    conf.intervals[0] = 60 * 1000;
    conf.intervals[1] = 120 * 1000;
    conf.intervals[2] = 240 * 1000;

    CellularRegistrationBackoff b(conf);
    system_tick_t t = 0;
    b.reset(t);
    for (unsigned i = 0; i < 2; i++) {
        t += conf.activeWindow;
        b.attemptFailed(t);
        REQUIRE_FALSE(b.inCooldown(t));
    }
    t += conf.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.inCooldown(t));
    REQUIRE(b.cooldownRemaining(t) == 50 * 1000);
    REQUIRE(CellularRegistrationBackoff::maxStage(conf) == 5);
}

TEST_CASE("cooldownLength() reports the whole cooldown, not what is left of it") {
    // What the entry log line uses
    // cooldownRemaining() has already lost the AT round trip in and truncates a second short
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = 0;
    b.reset(t);
    REQUIRE(b.cooldownLength() == 0); // nothing to report outside a cooldown

    t = failFirstHour(b, t);
    t = failWindow(b, t);
    REQUIRE(b.cooldownLength() == 50 * MIN);
    REQUIRE(b.cooldownLength() / 1000 == 50 * 60);

    // Unchanged as the cooldown runs down, which is the whole point
    REQUIRE(b.cooldownRemaining(t + 300) == 50 * MIN - 300);
    REQUIRE(b.cooldownLength() == 50 * MIN);
    REQUIRE(b.cooldownRemaining(t + 300) / 1000 == 50 * 60 - 1); // the truncation being avoided

    b.breakCooldown(t);
    REQUIRE(b.cooldownLength() == 0);
}

TEST_CASE("The compiled in schedule is self consistent") {
    // The one case that reads the compiled in macros rather than productionConfig()
    CellularRegistrationBackoff b;
    const auto& c = b.config();
    REQUIRE(c.firstHourStages == (unsigned)CELLULAR_BACKOFF_FIRST_STAGES);
    REQUIRE(c.activeWindow == (system_tick_t)CELLULAR_BACKOFF_ACTIVE_WINDOW);
    for (unsigned i = 0; i < CellularRegistrationBackoff::INTERVAL_COUNT; i++) {
        REQUIRE(c.intervals[i] > c.activeWindow);
    }
    REQUIRE(c.intervals[1] > c.intervals[0]);
    REQUIRE(c.intervals[2] > c.intervals[1]);

    // And it actually runs: the first hour stages are free, the next one cools down
    system_tick_t t = 0;
    b.reset(t);
    for (unsigned i = 0; i < c.firstHourStages; i++) {
        t += c.activeWindow;
        b.attemptFailed(t);
        REQUIRE_FALSE(b.inCooldown(t));
    }
    t += c.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.inCooldown(t));
    REQUIRE(b.cooldownRemaining(t) == c.intervals[0] - c.activeWindow);
}

TEST_CASE("An interval shorter than its own active window clamps to no cooldown") {
    CellularRegistrationBackoff::Config conf;
    conf.firstHourStages = 1;
    conf.activeWindow = 60 * 1000;
    conf.intervals[0] = 30 * 1000; // misconfigured, would wrap to ~49 days unclamped
    conf.intervals[1] = 30 * 1000;
    conf.intervals[2] = 30 * 1000;

    CellularRegistrationBackoff b(conf);
    system_tick_t t = 0;
    b.reset(t);
    t += conf.activeWindow;
    b.attemptFailed(t);
    REQUIRE_FALSE(b.inCooldown(t));

    t += conf.activeWindow;
    b.attemptFailed(t);
    REQUIRE_FALSE(b.inCooldown(t));
    REQUIRE(b.cooldownExpired(t)); // zero length, so the next window opens at once
    REQUIRE(b.cooldownRemaining(t) == 0);
}

TEST_CASE("The active window tracks the client's registration timeout") {
    CellularRegistrationBackoff b(productionConfig());

    SECTION("by default the cooldown is the interval less one ten minute window") {
        system_tick_t t = failFirstHour(b, 0);
        t = failWindow(b, t);
        REQUIRE(b.cooldownLength() == 1 * HR - 10 * MIN);
    }

    SECTION("a longer registration timeout shortens the cooldown to match") {
        // Before setActiveWindow() existed this kept a stale 10 minute copy and ran 20 long
        b.setActiveWindow(30 * MIN);
        const system_tick_t t = failWindowsOf(b, 0, 7, 30 * MIN);
        REQUIRE(b.stage() == 8);
        REQUIRE(b.cooldownLength() == 1 * HR - 30 * MIN);
        REQUIRE(b.inCooldown(t));
    }

    SECTION("a window at least as long as its interval leaves no cooldown at all") {
        b.setActiveWindow(1 * HR);
        const system_tick_t t = failWindowsOf(b, 0, 7, 1 * HR);
        REQUIRE(b.stage() == 8);
        REQUIRE(b.cooldownLength() == 0);
        REQUIRE_FALSE(b.inCooldown(t));
    }

    SECTION("a cooldown already running keeps the length it was given") {
        system_tick_t t = failFirstHour(b, 0);
        t = failWindow(b, t);
        const system_tick_t was = b.cooldownRemaining(t);
        b.setActiveWindow(30 * MIN);
        REQUIRE(b.cooldownRemaining(t) == was);
    }
}

TEST_CASE("setSchedule() replaces the whole schedule for an integration test") {
    CellularRegistrationBackoff b(productionConfig());

    CellularRegistrationBackoff::Config fast;
    fast.firstHourStages = 1;
    fast.activeWindow = 2 * MIN;
    fast.intervals[0] = 3 * MIN;
    fast.intervals[1] = 5 * MIN;
    fast.intervals[2] = 9 * MIN;
    fast.heartbeatInterval = 30 * 1000;
    b.setSchedule(fast);

    system_tick_t t = 0;

    // One free stage, matching firstHourStages
    t += fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE_FALSE(b.inCooldown(t));
    REQUIRE(b.stage() == 2);

    // Stage is asserted alongside length because the rig asserts it too
    // A cooldown is entered by stage n failing but reports n + 1, so the first here is 3
    t += fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.cooldownLength() == 3 * MIN - 2 * MIN);
    REQUIRE(b.stage() == 3);

    t += b.cooldownRemaining(t) + fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.cooldownLength() == 5 * MIN - 2 * MIN);
    REQUIRE(b.stage() == 4);

    t += b.cooldownRemaining(t) + fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.cooldownLength() == 9 * MIN - 2 * MIN);
    REQUIRE(b.stage() == 5);

    // And the last interval still repeats forever
    t += b.cooldownRemaining(t) + fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.cooldownLength() == 9 * MIN - 2 * MIN);
    REQUIRE(b.stage() == 6);

    // maxStage() follows the new schedule rather than the old one
    REQUIRE(b.maxStage() == 1 + CellularRegistrationBackoff::INTERVAL_COUNT);
}

TEST_CASE("The cooldown heartbeat says the device is alive without touching the radio") {
    CellularRegistrationBackoff b(productionConfig());

    SECTION("nothing to say outside a cooldown") {
        REQUIRE_FALSE(b.heartbeatDue(0));
        const system_tick_t t = failWindow(b, 0);
        REQUIRE_FALSE(b.inCooldown(t));
        REQUIRE_FALSE(b.heartbeatDue(t));
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * HR));
    }

    SECTION("due one whole interval in, not before") {
        system_tick_t t = failFirstHour(b, 0);
        t = failWindow(b, t);
        REQUIRE_FALSE(b.heartbeatDue(t));
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * MIN - 1));
        REQUIRE(b.heartbeatDue(t + 10 * MIN));
    }

    SECTION("heartbeatSent() rearms it for another interval") {
        system_tick_t t = failFirstHour(b, 0);
        t = failWindow(b, t);
        t += 10 * MIN;
        REQUIRE(b.heartbeatDue(t));
        b.heartbeatSent(t);
        REQUIRE_FALSE(b.heartbeatDue(t));
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * MIN - 1));
        REQUIRE(b.heartbeatDue(t + 10 * MIN));
    }

    SECTION("a four hour cooldown is not silent, and stops talking when it expires") {
        system_tick_t t = failFirstHour(b, 0);
        for (unsigned i = 0; i < 3; i++) {
            t = failWindow(b, t);
            t += b.cooldownRemaining(t);
        }
        t = failWindow(b, t);
        REQUIRE(b.cooldownLength() == 4 * HR - 10 * MIN);

        // Walk the whole cooldown a minute at a time and count what a terminal would have seen
        unsigned beats = 0;
        const system_tick_t end = t + b.cooldownLength();
        for (system_tick_t now = t; now <= end; now += 1 * MIN) {
            if (b.heartbeatDue(now)) {
                b.heartbeatSent(now);
                beats++;
            }
        }
        // 230 minutes of cooldown, one every 10, and none at the boundary where it has expired
        REQUIRE(beats == 22);
        REQUIRE_FALSE(b.heartbeatDue(end));
    }

    SECTION("and it survives a tick rollover mid cooldown") {
        const system_tick_t nearWrap = 0u - (2 * HR);
        b.restoreStage(9);
        system_tick_t t = nearWrap;
        b.attemptFailed(t);
        REQUIRE(b.cooldownLength() == 4 * HR - 10 * MIN);

        unsigned beats = 0;
        for (unsigned i = 1; i <= 230; i++) {
            const system_tick_t now = nearWrap + i * MIN; // wraps partway through
            if (b.heartbeatDue(now)) {
                b.heartbeatSent(now);
                beats++;
            }
        }
        REQUIRE(beats == 22);
    }
}

TEST_CASE("Breaking a cooldown also stops the heartbeat") {
    // The forced connect path, we must not still log a pending next attempt
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = failFirstHour(b, 0);
    t = failWindow(b, t);
    REQUIRE(b.heartbeatDue(t + 10 * MIN));

    SECTION("via reset()") {
        b.reset(t);
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * MIN));
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * HR));
    }

    SECTION("via breakCooldown()") {
        b.breakCooldown(t);
        REQUIRE_FALSE(b.heartbeatDue(t + 10 * MIN));
    }
}

TEST_CASE("Changing the schedule does not disturb a cooldown already running") {
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = failFirstHour(b, 0);
    t = failWindow(b, t);
    const system_tick_t was = b.cooldownRemaining(t);
    REQUIRE(was == 50 * MIN);

    CellularRegistrationBackoff::Config fast;
    fast.firstHourStages = 1;
    fast.activeWindow = 1 * MIN;
    fast.intervals[0] = 2 * MIN;
    fast.intervals[1] = 3 * MIN;
    fast.intervals[2] = 4 * MIN;
    fast.heartbeatInterval = 15 * 1000;
    b.setSchedule(fast);

    // Its length was fixed when the attempt failed, only the next one picks up the new schedule
    REQUIRE(b.cooldownRemaining(t) == was);

    t += b.cooldownRemaining(t);
    t += fast.activeWindow;
    b.attemptFailed(t);
    REQUIRE(b.cooldownLength() == 4 * MIN - 1 * MIN); // past the new firstHourStages, so the cap
}

TEST_CASE("restoreStage() clamps against the schedule in force, not the one that stored it") {
    CellularRegistrationBackoff b(productionConfig());
    REQUIRE(b.maxStage() == 9);

    CellularRegistrationBackoff::Config fast;
    fast.firstHourStages = 1;
    fast.activeWindow = 1 * MIN;
    fast.intervals[0] = 2 * MIN;
    fast.intervals[1] = 3 * MIN;
    fast.intervals[2] = 4 * MIN;
    b.setSchedule(fast);
    REQUIRE(b.maxStage() == 4);

    // A stage retained under one schedule must not resume where another has no interval for it
    b.restoreStage(9);
    REQUIRE(b.stage() == 4);

    const system_tick_t t = failWindowsOf(b, 0, 1, fast.activeWindow);
    REQUIRE(b.cooldownLength() == 4 * MIN - 1 * MIN);
    REQUIRE(b.inCooldown(t));
}

TEST_CASE("cooldownRemaining() counts down to the millisecond the window reopens") {
    // What the heartbeat divides down to seconds
    CellularRegistrationBackoff b(productionConfig());
    system_tick_t t = failFirstHour(b, 0);
    t = failWindow(b, t);
    const system_tick_t len = b.cooldownLength();

    REQUIRE(b.cooldownRemaining(t) == len);
    REQUIRE(b.cooldownRemaining(t + 1) == len - 1);
    REQUIRE(b.cooldownRemaining(t + len - 1) == 1);
    // A stale non-zero here would claim a pending attempt while the modem is already powering up
    REQUIRE(b.cooldownRemaining(t + len) == 0);
    REQUIRE_FALSE(b.inCooldown(t + len));
    REQUIRE(b.cooldownExpired(t + len));
}
