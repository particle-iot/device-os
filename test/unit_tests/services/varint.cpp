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

#include "varint.h"

#include <catch2/catch.hpp>

using namespace particle;

TEST_CASE("encodeUnsignedVarint()") {
    SECTION("allows the destination buffer to be empty") {
        auto r = encodeUnsignedVarint(nullptr, 0, 123);
        CHECK(r == 1);
        r = encodeUnsignedVarint(nullptr, 0, 1234);
        CHECK(r == 2);
    }

    SECTION("does not attempt to write more than `size` bytes to the destination buffer") {
        char buf[2] = {};
        auto r = encodeUnsignedVarint(buf, 1, 1234);
        CHECK(r == 2);
        CHECK(buf[1] == 0);
    }

    SECTION("encodes unsigned varints correctly") {
        char buf[8] = {};
        // 0
        auto r = encodeUnsignedVarint(buf, sizeof(buf), 0);
        CHECK(r == 1);
        CHECK(memcmp(buf, "\x00", 1) == 0);
        // 1
        r = encodeUnsignedVarint(buf, sizeof(buf), 1);
        CHECK(r == 1);
        CHECK(memcmp(buf, "\x01", 1) == 0);
        // 127
        r = encodeUnsignedVarint(buf, sizeof(buf), 127);
        CHECK(r == 1);
        CHECK(memcmp(buf, "\x7f", 1) == 0);
        // 128
        r = encodeUnsignedVarint(buf, sizeof(buf), 128);
        CHECK(r == 2);
        CHECK(memcmp(buf, "\x80\x01", 2) == 0);
        // 300
        r = encodeUnsignedVarint(buf, sizeof(buf), 300);
        CHECK(r == 2);
        CHECK(memcmp(buf, "\xac\x02", 2) == 0);
        // 16383
        r = encodeUnsignedVarint(buf, sizeof(buf), 16383);
        CHECK(r == 2);
        CHECK(memcmp(buf, "\xff\x7f", 2) == 0);
        // 16384
        r = encodeUnsignedVarint(buf, sizeof(buf), 16384);
        CHECK(r == 3);
        CHECK(memcmp(buf, "\x80\x80\x01", 3) == 0);
        // 300000
        r = encodeUnsignedVarint(buf, sizeof(buf), 300000);
        CHECK(r == 3);
        CHECK(memcmp(buf, "\xe0\xa7\x12", 3) == 0);
        // 0xffffffff
        r = encodeUnsignedVarint(buf, sizeof(buf), 0xffffffffu);
        CHECK(r == 5);
        CHECK(memcmp(buf, "\xff\xff\xff\xff\x0f", 5) == 0);
    }
}

TEST_CASE("decodeUnsignedVarint()") {
    SECTION("fails if the source buffer is empty") {
        unsigned v = 0;
        auto r = decodeUnsignedVarint(nullptr, 0, &v);
        CHECK(r == SYSTEM_ERROR_NOT_ENOUGH_DATA);
    }

    SECTION("fails if the varint data is incomplete") {
        unsigned v = 0;
        char buf[] = "\x80";
        auto r = decodeUnsignedVarint(buf, 1, &v);
        CHECK(r == SYSTEM_ERROR_NOT_ENOUGH_DATA);
    }

    SECTION("returns the number of bytes read") {
        unsigned v = 0;
        char buf[] = "\x80\x01qwerty"; // 128
        auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
        CHECK(r == 2);
    }

    SECTION("allows the pointer to the destination variable to be null") {
        char buf[] = "\x80\x01"; // 128
        auto r = decodeUnsignedVarint<unsigned>(buf, sizeof(buf), nullptr);
        CHECK(r == 2);
    }

    SECTION("fails if the decoded value does not fit into the destination variable") {
        {
            uint8_t v = 0;
            char buf[] = "\x80\x02"; // 256
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == SYSTEM_ERROR_TOO_LARGE);
        }
        {
            uint16_t v = 0;
            char buf[] = "\x80\x80\x04"; // 65536
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == SYSTEM_ERROR_TOO_LARGE);
        }
        {
            uint32_t v = 0;
            char buf[] = "\x80\x80\x80\x80\x10"; // 0x100000000
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == SYSTEM_ERROR_TOO_LARGE);
        }
    }

    SECTION("decodes unsigned varints correctly") {
        {
            uint8_t v = -1;
            char buf[] = "\x00"; // 0
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 1);
            CHECK(v == 0);
        }
        {
            uint8_t v = 0;
            char buf[] = "\x01"; // 1
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 1);
            CHECK(v == 1);
        }
        {
            uint8_t v = 0;
            char buf[] = "\x7f"; // 127
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 1);
            CHECK(v == 127);
        }
        {
            uint8_t v = 0;
            char buf[] = "\x80\x01"; // 128
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 2);
            CHECK(v == 128);
        }
        {
            uint8_t v = 0;
            char buf[] = "\xff\x01"; // 255
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 2);
            CHECK(v == 255);
        }
        {
            uint16_t v = 0;
            char buf[] = "\xac\x02"; // 300
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 2);
            CHECK(v == 300);
        }
        {
            uint16_t v = 0;
            char buf[] = "\xff\x7f"; // 16383
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 2);
            CHECK(v == 16383);
        }
        {
            uint16_t v = 0;
            char buf[] = "\x80\x80\x01"; // 16384
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 3);
            CHECK(v == 16384);
        }
        {
            uint16_t v = 0;
            char buf[] = "\xff\xff\x03"; // 65535
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 3);
            CHECK(v == 65535);
        }
        {
            uint32_t v = 0;
            char buf[] = "\xe0\xa7\x12"; // 300000
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 3);
            CHECK(v == 300000);
        }
        {
            uint32_t v = 0;
            char buf[] = "\xff\xff\xff\xff\x0f"; // 0xffffffff
            auto r = decodeUnsignedVarint(buf, sizeof(buf), &v);
            CHECK(r == 5);
            CHECK(v == 0xffffffff);
        }
    }
}
