/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "coap_message_encoder.h"

#include <catch2/catch.hpp>

#include <memory>

namespace {

using namespace particle::protocol;

inline CoapMessageEncoder makeEncoder(char* buf = nullptr, size_t size = 0) {
    return CoapMessageEncoder(buf, size);
}

} // namespace

TEST_CASE("CoapMessageEncoder") {
    SECTION("can be used with a null buffer") {
        auto e = makeEncoder(nullptr, 0);
        e.type(CoapType::CON);
        CHECK(e.encode() == 4);
    }
    SECTION("can be used with a buffer smaller than the size of the encoded message") {
        char buf[3] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        CHECK(e.encode() == 4);
    }
    SECTION("fails if message type is not specified") {
        CoapMessageEncoder e(nullptr, 0);
        CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
    }
    SECTION("fails if message fields are encoded in incorrect order") {
        auto e = makeEncoder();
        SECTION("encoding message type more than once") {
            e.type(CoapType::CON);
            e.type(CoapType::CON);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message code more than once") {
            e.type(CoapType::CON);
            e.code(CoapCode::CONTENT);
            e.code(CoapCode::CONTENT);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message ID more than once") {
            e.type(CoapType::CON);
            e.id(1234);
            e.id(1234);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding token more than once") {
            e.type(CoapType::CON);
            e.token("t", 1);
            e.token("t", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding payload more than once") {
            e.type(CoapType::CON);
            e.payload("p", 1);
            e.payload("p", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding options in descending order") {
            e.type(CoapType::CON);
            e.option(1235, 1);
            e.option(1234, 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding token without specifying message type") {
            e.token("t", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding options without specifying message type") {
            e.option(1234, 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding payload without specifying message type") {
            e.payload("p", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding token after options") {
            e.type(CoapType::CON);
            e.option(1234, 1);
            e.token("t", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding token after payload") {
            e.type(CoapType::CON);
            e.payload("p", 1);
            e.token("t", 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message code after token") {
            e.type(CoapType::CON);
            e.token("t", 1);
            e.code(CoapCode::CONTENT);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message code after options") {
            e.type(CoapType::CON);
            e.option(1234, 1);
            e.code(CoapCode::CONTENT);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message code after payload") {
            e.type(CoapType::CON);
            e.payload("p", 1);
            e.code(CoapCode::CONTENT);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message ID after token") {
            e.type(CoapType::CON);
            e.token("t", 1);
            e.id(1234);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message ID after options") {
            e.type(CoapType::CON);
            e.option(1234, 1);
            e.id(1234);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding message ID after payload") {
            e.type(CoapType::CON);
            e.payload("p", 1);
            e.id(1234);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
        SECTION("encoding options after payload") {
            e.type(CoapType::CON);
            e.payload("p", 1);
            e.option(1234, 1);
            CHECK(e.encode() == SYSTEM_ERROR_INVALID_STATE);
        }
    }
    SECTION("encodes message type correctly") {
        char buf[4] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        SECTION("CON") {
            e.type(CoapType::CON);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x40\x00\x00\x00", 4) == 0);
        }
        SECTION("NON") {
            e.type(CoapType::NON);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x50\x00\x00\x00", 4) == 0);
        }
        SECTION("ACK") {
            e.type(CoapType::ACK);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x60\x00\x00\x00", 4) == 0);
        }
        SECTION("RST") {
            e.type(CoapType::RST);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x70\x00\x00\x00", 4) == 0);
        }
    }
    SECTION("encodes message code correctly") {
        struct {
            CoapCode id;
            uint8_t value;
        } codes[] = {
            { CoapCode::EMPTY, 0x00 }, // 0.00
            { CoapCode::GET, 0x01 }, // 0.01
            { CoapCode::POST, 0x02 }, // 0.02
            { CoapCode::PUT, 0x03 }, // 0.03
            { CoapCode::DELETE, 0x04 }, // 0.04
            { CoapCode::CREATED, 0x41 }, // 2.01
            { CoapCode::DELETED, 0x42 }, // 2.02
            { CoapCode::VALID, 0x43 }, // 2.03
            { CoapCode::CHANGED, 0x44 }, // 2.04
            { CoapCode::CONTENT, 0x45 }, // 2.05
            { CoapCode::BAD_REQUEST, 0x80 }, // 4.00
            { CoapCode::UNAUTHORIZED, 0x81 }, // 4.01
            { CoapCode::BAD_OPTION, 0x82 }, // 4.02
            { CoapCode::FORBIDDEN, 0x83 }, // 4.03
            { CoapCode::NOT_FOUND, 0x84 }, // 4.04
            { CoapCode::METHOD_NOT_ALLOWED, 0x85 }, // 4.05
            { CoapCode::NOT_ACCEPTABLE, 0x86 }, // 4.06
            { CoapCode::PRECONDITION_FAILED, 0x8c }, // 4.12
            { CoapCode::REQUEST_ENTITY_TOO_LARGE, 0x8d }, // 4.13
            { CoapCode::UNSUPPORTED_CONTENT_FORMAT, 0x8f }, // 4.15
            { CoapCode::INTERNAL_SERVER_ERROR, 0xa0 }, // 5.00
            { CoapCode::NOT_IMPLEMENTED, 0xa1 }, // 5.01
            { CoapCode::BAD_GATEWAY, 0xa2 }, // 5.02
            { CoapCode::SERVICE_UNAVAILABLE, 0xa3 }, // 5.03
            { CoapCode::GATEWAY_TIMEOUT, 0xa4 }, // 5.04
            { CoapCode::PROXYING_NOT_SUPPORTED, 0xa5 } // 5.05
        };
        char buf[4] = {};
        for (auto code: codes) {
            auto e = makeEncoder(buf, sizeof(buf));
            e.type(CoapType::CON);
            e.code(code.id);
            CHECK(e.encode() == 4);
            CHECK((uint8_t)buf[1] == code.value);
        }
    }
    SECTION("encodes message ID correctly") {
        char buf[4] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        SECTION("0") {
            e.id(0);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x40\x45\x00\x00", 4) == 0);
        }
        SECTION("1") {
            e.id(1);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x40\x45\x00\x01", 4) == 0);
        }
        SECTION("65535") {
            e.id(65535);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x40\x45\xff\xff", 4) == 0);
        }
    }
    SECTION("encodes message token correctly") {
        char buf[12] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        SECTION("empty") {
            e.token("\x01", 0);
            CHECK(e.encode() == 4);
            CHECK(memcmp(buf, "\x40\x45\x04\xd2", 4) == 0);
        }
        SECTION("1 byte") {
            e.token("\x01", 1);
            CHECK(e.encode() == 5);
            CHECK(memcmp(buf, "\x41\x45\x04\xd2\x01", 5) == 0);
        }
        SECTION("8 bytes") {
            e.token("\x01\x02\x03\x04\x05\x06\x07\x08", 8);
            CHECK(e.encode() == 12);
            CHECK(memcmp(buf, "\x48\x45\x04\xd2\x01\x02\x03\x04\x05\x06\x07\x08", 12) == 0);
        }
    }
    SECTION("fails if message token is too long") {
        auto e = makeEncoder(nullptr, 0);
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\x01\x02\x03\x04\x05\x06\x07\x08\x09", 9);
        CHECK(e.encode() == SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    SECTION("encodes option numbers correctly") {
        char buf[16] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        SECTION("0") { // Option number delta is encoded in the same byte with the option length
            e.option(0);
            CHECK(e.encode() == 7);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\x00", 7) == 0);
        }
        SECTION("12") { // ditto
            e.option(12);
            CHECK(e.encode() == 7);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\xc0", 7) == 0);
        }
        SECTION("13") { // Extended 1-byte option number delta
            e.option(13);
            CHECK(e.encode() == 8);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\xd0\x00", 8) == 0);
        }
        SECTION("268") { // ditto
            e.option(268);
            CHECK(e.encode() == 8);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\xd0\xff", 8) == 0);
        }
        SECTION("269") { // Extended 2-byte option number delta
            e.option(269);
            CHECK(e.encode() == 9);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\xe0\x00\x00", 9) == 0);
        }
        SECTION("65804") { // ditto
            e.option(65804);
            CHECK(e.encode() == 9);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\xe0\xff\xff", 9) == 0);
        }
    }
    SECTION("fails if option number is too large") {
        auto e = makeEncoder(nullptr, 0);
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        e.option(65805);
        CHECK(e.encode() == SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    SECTION("encodes option values correctly") {
        auto bufSize = 128 * 1024;
        auto buf = std::make_unique<char[]>(bufSize);
        auto e = makeEncoder(buf.get(), bufSize);
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        SECTION("empty") {
            e.option(1);
            CHECK(e.encode() == 7);
            CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x10", 7) == 0);
        }
        SECTION("uint") {
            SECTION("0") {
                e.option(1, 0);
                CHECK(e.encode() == 7);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x10", 7) == 0);
            }
            SECTION("1 byte") {
                e.option(1, 0x01);
                CHECK(e.encode() == 8);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x11\x01", 8) == 0);
            }
            SECTION("2 bytes") {
                e.option(1, 0x0102);
                CHECK(e.encode() == 9);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x12\x01\x02", 9) == 0);
            }
            SECTION("4 bytes") {
                // For simplicity, values in the range [2^16, 2^24) are encoded in 4 bytes
                e.option(1, 0x00010203);
                CHECK(e.encode() == 11);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x14\x00\x01\x02\x03", 11) == 0);
            }
        }
        SECTION("string") {
            SECTION("empty") { // Option length is encoded in the same byte with the option number delta
                e.option(1, "");
                CHECK(e.encode() == 7);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x10", 7) == 0);
            }
            SECTION("12 bytes") { // ditto
                e.option(1, "abcdefghijkl");
                CHECK(e.encode() == 19);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x1c\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c", 19) == 0);
            }
            SECTION("13 bytes") { // Extended 1-byte option length
                e.option(1, "abcdefghijklm");
                CHECK(e.encode() == 21);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x1d\x00\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d", 21) == 0);
            }
            SECTION("268 bytes") { // ditto
                auto s = std::string(268, 'x');
                e.option(1, s.data());
                CHECK(e.encode() == 276);
                CHECK(std::string(buf.get(), 276) == std::string("\x42\x45\x04\xd2\xaa\xbb\x1d\xff", 8) + s);
            }
            SECTION("269 bytes") { // Extended 2-byte option length
                auto s = std::string(269, 'x');
                e.option(1, s.data());
                CHECK(e.encode() == 278);
                CHECK(std::string(buf.get(), 278) == std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\x00\x00", 9) + s);
            }
            SECTION("65804 bytes") { // ditto
                auto s = std::string(65804, 'x');
                e.option(1, s.data());
                CHECK(e.encode() == 65813);
                CHECK(std::string(buf.get(), 65813) == std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\xff\xff", 9) + s);
            }
        }
        SECTION("opaque") {
            // Opaque and string options are encoded identically. This test only verifies that the
            // relevant API methods take a size argument explicitly
            SECTION("empty") {
                e.option(1, nullptr, 0);
                CHECK(e.encode() == 7);
                CHECK(memcmp(buf.get(), "\x42\x45\x04\xd2\xaa\xbb\x10", 7) == 0);
            }
            SECTION("500 bytes") {
                auto s = std::string(500, 'x');
                e.option(1, s.data(), s.size());
                CHECK(e.encode() == 509);
                CHECK(std::string(buf.get(), 509) == std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\x00\xe7", 9) + s);
            }
        }
    }
    SECTION("fails if option value is too long") {
        auto e = makeEncoder(nullptr, 0);
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        e.option(1, std::string(65805, 'x').data());
        CHECK(e.encode() == SYSTEM_ERROR_INVALID_ARGUMENT);
    }
    SECTION("encodes multiple options correctly") {
        char buf[2048] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        SECTION("repeated options") {
            e.option(1, 10);
            e.option(1, 100);
            e.option(1, 1000);
            e.option(1, 10000);
            CHECK(e.encode() == 16);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\x11\x0a\x01\x64\x02\x03\xe8\x02\x27\x10", 16) == 0);
        }
        SECTION("options of different format and length") {
            e.option(1); // empty
            e.option(1 + 12, 32767); // uint
            auto s1 = std::string("abcdefghijklmnopqrstuvwxyz");
            e.option(1 + 12 + 269, s1.data()); // string
            auto s2 = std::string(1000, 'x');
            e.option(1 + 12 + 269 + 10000, s2.data(), s2.size()); // opaque
            CHECK(e.encode() == 1045);
            CHECK(std::string(buf, 1045) == std::string("\x42\x45\x04\xd2\xaa\xbb\x10\xc2\x7f\xff\xed\x00\x00\x0d", 14) + s1 +
                    std::string("\xee\x26\x03\x02\xdb", 5) + s2);
        }
        SECTION("basic options") {
            e.option(CoapOption::IF_MATCH, "a", 1); // opaque
            e.option(CoapOption::URI_HOST, "b"); // string
            e.option(CoapOption::ETAG, "c", 1); // opaque
            e.option(CoapOption::IF_NONE_MATCH); // empty
            e.option(CoapOption::URI_PORT, 1); // uint
            e.option(CoapOption::LOCATION_PATH, "d"); // string
            e.option(CoapOption::URI_PATH, "e"); // string
            e.option(CoapOption::CONTENT_FORMAT, 2); // uint
            e.option(CoapOption::MAX_AGE, 3); // uint
            e.option(CoapOption::URI_QUERY, "f"); // string
            e.option(CoapOption::ACCEPT, 4); // uint
            e.option(CoapOption::LOCATION_QUERY, "g"); // string
            e.option(CoapOption::PROXY_URI, "h"); // string
            e.option(CoapOption::PROXY_SCHEME, "i"); // string
            e.option(CoapOption::SIZE1, 5); // uint
            e.payload("x", 1);
            CHECK(e.encode() == 39);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\x11\x61\x21\x62\x11\x63\x10\x21\x01\x11\x64\x31\x65\x11\x02\x21\x03\x11\x66\x21\x04\x31\x67\xd1\x02\x68\x41\x69\xd1\x08\x05\xff\x78", 39) == 0);
        }
    }
    SECTION("encodes payload data correctly") {
        char buf[2048] = {};
        auto e = makeEncoder(buf, sizeof(buf));
        e.type(CoapType::CON);
        e.code(CoapCode::CONTENT);
        e.id(1234);
        e.token("\xaa\xbb", 2);
        e.option(1, "abcd");
        SECTION("empty") {
            e.payload(nullptr, 0);
            CHECK(e.encode() == 11);
            CHECK(memcmp(buf, "\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64", 11) == 0);
        }
        SECTION("1000 bytes") {
            auto s = std::string(1000, 'x');
            e.payload(s.data(), s.size());
            CHECK(e.encode() == 1012);
            CHECK(std::string(buf, 1012) == std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64\xff", 12) + s);
        }
    }
    SECTION("allows encoding payload data in place") {
        SECTION("too small buffer") {
            char buf[5] = {};
            auto e = makeEncoder(buf, sizeof(buf));
            e.type(CoapType::CON);
            e.code(CoapCode::CONTENT);
            e.id(1234);
            CHECK(e.maxPayloadSize() == 0);
            CHECK(e.payloadData() == nullptr);
            e.payloadSize(1);
            CHECK(e.encode() == 6);
            CHECK(std::string(buf, 5) == std::string("\x40\x45\x04\xd2\xff", 5));
        }
        SECTION("message without token and options") {
            char buf[6] = {};
            auto e = makeEncoder(buf, sizeof(buf));
            e.type(CoapType::CON);
            e.code(CoapCode::CONTENT);
            e.id(1234);
            CHECK(e.maxPayloadSize() == 1);
            CHECK(e.payloadData() == buf + 5);
            *e.payloadData() = 'x';
            e.payloadSize(1);
            CHECK(e.encode() == 6);
            CHECK(std::string(buf, 6) == std::string("\x40\x45\x04\xd2\xff\x78", 6));
        }
        SECTION("message with token and options") {
            char buf[16] = {};
            auto e = makeEncoder(buf, sizeof(buf));
            e.type(CoapType::CON);
            e.code(CoapCode::CONTENT);
            e.id(1234);
            e.token("\xaa\xbb", 2);
            e.option(1, "abcd");
            CHECK(e.maxPayloadSize() == 4);
            CHECK(e.payloadData() == buf + 12);
            memcpy(e.payloadData(), "xxxx", 4);
            e.payloadSize(4);
            CHECK(e.encode() == 16);
            CHECK(std::string(buf, 16) == std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64\xff\x78\x78\x78\x78", 16));
        }
    }
}
