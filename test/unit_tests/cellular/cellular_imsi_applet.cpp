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

#include "ncp_client/quectel/imsi_applet.h"

#include "system_error.h"
#include "str_util.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

#undef WARN
#undef INFO
#include "catch2/catch.hpp"

using namespace particle;
using namespace particle::kore;

namespace {

std::string bytes(const std::string& hex) {
    std::string s(hex.size() / 2, '\0');
    s.resize(fromHex(hex.data(), hex.size(), &s[0], s.size()));
    return s;
}

// As documented by Kore, kept apart from the arrays in imsi_applet.cpp so a typo in those shows up
const std::string STATUS_CURRENT = bytes("80C2000008CF06020282814C00");
const std::string STATUS_NEW = bytes("80C2000009CF0702028281B30100");
const std::string FORCE_CURRENT = bytes("80C2000009CF07020282814E0101");
const std::string FORCE_NEW = bytes("80C2000009CF0702028281A001AA");
const std::string AUTOMATIC_CURRENT = bytes("80C200000DCF0B020282814F050190000000");
const std::string AUTOMATIC_NEW = bytes("80C2000009CF0702028281A001EE");
const std::string GET_RESPONSE_9 = bytes("00C0000009");

// The rest of the 9 byte status after the mode byte, nothing looks at it
const std::string STATUS_TAIL = "0102030405060708";

// Answers each command from a table of hex responses, and 9000 with no data to anything it does not
// know, which is what each applet does with the other one's status query
class FakeCard {
public:
    std::map<std::string, std::string> responses;
    std::vector<std::string> sent; // Raw commands
    int error = 0;

    static int send(void* ctx, const char* cmd, size_t cmdSize, char* resp, size_t& respSize) {
        auto self = static_cast<FakeCard*>(ctx);
        const std::string c(cmd, cmdSize);
        self->sent.push_back(c);
        if (self->error) {
            return self->error;
        }
        const auto it = self->responses.find(c);
        const auto data = bytes(it != self->responses.end() ? it->second : "9000");
        REQUIRE(data.size() <= respSize);
        memcpy(resp, data.data(), data.size());
        respSize = data.size();
        return 0;
    }
};

} // namespace

TEST_CASE("Kore IMSI applet status") {
    FakeCard card;
    ImsiApplet applet = ImsiApplet::NEW;
    bool automatic = true;

    SECTION("current applet in automatic mode") {
        // Mode 82, as seen on an EG916Q
        card.responses[STATUS_CURRENT] = "82" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::CURRENT);
        CHECK(automatic);
        // Found on the first query, the new one is never asked
        CHECK(card.sent == std::vector<std::string>{ STATUS_CURRENT });
    }

    SECTION("current applet in Default IMSI mode is not automatic") {
        card.responses[STATUS_CURRENT] = "83" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::CURRENT);
        CHECK_FALSE(automatic);
    }

    SECTION("current applet status without bit 7 is not the current applet") {
        card.responses[STATUS_CURRENT] = "02" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::NONE);
        CHECK(card.sent == std::vector<std::string>{ STATUS_CURRENT, STATUS_NEW });
    }

    SECTION("new applet in automatic mode") {
        card.responses[STATUS_NEW] = "01" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::NEW);
        CHECK(automatic);
        CHECK(card.sent == std::vector<std::string>{ STATUS_CURRENT, STATUS_NEW });
    }

    SECTION("new applet with switching disabled is not automatic") {
        card.responses[STATUS_NEW] = "00" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::NEW);
        CHECK_FALSE(automatic);
    }

    SECTION("status waiting to be fetched is read with GET RESPONSE") {
        card.responses[STATUS_NEW] = "6109";
        card.responses[GET_RESPONSE_9] = "01" + STATUS_TAIL + "9000";
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::NEW);
        CHECK(automatic);
        CHECK(card.sent == std::vector<std::string>{ STATUS_CURRENT, STATUS_NEW, GET_RESPONSE_9 });
    }

    SECTION("no applet") {
        REQUIRE(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == 0);
        CHECK(applet == ImsiApplet::NONE);
        CHECK_FALSE(automatic);
    }

    SECTION("an error sending is passed back") {
        card.error = SYSTEM_ERROR_TIMEOUT;
        CHECK(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == SYSTEM_ERROR_TIMEOUT);
        CHECK(applet == ImsiApplet::NONE);
    }

    SECTION("a response too short for a status word is an error") {
        card.responses[STATUS_CURRENT] = "90";
        CHECK(readImsiAppletStatus(FakeCard::send, &card, applet, automatic) == SYSTEM_ERROR_BAD_DATA);
    }

    SECTION("no sender") {
        CHECK(readImsiAppletStatus(nullptr, &card, applet, automatic) == SYSTEM_ERROR_INVALID_ARGUMENT);
    }
}

