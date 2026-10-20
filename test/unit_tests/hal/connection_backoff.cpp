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

#include "network/ncp/connection_backoff.h"

#include "catch2/catch.hpp"

using namespace particle;

namespace {

const system_tick_t SEC = 1000;

ConnectionBackoff::Config attemptConfig(unsigned jitterPercent) {
    ConnectionBackoff::Config c;
    c.intervals[0] = 10 * SEC;
    c.intervals[1] = 60 * SEC;
    c.intervals[2] = 300 * SEC;
    c.jitterPercent = jitterPercent;
    return c;
}

} // anonymous

TEST_CASE("ConnectionBackoff without an active window") {
    SECTION("is not in a cooldown before the first failure") {
        ConnectionBackoff b(attemptConfig(0));
        CHECK_FALSE(b.inCooldown(0));
        CHECK(b.cooldownLength() == 0);
    }

    SECTION("every failure starts a cooldown, the last interval repeats") {
        ConnectionBackoff b(attemptConfig(0));
        system_tick_t now = 0;
        const system_tick_t expected[] = { 10 * SEC, 60 * SEC, 300 * SEC, 300 * SEC, 300 * SEC };
        for (auto len: expected) {
            b.attemptFailed(now);
            CHECK(b.cooldownLength() == len);
            CHECK(b.inCooldown(now + len - 1));
            CHECK_FALSE(b.inCooldown(now + len));
            now += len;
        }
    }

    SECTION("failures in quick succession are all counted") {
        ConnectionBackoff b(attemptConfig(0));
        b.attemptFailed(1000);
        b.attemptFailed(1001);
        CHECK(b.stage() == 3);
        CHECK(b.cooldownLength() == 60 * SEC);
    }

    SECTION("reset ends the cooldown and restarts the schedule") {
        ConnectionBackoff b(attemptConfig(0));
        b.attemptFailed(0);
        b.attemptFailed(10 * SEC);
        b.reset();
        CHECK_FALSE(b.inCooldown(10 * SEC));
        b.attemptFailed(10 * SEC);
        CHECK(b.cooldownLength() == 10 * SEC);
    }

    SECTION("handles tick counter wraparound") {
        ConnectionBackoff b(attemptConfig(0));
        const system_tick_t now = 0xffffffff - 5 * SEC;
        b.attemptFailed(now);
        CHECK(b.inCooldown(now + 10 * SEC - 1));
        CHECK_FALSE(b.inCooldown(now + 10 * SEC));
    }
}

TEST_CASE("ConnectionBackoff jitter") {
    SECTION("stays within the configured range") {
        ConnectionBackoff low(attemptConfig(20));
        low.attemptFailed(0, 0);
        CHECK(low.cooldownLength() == 8 * SEC);
        ConnectionBackoff high(attemptConfig(20));
        high.attemptFailed(0, 4000);
        CHECK(high.cooldownLength() == 12 * SEC);
        ConnectionBackoff wrapped(attemptConfig(20));
        wrapped.attemptFailed(0, 4001);
        CHECK(wrapped.cooldownLength() == 8 * SEC);
    }

    SECTION("does not change the underlying schedule") {
        ConnectionBackoff b(attemptConfig(20));
        b.attemptFailed(0, 0);
        b.attemptFailed(0, 0);
        CHECK(b.cooldownLength() == 48 * SEC);
        b.attemptFailed(0, 0);
        CHECK(b.cooldownLength() == 240 * SEC);
    }

    SECTION("is not applied by default") {
        ConnectionBackoff b(attemptConfig(0));
        b.attemptFailed(0, 0xffffffff);
        CHECK(b.cooldownLength() == 10 * SEC);
    }

    SECTION("clamps a jittered cooldown to the tick range") {
        ConnectionBackoff::Config c;
        c.intervals[0] = 0xf0000000;
        c.jitterPercent = 20;
        ConnectionBackoff b(c);
        b.attemptFailed(0, 0x60000000);
        CHECK(b.cooldownLength() == 0xffffffff);
    }
}
