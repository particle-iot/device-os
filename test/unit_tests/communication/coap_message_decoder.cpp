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

#include "coap_message_decoder.h"

#include <catch2/catch.hpp>

#include <memory>

namespace {

using namespace particle::protocol;

inline CoapMessageDecoder makeDecoder() {
    return CoapMessageDecoder();
}

} // namespace

TEST_CASE("CoapMessageDecoder") {
    SECTION("fails if protocol version is invalid") {
        auto d = makeDecoder();
        auto buf = std::string("\xa0\x00\x00\x00", 4); // Ver=2
        CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_BAD_DATA);
    }
    SECTION("decodes message type correctly") {
        auto d = makeDecoder();
        SECTION("CON") {
            auto buf = std::string("\x40\x41\x00\x01", 4);
            CHECK(d.decode(buf.data(), buf.size()) == 4);
            CHECK(d.type() == CoapType::CON);
        }
        SECTION("NON") {
            auto buf = std::string("\x50\x41\x00\x01", 4);
            CHECK(d.decode(buf.data(), buf.size()) == 4);
            CHECK(d.type() == CoapType::NON);
        }
        SECTION("ACK") {
            auto buf = std::string("\x60\x00\x00\x01", 4);
            CHECK(d.decode(buf.data(), buf.size()) == 4);
            CHECK(d.type() == CoapType::ACK);
        }
        SECTION("RST") {
            auto buf = std::string("\x70\x00\x00\x01", 4);
            CHECK(d.decode(buf.data(), buf.size()) == 4);
            CHECK(d.type() == CoapType::RST);
        }
    }
    SECTION("decodes message code correctly") {
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
        auto d = makeDecoder();
        for (auto code: codes) {
            auto buf = std::string("\x40\x41\x00\x01", 4);
            buf[1] = code.value;
            d.decode(buf.data(), buf.size());
            CHECK(d.code() == code.id);
        }
    }
    SECTION("fails if message code is invalid") {
        auto d = makeDecoder();
        auto buf = std::string("\x40\x20\x00\x01", 4); // Code=1.00
        CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_BAD_DATA);
    }
    SECTION("decodes message token correctly") {
        auto d = makeDecoder();
        SECTION("empty") {
            auto buf = std::string("\x40\x45\x04\xd2", 4);
            CHECK(d.decode(buf.data(), buf.size()) == 4);
            CHECK(d.hasToken() == false);
            CHECK(d.tokenSize() == 0);
            CHECK(d.token() == nullptr);
        }
        SECTION("1 byte") {
            auto buf = std::string("\x41\x45\x04\xd2\x01", 5);
            CHECK(d.decode(buf.data(), buf.size()) == 5);
            CHECK(d.hasToken() == true);
            CHECK(std::string(d.token(), d.tokenSize()) == std::string("\x01", 1));
        }
        SECTION("8 bytes") {
            auto buf = std::string("\x48\x45\x04\xd2\x01\x02\x03\x04\x05\x06\x07\x08", 12);
            CHECK(d.decode(buf.data(), buf.size()) == 12);
            CHECK(d.hasToken() == true);
            CHECK(std::string(d.token(), d.tokenSize()) == std::string("\x01\x02\x03\x04\x05\x06\x07\x08", 8));
        }
    }
    SECTION("fails if token length is invalid") {
        auto d = makeDecoder();
        auto buf = std::string("\x49\x45\x04\xd2\x01\x02\x03\x04\x05\x06\x07\x08\x09", 13); // TKL=9
        CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_BAD_DATA);
    }
    SECTION("decodes message without options correctly") {
        auto d = makeDecoder();
        auto buf = std::string("\x41\x45\x04\xd2\x01", 5);
        CHECK(d.decode(buf.data(), buf.size()) == 5);
        CHECK(d.hasOptions() == false);
        CHECK(d.hasOption(CoapOption::URI_PATH) == false);
        auto it = d.findOption(CoapOption::URI_PATH);
        CHECK(it == CoapOptionIterator());
        CHECK(it == false);
        CHECK(!it == true);
        CHECK(it.option() == 0);
        CHECK(it.size() == 0);
        CHECK(it.data() == nullptr);
        CHECK(it.toUInt() == 0);
        CHECK(it.next() == false);
    }
    SECTION("decodes option numbers correctly") {
        auto d = makeDecoder();
        SECTION("0") { // Option number delta is encoded in the same byte with the option length
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x00", 7);
            CHECK(d.decode(buf.data(), buf.size()) == 7);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(0) == true);
            auto it = d.findOption(0);
            CHECK(it.option() == 0);
        }
        SECTION("12") { // ditto
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xc0", 7);
            CHECK(d.decode(buf.data(), buf.size()) == 7);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(12) == true);
            auto it = d.findOption(12);
            CHECK(it.option() == 12);
        }
        SECTION("13") { // Extended 1-byte option number delta
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xd0\x00", 8);
            CHECK(d.decode(buf.data(), buf.size()) == 8);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(13) == true);
            auto it = d.findOption(13);
            CHECK(it.option() == 13);
        }
        SECTION("268") { // ditto
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xd0\xff", 8);
            CHECK(d.decode(buf.data(), buf.size()) == 8);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(268) == true);
            auto it = d.findOption(268);
            CHECK(it.option() == 268);
        }
        SECTION("269") { // Extended 2-byte option number delta
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xe0\x00\x00", 9);
            CHECK(d.decode(buf.data(), buf.size()) == 9);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(269) == true);
            auto it = d.findOption(269);
            CHECK(it.option() == 269);
        }
        SECTION("65804") { // ditto
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xe0\xff\xff", 9);
            CHECK(d.decode(buf.data(), buf.size()) == 9);
            CHECK(d.hasOptions() == true);
            CHECK(d.hasOption(65804) == true);
            auto it = d.findOption(65804);
            CHECK(it.option() == 65804);
        }
    }
    SECTION("decodes option values correctly") {
        auto d = makeDecoder();
        SECTION("empty") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x10", 7);
            CHECK(d.decode(buf.data(), buf.size()) == 7);
            auto it = d.findOption(1);
            CHECK(it.size() == 0);
            CHECK(it.data() == nullptr);
            CHECK(it.toUInt() == 0);
        }
        SECTION("uint") {
            SECTION("0") {
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x10", 7);
                CHECK(d.decode(buf.data(), buf.size()) == 7);
                auto it = d.findOption(1);
                CHECK(it.size() == 0);
                CHECK(it.data() == nullptr);
                CHECK(it.toUInt() == 0);
            }
            SECTION("1 byte") {
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x11\x01", 8);
                CHECK(d.decode(buf.data(), buf.size()) == 8);
                auto it = d.findOption(1);
                CHECK(it.size() == 1);
                CHECK(it.data() != nullptr);
                CHECK(it.toUInt() == 0x01);
            }
            SECTION("2 bytes") {
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x12\x01\x02", 9);
                CHECK(d.decode(buf.data(), buf.size()) == 9);
                auto it = d.findOption(1);
                CHECK(it.size() == 2);
                CHECK(it.data() != nullptr);
                CHECK(it.toUInt() == 0x0102);
            }
            SECTION("3 bytes") {
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x13\x01\x02\x03", 11);
                CHECK(d.decode(buf.data(), buf.size()) == 11);
                auto it = d.findOption(1);
                CHECK(it.size() == 3);
                CHECK(it.data() != nullptr);
                CHECK(it.toUInt() == 0x00010203);
            }
            SECTION("4 bytes") {
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x01\x02\x03\x04", 11);
                CHECK(d.decode(buf.data(), buf.size()) == 11);
                auto it = d.findOption(1);
                CHECK(it.size() == 4);
                CHECK(it.data() != nullptr);
                CHECK(it.toUInt() == 0x01020304);
            }
        }
        SECTION("string/opaque") {
            SECTION("empty") { // Option length is encoded in the same byte with the option number delta
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x10", 7);
                CHECK(d.decode(buf.data(), buf.size()) == 7);
                auto it = d.findOption(1);
                CHECK(it.size() == 0);
                CHECK(it.data() == nullptr);
                CHECK(it.toUInt() == 0);
            }
            SECTION("12 bytes") { // ditto
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1c\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c", 19);
                CHECK(d.decode(buf.data(), buf.size()) == 19);
                auto it = d.findOption(1);
                CHECK(std::string(it.data(), it.size()) == "abcdefghijkl");
            }
            SECTION("13 bytes") { // Extended 1-byte option length
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1d\x00\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d", 21);
                CHECK(d.decode(buf.data(), buf.size()) == 21);
                auto it = d.findOption(1);
                CHECK(std::string(it.data(), it.size()) == "abcdefghijklm");
            }
            SECTION("268 bytes") { // ditto
                auto s = std::string(268, 'x');
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1d\xff", 8) + s;
                CHECK(d.decode(buf.data(), buf.size()) == 276);
                auto it = d.findOption(1);
                CHECK(std::string(it.data(), it.size()) == s);
            }
            SECTION("269 bytes") { // Extended 2-byte option length
                auto s = std::string(269, 'x');
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\x00\x00", 9) + s;
                CHECK(d.decode(buf.data(), buf.size()) == 278);
                auto it = d.findOption(1);
                CHECK(std::string(it.data(), it.size()) == s);
            }
            SECTION("65804 bytes") { // ditto
                auto s = std::string(65804, 'x');
                auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\xff\xff", 9) + s;
                CHECK(d.decode(buf.data(), buf.size()) == 65813);
                auto it = d.findOption(1);
                CHECK(std::string(it.data(), it.size()) == s);
            }
        }
    }
    SECTION("decodes multiple options correctly") {
        auto d = makeDecoder();
        SECTION("repeated options") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x11\x0a\x01\x64\x02\x03\xe8\x02\x27\x10", 16);
            CHECK(d.decode(buf.data(), buf.size()) == 16);
            CHECK(d.hasOptions() == true);
            auto it = d.options();
            // 1st option
            CHECK(it.next() == true);
            CHECK(it.option() == 1);
            CHECK(it.toUInt() == 10);
            // 2nd option
            CHECK(it.next() == true);
            CHECK(it.option() == 1);
            CHECK(it.toUInt() == 100);
            // 3rd option
            CHECK(it.next() == true);
            CHECK(it.option() == 1);
            CHECK(it.toUInt() == 1000);
            // 4th option
            CHECK(it.next() == true);
            CHECK(it.option() == 1);
            CHECK(it.toUInt() == 10000);
            CHECK(it.next() == false);
            CHECK(it == CoapOptionIterator());
        }
        SECTION("options of different format and length") {
            auto s1 = std::string("abcdefghijklmnopqrstuvwxyz");
            auto s2 = std::string(1000, 'x');
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x10\xc2\x7f\xff\xed\x00\x00\x0d", 14) + s1 +
                    std::string("\xee\x26\x03\x02\xdb", 5) + s2;
            CHECK(d.decode(buf.data(), buf.size()) == 1045);
            CHECK(d.hasOptions() == true);
            auto it = d.options();
            // Empty option
            CHECK(it.next() == true);
            CHECK(it.option() == 1);
            CHECK(it.size() == 0);
            CHECK(it.data() == nullptr);
            CHECK(it.toUInt() == 0);
            // Integer option
            CHECK(it.next() == true);
            CHECK(it.option() == 1 + 12);
            CHECK(it.size() != 0);
            CHECK(it.data() != nullptr);
            CHECK(it.toUInt() == 32767);
            // String option
            CHECK(it.next() == true);
            CHECK(it.option() == 1 + 12 + 269);
            CHECK(std::string(it.data(), it.size()) == s1);
            // 2nd string option
            CHECK(it.next() == true);
            CHECK(it.option() == 1 + 12 + 269 + 10000);
            CHECK(std::string(it.data(), it.size()) == s2);
            CHECK(it.next() == false);
            CHECK(it == CoapOptionIterator());
        }
        SECTION("basic options") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x11\x61\x21\x62\x11\x63\x10\x21\x01\x11\x64\x31\x65\x11\x02\x21\x03\x11\x66\x21\x04\x31\x67\xd1\x02\x68\x41\x69\xd1\x08\x05\xff\x78", 39);
            CHECK(d.decode(buf.data(), buf.size()) == 39);
            CHECK(d.hasOptions() == true);
            auto it = d.options();
            // If-Match (opaque)
            CHECK(d.hasOption(CoapOption::IF_MATCH));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::IF_MATCH);
            CHECK(std::string(it.data(), it.size()) == "a");
            // Uri-Host (string)
            CHECK(d.hasOption(CoapOption::URI_HOST));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::URI_HOST);
            CHECK(std::string(it.data(), it.size()) == "b");
            // ETag (opaque)
            CHECK(d.hasOption(CoapOption::ETAG));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::ETAG);
            CHECK(std::string(it.data(), it.size()) == "c");
            // If-None-Match (empty)
            CHECK(d.hasOption(CoapOption::IF_NONE_MATCH));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::IF_NONE_MATCH);
            CHECK(it.size() == 0);
            // Uri-Port (uint)
            CHECK(d.hasOption(CoapOption::URI_PORT));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::URI_PORT);
            CHECK(it.toUInt() == 1);
            // Location-Path (string)
            CHECK(d.hasOption(CoapOption::LOCATION_PATH));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::LOCATION_PATH);
            CHECK(std::string(it.data(), it.size()) == "d");
            // Uri-Path (string)
            CHECK(d.hasOption(CoapOption::URI_PATH));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::URI_PATH);
            CHECK(std::string(it.data(), it.size()) == "e");
            // Content-Format (uint)
            CHECK(d.hasOption(CoapOption::CONTENT_FORMAT));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::CONTENT_FORMAT);
            CHECK(it.toUInt() == 2);
            // Max-Age (uint)
            CHECK(d.hasOption(CoapOption::MAX_AGE));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::MAX_AGE);
            CHECK(it.toUInt() == 3);
            // Uri-Query (string)
            CHECK(d.hasOption(CoapOption::URI_QUERY));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::URI_QUERY);
            CHECK(std::string(it.data(), it.size()) == "f");
            // Accept (uint)
            CHECK(d.hasOption(CoapOption::ACCEPT));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::ACCEPT);
            CHECK(it.toUInt() == 4);
            // Location-Query (string)
            CHECK(d.hasOption(CoapOption::LOCATION_QUERY));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::LOCATION_QUERY);
            CHECK(std::string(it.data(), it.size()) == "g");
            // Proxy-Uri (string)
            CHECK(d.hasOption(CoapOption::PROXY_URI));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::PROXY_URI);
            CHECK(std::string(it.data(), it.size()) == "h");
            // Proxy-Scheme (string)
            CHECK(d.hasOption(CoapOption::PROXY_SCHEME));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::PROXY_SCHEME);
            CHECK(std::string(it.data(), it.size()) == "i");
            // Size1 (uint)
            CHECK(d.hasOption(CoapOption::SIZE1));
            CHECK(it.next());
            CHECK(it.option() == CoapOption::SIZE1);
            CHECK(it.toUInt() == 5);
            CHECK(it.next() == false);
            CHECK(it == CoapOptionIterator());
        }
    }
    SECTION("decodes payload data correctly") {
        auto d = makeDecoder();
        SECTION("empty") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64", 11);
            CHECK(d.decode(buf.data(), buf.size()) == 11);
            CHECK(d.hasPayload() == false);
            CHECK(d.payloadSize() == 0);
            CHECK(d.payload() == nullptr);
        }
        SECTION("trailing payload marker") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64\xff", 12);
            CHECK(d.decode(buf.data(), buf.size()) == 12);
            CHECK(d.hasPayload() == false);
            CHECK(d.payloadSize() == 0);
            CHECK(d.payload() == nullptr);
        }
        SECTION("1000 bytes") {
            auto s = std::string(1000, 'x');
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x14\x61\x62\x63\x64\xff", 12) + s;
            CHECK(d.decode(buf.data(), buf.size()) == 1012);
            CHECK(d.hasPayload() == true);
            CHECK(std::string(d.payload(), d.payloadSize()) == s);
        }
    }
    SECTION("fails if message data ends unexpectedly") {
        auto d = makeDecoder();
        SECTION("empty buffer") {
            CHECK(d.decode(nullptr, 0) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("incomplete header") {
            auto buf = std::string("\x60\x00\x00", 3);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("missing token") {
            auto buf = std::string("\x41\x45\x04\xd2", 4);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("incomplete token") {
            auto buf = std::string("\x48\x45\x04\xd2\x01\x02\x03\x04", 8);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("missing extended option number") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xd0", 7);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("incomplete extended option number") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\xe0\x00", 8);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("missing extended option length") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1d", 7);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("incomplete extended option length") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1e\xff", 8);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("missing option value") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1c", 7);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
        SECTION("incomplete option value") {
            auto buf = std::string("\x42\x45\x04\xd2\xaa\xbb\x1c\x61\x62\x63", 10);
            CHECK(d.decode(buf.data(), buf.size()) == SYSTEM_ERROR_NOT_ENOUGH_DATA);
        }
    }
}
