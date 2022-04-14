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

#include <catch2/catch.hpp>

using namespace particle;

TEST_CASE("escape()") {
    char buf[100] = {};

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
        auto n = escape("abcdefghijklmnop", "ap", '@', buf, 10);
        CHECK(n == 18); // Size of the entire escaped string
        CHECK(strcmp(buf, "@abcdefgh") == 0);
    }
    SECTION("destination buffer can be empty") {
        auto n = escape("abcd", "bd", '@', nullptr, 0);
        CHECK(n == 6);
    }
}

TEST_CASE("toHex()") {
    char buf[100] = {};

    SECTION("source data can be empty") {
        auto n = toHex(nullptr, 0, buf, sizeof(buf));
        CHECK(n == 0);
    }
    SECTION("null-terminates the destination buffer") {
        memset(buf, 0xff, sizeof(buf));
        auto n = toHex(nullptr, 0, buf, sizeof(buf));
        CHECK(n == 0);
        CHECK(buf[0] == '\0');
    }
    SECTION("converts data to hex correctly") {
        auto n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, sizeof(buf));
        CHECK(n == 16);
        CHECK(strcmp(buf, "0123456789abcdef") == 0);
    }
    SECTION("destination buffer can be smaller than necessary to store the entire string") {
        memset(buf, 0xff, sizeof(buf));
        auto n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, 1);
        CHECK(n == 0);
        CHECK(strcmp(buf, "") == 0);
        n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, 2);
        CHECK(n == 1);
        CHECK(strcmp(buf, "0") == 0);
        n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, 3);
        CHECK(n == 2);
        CHECK(strcmp(buf, "01") == 0);
        n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, 4);
        CHECK(n == 3);
        CHECK(strcmp(buf, "012") == 0);
    }
    SECTION("destination buffer can be empty") {
        memset(buf, 0xff, sizeof(buf));
        auto n = toHex("\x01\x23\x45\x67\x89\xab\xcd\xef", 8, buf, 0);
        CHECK(n == 0);
        CHECK((uint8_t)buf[0] == 0xff);
    }
}
