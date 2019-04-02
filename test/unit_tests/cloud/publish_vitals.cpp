/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include <limits>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <fakeit.hpp>

#include "system_publish_vitals.h"
#include "../test/unit_tests/mock_types.h"

// ASSUMPTION!!! - Period "getter" method works correctly - without being testing.

TEST_CASE("Initialized values", "[VitalsPublisher::VitalsPublisher]")
{
    particle::mock_type::Timer t;

    SECTION("When constructed, then period is initialized to MAX value.")
    {
        // Arrange

        // Act
        particle::system::VitalsPublisher<particle::mock_type::Timer> vp(nullptr, &t);

        // Assert
        CHECK(std::numeric_limits<system_tick_t>::max() == vp.period());
    }
}

TEST_CASE("Cleanup", "[VitalsPublisher::~VitalsPublisher]")
{
    fakeit::Mock<particle::mock_type::Timer> mock_timer;

    // Configure Mock
    fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
    fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
    fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
    fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
    fakeit::When(Method(mock_timer, start)).AlwaysReturn();
    fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

    SECTION("When destroyed, then the timer will be disposed.")
    {
        // Arrange

        // Act
        {
            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(nullptr,
                                                                             &mock_timer.get());
        }

        // Assert
        fakeit::Verify(Method(mock_timer, dispose));
    }
}

TEST_CASE("Stop publishing", "[VitalsPublisher::disablePeriodicPublish]")
{
    fakeit::Mock<particle::mock_type::Timer> mock_timer;
    particle::system::VitalsPublisher<particle::mock_type::Timer> vp(nullptr, &mock_timer.get());

    // Configure Mock
    fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
    fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
    fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
    fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
    fakeit::When(Method(mock_timer, start)).AlwaysReturn();
    fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

    SECTION("When disabled, then the timer will be disposed.")
    {
        // Arrange

        // Act
        vp.disablePeriodicPublish();

        // Assert
        fakeit::Verify(Method(mock_timer, stop));
    }
}

TEST_CASE("Start publishing", "[VitalsPublisher::enablePeriodicPublish]")
{
    fakeit::Mock<particle::mock_type::Timer> mock_timer;
    particle::system::VitalsPublisher<particle::mock_type::Timer> vp(nullptr, &mock_timer.get());

    // Configure Mock
    fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
    fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
    fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
    fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
    fakeit::When(Method(mock_timer, start)).AlwaysReturn();
    fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

    SECTION("When enabled, then the timer will be started.")
    {
        // Arrange

        // Act
        vp.enablePeriodicPublish();

        // Assert
        fakeit::Verify(Method(mock_timer, start));
    }
}

TEST_CASE("Setting period value", "[VitalsPublisher::period]")
{
    fakeit::Mock<particle::mock_type::Timer> mock_timer;
    particle::system::VitalsPublisher<particle::mock_type::Timer> vp(nullptr, &mock_timer.get());

    // Configure Mock
    fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
    fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
    fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
    fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
    fakeit::When(Method(mock_timer, start)).AlwaysReturn();
    fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

    SECTION("When setter is invoked, then period is stored.")
    {
        // Arrange
        const system_tick_t PERIOD_VALUE = 1979;

        // Act
        vp.period(PERIOD_VALUE);

        // Assert
        CHECK(PERIOD_VALUE == vp.period());
    }

    SECTION("When setter is invoked (seconds), then timer period is updated (milliseconds)")
    {
        // Arrange
        const system_tick_t PERIOD_S = 1979;
        const system_tick_t PERIOD_MS = (PERIOD_S * 1000);

        // Act
        vp.period(PERIOD_S);

        // Assert
        fakeit::Verify(Method(mock_timer, changePeriod).Using(PERIOD_MS));
    }

    SECTION("When unable to update timer period, then publisher period is not updated")
    {
        // Arrange
        const system_tick_t PERIOD_S = 1979;

        fakeit::When(Method(mock_timer, changePeriod)).Return(false);

        // Act
        vp.period(PERIOD_S);

        // Assert
        fakeit::Verify(Method(mock_timer, changePeriod));
        CHECK(std::numeric_limits<system_tick_t>::max() == vp.period());
    }

    SECTION("When timer is active before period is updated, then timer is reset")
    {
        // Arrange
        const system_tick_t PERIOD_S = 1979;

        fakeit::When(Method(mock_timer, isActive)).Return(true);

        // Act
        vp.period(PERIOD_S);

        // Assert
        fakeit::Verify(Method(mock_timer, isActive) + Method(mock_timer, changePeriod) +
                       Method(mock_timer, reset));
    }

    SECTION("When timer is inactive before period is updated, then timer is stopped")
    {
        // Arrange
        const system_tick_t PERIOD_S = 1979;

        fakeit::When(Method(mock_timer, isActive)).Return(false);

        // Act
        vp.period(PERIOD_S);

        // Assert
        fakeit::Verify(Method(mock_timer, isActive) + Method(mock_timer, changePeriod) +
                       Method(mock_timer, stop));
    }
}

TEST_CASE("Publish vitals", "[VitalsPublisher::publish]")
{
    bool mock_publish_fn_called;
    int mock_publish_fn_result;
    particle::system::VitalsPublisher<particle::mock_type::Timer>::publish_fn_t mock_publish_fn =
        [&mock_publish_fn_called, &mock_publish_fn_result](void) -> int {
        mock_publish_fn_called = true;
        return mock_publish_fn_result;
    };
    particle::mock_type::Timer t;

    SECTION("When publish is called on VitalsPublisher, then the stored publish function (passed "
            "to constructor) is called.")
    {
        // Arrange
        mock_publish_fn_called = false;
        particle::system::VitalsPublisher<particle::mock_type::Timer> vp(mock_publish_fn, &t);

        // Act
        vp.publish();

        // Assert
        CHECK(mock_publish_fn_called);
    }

    SECTION("When publish is called on VitalsPublisher, then the result of the stored publish "
            "function is returned to caller.")
    {
        // Arrange
        const int EXPECTED_RESULT = 1979;
        mock_publish_fn_called = false;
        mock_publish_fn_result = EXPECTED_RESULT;
        particle::system::VitalsPublisher<particle::mock_type::Timer> vp(mock_publish_fn, &t);

        // Act
        const int ACTUAL_RESULT = vp.publish();

        // Assert
        REQUIRE(mock_publish_fn_called);
        CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
    }
}
