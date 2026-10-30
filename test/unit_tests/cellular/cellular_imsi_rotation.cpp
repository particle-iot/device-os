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

#include "ncp/cellular/cellular_imsi_rotation.h"

#undef WARN
#undef INFO
#include "catch2/catch.hpp"

using namespace particle;

namespace {

const system_tick_t SEC = 1000;
const system_tick_t MIN = 60 * SEC;

// IMSIs seen in the field on one multi-IMSI SIM
const char HOME[] = "234108029098056";
const char ALT1[] = "232104856814585";
const char ALT2[] = "234502125320556";

// Boot, then the first IMSI check as the baseline
CellularImsiRotation booted(system_tick_t& now) {
    CellularImsiRotation r;
    r.fplmnCleared();
    r.windowStarted();
    now = 1 * MIN;
    r.imsiChecked(HOME, now);
    return r;
}

} // namespace

TEST_CASE("CellularImsiRotation boot") {
    CellularImsiRotation r;

    SECTION("clears FPLMN and does not force a rotation") {
        CHECK(r.fplmnClearDue());
        CHECK_FALSE(r.rotationDue());
        r.fplmnCleared();
        CHECK_FALSE(r.fplmnClearDue());
    }

    SECTION("the first IMSI is a baseline, not a change") {
        r.fplmnCleared();
        r.simRefreshed();
        CHECK_FALSE(r.imsiChecked(HOME, 10 * SEC));
        CHECK_FALSE(r.fplmnClearDue());
        CHECK(std::string(r.imsi()) == HOME);
    }
}

TEST_CASE("CellularImsiRotation IMSI checks") {
    system_tick_t now = 0;
    auto r = booted(now);

    SECTION("due once a minute") {
        CHECK_FALSE(r.imsiCheckDue(now + 59 * SEC));
        CHECK(r.imsiCheckDue(now + 60 * SEC));
    }

    SECTION("due straight away after a SIM refresh") {
        r.simRefreshed();
        CHECK(r.imsiCheckDue(now + 1 * SEC));
    }

    SECTION("a change found by polling does not clear FPLMN") {
        CHECK(r.imsiChecked(ALT1, now + 60 * SEC));
        CHECK_FALSE(r.fplmnClearDue());
        CHECK(std::string(r.imsi()) == ALT1);
    }

    SECTION("a change right after a SIM refresh does not clear FPLMN") {
        // The applet's REFRESH reloads it, a clear here would only cycle CFUN again
        r.simRefreshed();
        CHECK(r.imsiChecked(ALT1, now + 5 * SEC));
        CHECK_FALSE(r.fplmnClearDue());
        CHECK(std::string(r.imsi()) == ALT1);
    }

    SECTION("a SIM refresh is used up by the next check") {
        r.simRefreshed();
        CHECK_FALSE(r.imsiChecked(HOME, now + 5 * SEC));
        // Back to the minute
        CHECK_FALSE(r.imsiCheckDue(now + 6 * SEC));
    }

    SECTION("a failed read uses up the refresh and restarts the minute") {
        r.simRefreshed();
        CHECK_FALSE(r.imsiChecked(nullptr, now + 5 * SEC));
        CHECK_FALSE(r.imsiCheckDue(now + 6 * SEC));
        CHECK(r.imsiChecked(ALT1, now + 65 * SEC));
        CHECK(std::string(r.imsi()) == ALT1);
    }

    SECTION("an over-long IMSI is truncated") {
        CHECK(r.imsiChecked("1234567890123456789", now + 60 * SEC));
        CHECK(std::string(r.imsi()) == "123456789012345");
    }
}

