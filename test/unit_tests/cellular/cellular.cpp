/**
 ******************************************************************************
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "system_error.h"
#include "net_hal.h"
#include <limits>
#include "spark_wiring_cellular_printable.h"
#include "appender.h"
#include "ncp/cellular/network_config_db.h"
#include "ncp/cellular/cellular_network_manager.h"

#undef WARN
#undef INFO
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

namespace {

using namespace particle;

class FakeStream: public Print {
private:
    BufferAppender append_;

protected:
    virtual size_t write(uint8_t c) override {
        return (size_t)append_.append(&c, 1);
    }

public:
    FakeStream(char* buf, size_t size) :
            append_((uint8_t*)buf, size) {
    }
};

} // namespace

using namespace Catch::Matchers;

TEST_CASE("Gen 3 cellular credentials") {
    SECTION("Blank IMSI defaults to empty apn") {
        const char imsi[] = "";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        CHECK_FALSE(creds.hasApn());
    }
    SECTION("Bad IMSI defaults to empty apn") {
        const char imsi[] = "123456789012345";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        CHECK_FALSE(creds.hasApn());
    }
    SECTION("Bad 4 byte IMSI defaults to empty apn") {
        const char imsi[] = "0404";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        CHECK_FALSE(creds.hasApn());
    }
    SECTION("Twilio with iccid prefix 1") {
        const char iccid[] = "89883234500011906351";
        auto creds = networkConfigForIccid(iccid, sizeof(iccid) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::TWILIO);
    }
    SECTION("Twilio with iccid prefix 2") {
        const char iccid[] = "89883074500011906351";
        auto creds = networkConfigForIccid(iccid, sizeof(iccid) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::TWILIO);
    }
    SECTION("Twilio with iccid prefix 3") {
        const char iccid[] = "89103924500011906351";
        auto creds = networkConfigForIccid(iccid, sizeof(iccid) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::TWILIO);
    }
    SECTION("Telefonica") {
        const char imsi[] = "214075555555555";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::TELEFONICA);
    }
    SECTION("Kore Vodafone") {
        const char imsi[] = "204049999999999";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::KORE_VODAFONE);
    }
    SECTION("Kore ATT") {
        const char imsi[] = "310410000000000";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::KORE_ATT);
    }
    SECTION("Kore ATT2") {
        const char imsi[] = "310030900000000";
        auto creds = networkConfigForImsi(imsi, sizeof(imsi) - 1);
        REQUIRE(creds.netProv() == CellularNetworkProvider::KORE_ATT);
    }

}

TEST_CASE("cellular_printable") {
    char output[32] = {};
    FakeStream ser(output, sizeof(output) - 1);

    SECTION("CellularSignal::printTo") {
        CellularSignal cs;
        cellular_signal_t sig = {0};
        sig.size = sizeof(sig);
        sig.rssi = -9000;
        sig.qual = -1400;
        sig.rat = 7;
        cs.fromHalCellularSignal(sig);

        ser.print(cs);
        // printf("%s", output);
        REQUIRE(strncmp(output, "-90.00,-14.00", sizeof(output)) == 0);
    }

    SECTION("CellularData::printTo") {
        CellularData cd;
        cd.cid = 10;
        cd.tx_session = 20;
        cd.rx_session = 30;
        cd.tx_total = 40;
        cd.rx_total = 50;

        ser.print(cd);
        // printf("%s", output);
        REQUIRE(strncmp(output, "10,20,30,40,50", sizeof(output)) == 0);
    }

    SECTION("CellularBand::printTo") {
        CellularBand cb;
        cb.count = 5;
        cb.band[0] = BAND_700;
        cb.band[1] = BAND_800;
        cb.band[2] = BAND_900;
        cb.band[3] = BAND_2100;
        cb.band[4] = BAND_2600;

        ser.print(cb);
        // printf("%s", output);
        REQUIRE(strncmp(output, "700,800,900,2100,2600", sizeof(output)) == 0);
    }
}