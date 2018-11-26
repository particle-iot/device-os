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

#include "str_util.h"
#include "catch.h"

using namespace particle;

TEST_CASE("escape()") {
    char buf[10] = {};

    SECTION("source string can be empty") {
        auto n = escape("", "", '@', buf, sizeof(buf));
        CHECK(n == 0);
    }
    SECTION("null-terminates the destination buffer") {
        memset(buf, 0xff, sizeof(buf));
        auto n = escape("", "", '@', buf, sizeof(buf));
        CHECK(n == 0);
        CHECK(buf[0] == '\0');
    }
    SECTION("escapes a string") {
        auto n = escape("abcd", "bd", '@', buf, sizeof(buf));
        CHECK(n == 6);
        CHECK(strcmp(buf, "a@bc@d") == 0);
    }
    SECTION("escapes a 1 character long string") {
        auto n = escape("a", "a", '@', buf, sizeof(buf));
        CHECK(strcmp(buf, "@a") == 0);
        CHECK(n == 2);
    }
    SECTION("string with escapable characters can be empty") {
        auto n = escape("abcd", "", '@', buf, sizeof(buf));
        CHECK(n == 4);
        CHECK(strcmp(buf, "abcd") == 0);
    }
    SECTION("destination buffer can be smaller than the source string") {
        auto n = escape("abcdefghijklmnop", "ap", '@', buf, sizeof(buf));
        CHECK(n == 18); // Size of the entire escaped string
        CHECK(strcmp(buf, "@abcdefgh") == 0);
    }
    SECTION("destination buffer can be empty") {
        auto n = escape("abcd", "bd", '@', nullptr, 0);
        CHECK(n == 6);
    }
}