TEST_CASE("Kore IMSI applet forced rotation") {
    FakeCard card;

    SECTION("current applet") {
        SECTION("sends the documented command") {
            forceImsiRotation(FakeCard::send, &card, ImsiApplet::CURRENT);
            CHECK(card.sent == std::vector<std::string>{ FORCE_CURRENT });
        }
        SECTION("91xx is the REFRESH waiting, a rotation started") {
            card.responses[FORCE_CURRENT] = "910B";
            CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::CURRENT) == 1);
        }
        SECTION("9000 is the applet answering without doing anything") {
            card.responses[FORCE_CURRENT] = "9000";
            CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::CURRENT) == 0);
        }
    }

    SECTION("new applet") {
        SECTION("sends the documented command") {
            forceImsiRotation(FakeCard::send, &card, ImsiApplet::NEW);
            CHECK(card.sent == std::vector<std::string>{ FORCE_NEW });
        }
        SECTION("9000, 61xx and 91xx all count as taken") {
            for (auto sw: { "9000", "6109", "910B" }) {
                card.responses[FORCE_NEW] = sw;
                CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::NEW) == 1);
            }
        }
        SECTION("anything else did not start one") {
            card.responses[FORCE_NEW] = "6985";
            CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::NEW) == 0);
        }
    }

    SECTION("an error sending is passed back") {
        card.error = SYSTEM_ERROR_TIMEOUT;
        CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::NEW) == SYSTEM_ERROR_TIMEOUT);
    }

    SECTION("no applet") {
        CHECK(forceImsiRotation(FakeCard::send, &card, ImsiApplet::NONE) == SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(card.sent.empty());
    }
}

TEST_CASE("Kore IMSI applet set automatic") {
    FakeCard card;

    SECTION("current applet") {
        SECTION("sends the documented command") {
            setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::CURRENT);
            CHECK(card.sent == std::vector<std::string>{ AUTOMATIC_CURRENT });
        }
        SECTION("9000 is done") {
            card.responses[AUTOMATIC_CURRENT] = "9000";
            CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::CURRENT) == 0);
        }
        SECTION("anything else is an error") {
            card.responses[AUTOMATIC_CURRENT] = "6985";
            CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::CURRENT) == SYSTEM_ERROR_INVALID_STATE);
        }
    }

    SECTION("new applet") {
        SECTION("sends the documented command") {
            setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::NEW);
            CHECK(card.sent == std::vector<std::string>{ AUTOMATIC_NEW });
        }
        SECTION("9000 and 6109 are both done") {
            for (auto sw: { "9000", "6109" }) {
                card.responses[AUTOMATIC_NEW] = sw;
                CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::NEW) == 0);
            }
        }
        SECTION("anything else is an error") {
            card.responses[AUTOMATIC_NEW] = "6985";
            CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::NEW) == SYSTEM_ERROR_INVALID_STATE);
        }
    }

    SECTION("an error sending is passed back") {
        card.error = SYSTEM_ERROR_TIMEOUT;
        CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::CURRENT) == SYSTEM_ERROR_TIMEOUT);
    }

    SECTION("no applet") {
        CHECK(setImsiAppletAutomatic(FakeCard::send, &card, ImsiApplet::NONE) == SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(card.sent.empty());
    }
}