TEST_CASE("CellularImsiRotation registration timeouts") {
    system_tick_t now = 0;
    auto r = booted(now);

    // What the client does at the start of a window: baseline, then the rotation if one is due
    auto startWindow = [&](const char* imsi, system_tick_t t) {
        r.windowStarted();
        r.imsiChecked(imsi, t);
    };

    SECTION("an unchanged IMSI forces a rotation and clears FPLMN") {
        r.registrationTimedOut();
        CHECK(r.rotationDue());
        CHECK(r.fplmnClearDue());
        r.rotationForced(true);
        r.fplmnCleared();
        CHECK_FALSE(r.rotationDue());
        CHECK_FALSE(r.fplmnClearDue());
    }

    SECTION("a backoff reset clears FPLMN without forcing a rotation") {
        r.registrationRestarted();
        CHECK(r.fplmnClearDue());
        CHECK_FALSE(r.rotationDue());
    }

    SECTION("a change during the window skips the rotation but still clears FPLMN") {
        r.imsiChecked(ALT1, now + 60 * SEC);
        r.registrationTimedOut();
        CHECK_FALSE(r.rotationDue());
        CHECK(r.fplmnClearDue());
    }

    SECTION("a change in an earlier window does not count") {
        r.imsiChecked(ALT1, now + 60 * SEC);
        r.registrationTimedOut();
        startWindow(ALT1, now + 11 * MIN);
        r.registrationTimedOut();
        CHECK(r.rotationDue());
    }

    SECTION("the SIM coming back on its home IMSI after a power cycle is not a change") {
        // The last window ended part way through a walk
        r.imsiChecked(ALT2, now + 60 * SEC);
        r.registrationTimedOut();
        r.windowStarted();
        CHECK_FALSE(r.imsiChecked(HOME, now + 11 * MIN));
        CHECK(std::string(r.imsi()) == HOME);
        r.registrationTimedOut();
        CHECK(r.rotationDue());
    }

    SECTION("at most one rotation per timeout once the applet starts one") {
        r.registrationTimedOut();
        startWindow(HOME, now + 11 * MIN);
        r.rotationForced(true);
        // Nothing during the window asks for another one
        r.simRefreshed();
        r.imsiChecked(HOME, now + 11 * MIN + 5 * SEC);
        r.imsiChecked(HOME, now + 12 * MIN);
        CHECK_FALSE(r.rotationDue());
    }

    SECTION("the walk a forced rotation starts counts as a change") {
        // Forced at the start of this window, the applet walks the IMSIs and comes back home
        r.registrationTimedOut();
        startWindow(HOME, now + 11 * MIN);
        r.rotationForced(true);
        r.imsiChecked(ALT1, now + 12 * MIN);
        r.imsiChecked(ALT2, now + 13 * MIN);
        r.imsiChecked(HOME, now + 16 * MIN);
        r.registrationTimedOut();
        CHECK_FALSE(r.rotationDue());
        // The window after that has nothing, so it is forced again
        startWindow(HOME, now + 22 * MIN);
        r.registrationTimedOut();
        CHECK(r.rotationDue());
    }
}

TEST_CASE("CellularImsiRotation applet not starting a rotation") {
    system_tick_t now = 0;
    auto r = booted(now);
    r.registrationTimedOut();
    now = 11 * MIN;
    r.windowStarted();
    r.imsiChecked(HOME, now);
    r.rotationForced(false);
    CHECK(r.rotationAttempts() == 1);

    SECTION("not resent before the next IMSI check") {
        CHECK_FALSE(r.rotationDue());
        CHECK_FALSE(r.imsiCheckDue(now + 59 * SEC));
    }

    SECTION("resent on the next check that finds the IMSI unchanged") {
        CHECK(r.imsiCheckDue(now + 60 * SEC));
        r.imsiChecked(HOME, now + 60 * SEC);
        CHECK(r.rotationDue());
    }

    SECTION("not resent if the IMSI moved anyway") {
        r.imsiChecked(ALT1, now + 60 * SEC);
        CHECK_FALSE(r.rotationDue());
    }

    SECTION("not resent on a failed read, the next good one decides") {
        r.imsiChecked(nullptr, now + 60 * SEC);
        CHECK_FALSE(r.rotationDue());
        r.imsiChecked(HOME, now + 120 * SEC);
        CHECK(r.rotationDue());
    }

    SECTION("stops once one starts") {
        r.imsiChecked(HOME, now + 60 * SEC);
        r.rotationForced(true);
        r.imsiChecked(HOME, now + 120 * SEC);
        CHECK_FALSE(r.rotationDue());
    }

    SECTION("gives up after the last attempt of the window") {
        for (unsigned i = 1; i < CELLULAR_IMSI_ROTATION_ATTEMPTS; i++) {
            r.imsiChecked(HOME, now + i * MIN);
            REQUIRE(r.rotationDue());
            r.rotationForced(false);
        }
        CHECK(r.rotationAttempts() == CELLULAR_IMSI_ROTATION_ATTEMPTS);
        r.imsiChecked(HOME, now + 10 * MIN);
        CHECK_FALSE(r.rotationDue());
    }

    SECTION("abandoned, no applet or not in automatic mode, nothing more this window") {
        r.imsiChecked(HOME, now + 60 * SEC);
        REQUIRE(r.rotationDue());
        r.rotationAbandoned();
        CHECK_FALSE(r.rotationDue());
        r.imsiChecked(HOME, now + 120 * SEC);
        CHECK_FALSE(r.rotationDue());
        // But the next timeout tries again, the mode may have changed by then
        r.registrationTimedOut();
        CHECK(r.rotationDue());
    }

    SECTION("the next window starts counting again") {
        r.registrationTimedOut();
        r.windowStarted();
        CHECK(r.rotationAttempts() == 0);
        CHECK(r.rotationDue());
    }
}
