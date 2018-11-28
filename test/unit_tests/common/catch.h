/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#define CATCH_CONFIG_PREFIX_ALL

#include <catch2/catch.hpp>

// Non-prefixed aliases for some typical macros that don't clash with the firmware code
#define CHECK(...) CATCH_CHECK(__VA_ARGS__)
#define CHECK_FALSE(...) CATCH_CHECK_FALSE(__VA_ARGS__)
#define REQUIRE(...) CATCH_REQUIRE(__VA_ARGS__)
#define REQUIRE_FALSE(...) CATCH_REQUIRE_FALSE(__VA_ARGS__)
#define FAIL(...) CATCH_FAIL(__VA_ARGS__)
#define TEST_CASE(...) CATCH_TEST_CASE(__VA_ARGS__)
#define SECTION(...) CATCH_SECTION(__VA_ARGS__)
