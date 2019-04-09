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

// Defined in CMakeList.txt
// Redefined to enable Intellisense
#ifdef LOG_DISABLE
#undef LOG_DISABLE
#endif
#define LOG_DISABLE
#ifdef UNIT_TEST
#undef UNIT_TEST
#endif
#define UNIT_TEST

#include "../test/unit_tests/mock_types.h"
#include "system_publish_vitals.h"

// ASSUMPTION!!! - Period "getter" method works correctly - without being testing.

TEST_CASE("Construction", "[VitalsPublisher::VitalsPublisher]")
{
    SECTION("Initialized values")
    {
        // Arrange
        GIVEN("Default Construction")
        {
            WHEN("Constructed")
            {
                particle::system::VitalsPublisher<particle::mock_type::Timer> vp;

                THEN("VitalsPublisher::period is initialized to MAX value")
                {
                    CHECK(std::numeric_limits<system_tick_t>::max() == vp.period());
                }
            }
        }
    }
}

TEST_CASE("Destruction", "[VitalsPublisher::~VitalsPublisher]")
{
    SECTION("Resource reclaimation")
    {
        GIVEN("Initialized Timer behavior")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            WHEN("Destroyed")
            {
                {
                    particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());
                }

                THEN("Timer is disposed")
                {
                    fakeit::Verify(Method(mock_timer, dispose));
                }
            }
        }
    }
}

TEST_CASE("Stop publishing", "[VitalsPublisher::disablePeriodicPublish]")
{
    SECTION("Timer state")
    {
        GIVEN("Initialized Timer behavior")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Disabled")
            {
                vp.disablePeriodicPublish();

                THEN("Timer is stopped")
                {
                    fakeit::Verify(Method(mock_timer, stop));
                }
            }
        }
    }
}

TEST_CASE("Start publishing", "[VitalsPublisher::enablePeriodicPublish]")
{
    SECTION("Timer state")
    {
        GIVEN("Initialized Timer behavior")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Enabled")
            {
                vp.enablePeriodicPublish();

                THEN("Timer is started")
                {
                    fakeit::Verify(Method(mock_timer, start));
                }
            }
        }
    }
}

TEST_CASE("Setting period value", "[VitalsPublisher::period]")
{
    SECTION("Internal period value")
    {
        GIVEN("A known period value (seconds) and initialized Timer behavior")
        {
            const system_tick_t PERIOD_S = 1979;
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Setter Invoked")
            {
                vp.period(PERIOD_S);

                THEN("Period is stored")
                {
                    CHECK(PERIOD_S == vp.period());
                }
            }
        }

        GIVEN("Timer fails to update period")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Setter Invoked")
            {
                vp.period(1979);

                THEN("Period is not updated")
                {
                    fakeit::Verify(Method(mock_timer, changePeriod));
                    CHECK(std::numeric_limits<system_tick_t>::max() == vp.period());
                }
            }
        }
    }

    SECTION("Timer state")
    {
        GIVEN("A known period value (seconds) and initialized Timer behavior")
        {
            const system_tick_t PERIOD_S = 1979;
            const system_tick_t PERIOD_MS = (PERIOD_S * 1000);
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Setter Invoked")
            {
                vp.period(PERIOD_S);

                THEN("Timer period updated in milliseconds")
                {
                    fakeit::Verify(Method(mock_timer, changePeriod).Using(PERIOD_MS));
                }
            }
        }

        GIVEN("Timer is active")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Setter Invoked")
            {
                vp.period(1979);

                THEN("Timer is reset")
                {
                    fakeit::Verify(Method(mock_timer, isActive) + Method(mock_timer, changePeriod) +
                                   Method(mock_timer, reset));
                }
            }
        }

        GIVEN("Timer is inactive")
        {
            fakeit::Mock<particle::mock_type::Timer> mock_timer;

            // Configure Mock
            fakeit::When(Method(mock_timer, changePeriod)).AlwaysReturn(true);
            fakeit::When(Method(mock_timer, dispose)).AlwaysReturn();
            fakeit::When(Method(mock_timer, isActive)).AlwaysReturn(false);
            fakeit::When(Method(mock_timer, reset)).AlwaysReturn();
            fakeit::When(Method(mock_timer, start)).AlwaysReturn();
            fakeit::When(Method(mock_timer, stop)).AlwaysReturn();

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp(&mock_timer());

            WHEN("Setter Invoked")
            {
                vp.period(1979);

                THEN("Timer is reset")
                {
                    fakeit::Verify(Method(mock_timer, isActive) + Method(mock_timer, changePeriod) +
                                   Method(mock_timer, stop));
                }
            }
        }
    }
}

TEST_CASE("Publishing vitals", "[VitalsPublisher::publish]")
{
    SECTION("spark_protocol_post_description invocation")
    {
        extern bool spark_protocol_post_description_called;
        extern int spark_protocol_post_description_result;

        GIVEN("A VitalsPublisher with mocked publish function and expected results")
        {
            const int EXPECTED_RESULT = 1979;
            spark_protocol_post_description_called = false;
            spark_protocol_post_description_result = EXPECTED_RESULT;

            particle::system::VitalsPublisher<particle::mock_type::Timer> vp;

            WHEN("Publish Invoked")
            {
                const int ACTUAL_RESULT = vp.publish();

                THEN("Static function invoked")
                {
                    CHECK(spark_protocol_post_description_called);
                }

                THEN("spark_protocol_post_description result is returned to caller")
                {
                    CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
                }
            }
        }
    }
}
