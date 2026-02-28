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

#include "system_error.h"
#include "net_hal.h"
#include <limits>
#include "spark_wiring_cellular_printable.h"
#include "appender.h"
#include "ncp/cellular/network_config_db.h"
#include "ncp/cellular/cellular_network_manager.h"
#include "cellular_stubs.h"
// #include "ncp/cellular/cellular_ncp_client.h"
// #include "platform_ncp.h"
#include "ncp_band_mask.h"
#include "ncp_env_var.h" // included just to ensure it doesn't break PLATFORM_GCC

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

TEST_CASE("hexString128toUint64Array") {
    using namespace particle;

    SECTION("All zeros yields zero bands") {
        uint64_t bands[2] = {0xDEAD, 0xBEEF};
        hexString128toUint64Array("00000000000000000000000000000000", bands);
        REQUIRE(bands[0] == 0x0ULL);
        REQUIRE(bands[1] == 0x0ULL);
    }

    SECTION("One zero yields zero bands") {
        uint64_t bands[2] = {0xDEAD, 0xBEEF};
        hexString128toUint64Array("0", bands);
        REQUIRE(bands[0] == 0x0ULL);
        REQUIRE(bands[1] == 0x0ULL);
    }

    SECTION("All 0xFF bytes yields all-ones bands lowercase") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("ffffffffffffffffffffffffffffffff", bands);
        REQUIRE(bands[0] == 0xFFFFFFFFFFFFFFFFULL);
        REQUIRE(bands[1] == 0xFFFFFFFFFFFFFFFFULL);
    }

    SECTION("All 0xFF bytes yields all-ones bands uppercase") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", bands);
        REQUIRE(bands[0] == 0xFFFFFFFFFFFFFFFFULL);
        REQUIRE(bands[1] == 0xFFFFFFFFFFFFFFFFULL);
    }

    SECTION("Value fits entirely in bands[1] (high 64 bits)") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("01020304050607080000000000000000", bands);
        REQUIRE(bands[0] == 0x0000000000000000ULL);
        REQUIRE(bands[1] == 0x0102030405060708ULL);
    }

    SECTION("Value fits entirely in bands[0] (low 64 bits)") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("00000000000000000102030405060708", bands);
        REQUIRE(bands[0] == 0x0102030405060708ULL);
        REQUIRE(bands[1] == 0x0000000000000000ULL);
    }

    SECTION("Short odd-length hex string is right-aligned") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("A", bands);
        REQUIRE(bands[0] == 0xAULL);
        REQUIRE(bands[1] == 0x0ULL);
    }

    SECTION("Short even-length hex string is right-aligned") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("AB", bands);
        REQUIRE(bands[0] == 0xABULL);
        REQUIRE(bands[1] == 0x0ULL);
    }

    SECTION("Full 128-bit hex string with A-F digits in both bands") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("AABBCCDD11223344EEFF998877665500", bands);
        REQUIRE(bands[0] == 0xEEFF998877665500ULL);
        REQUIRE(bands[1] == 0xAABBCCDD11223344ULL);
    }

    SECTION("Odd-length hex string with A-F digits spans both bands") {
        uint64_t bands[2] = {};
        hexString128toUint64Array("ACAFE0123456789ABCDEF", bands);
        REQUIRE(bands[0] == 0x123456789ABCDEFULL);
        REQUIRE(bands[1] == 0xACAFEULL);
    }
}

TEST_CASE("getBandMaskByNcpIdForRAT") {
    using namespace particle;
    using RAT = CellularAccessTechnology;

    SECTION("SARA-R510 CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R510, RAT::LTE_CAT_M1, false, false) == UBLOX_NCP_BANDMASK_1_64_R510);
    }
    SECTION("SARA-R510 CAT-M1 upper bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R510, RAT::LTE_CAT_M1, true, false) == UBLOX_NCP_BANDMASK_65_128_R510);
    }
    SECTION("SARA-R510 CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R510, RAT::LTE_CAT_M1, false, true) == UBLOX_NCP_BANDMASK_1_64_R510);
    }
    SECTION("SARA-R510 CAT-M1 upper bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R510, RAT::LTE_CAT_M1, true, true) == UBLOX_NCP_BANDMASK_65_128_R510);
    }

    SECTION("SARA-R410 CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R410, RAT::LTE_CAT_M1, false, false) == UBLOX_NCP_BANDMASK_1_64_R410);
    }
    SECTION("SARA-R410 CAT-M1 upper bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R410, RAT::LTE_CAT_M1, true, false) == UBLOX_NCP_BANDMASK_65_128_R410);
    }
    SECTION("SARA-R410 CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R410, RAT::LTE_CAT_M1, false, true) == UBLOX_NCP_BANDMASK_1_64_R410_LEGACY);
    }
    SECTION("SARA-R410 CAT-M1 upper bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R410, RAT::LTE_CAT_M1, true, true) == UBLOX_NCP_BANDMASK_65_128_R410);
    }

    SECTION("BG95-M5 CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG95_M5, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95);
    }
    SECTION("BG95-M5 CAT-M1 upper bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG95_M5, RAT::LTE_CAT_M1, true, false) == QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95);
    }
    SECTION("BG95-M5 CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG95_M5, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CATM1_1_64_BG95);
    }
    SECTION("BG95-M5 CAT-M1 upper bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG95_M5, RAT::LTE_CAT_M1, true, true) == QUECTEL_NCP_BANDMASK_CATM1_65_128_BG95);
    }

    SECTION("BG96 CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG96, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CATM1_1_64_BG96_MC);
    }
    SECTION("BG96 CAT-M1 upper bands returns zero (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG96, RAT::LTE_CAT_M1, true, false) == 0);
    }
    SECTION("BG96 CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG96, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CATM1_1_64_BG96_MC);
    }
    SECTION("BG96 CAT-M1 upper bands returns zero (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_BG96, RAT::LTE_CAT_M1, true, true) == 0);
    }

    SECTION("EG91-E CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_E, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX);
    }
    SECTION("EG91-E CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_E, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX);
    }

    SECTION("EG91-EX CAT-M1 lower band (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_EX, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX);
    }
    SECTION("EG91-EX CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_EX, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_E_EX);
    }

    SECTION("EG91-NA CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_NA, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NA);
    }
    SECTION("EG91-NA CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_NA, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NA);
    }

    SECTION("EG91-NAX CAT-M1 lower bands (non-legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_NAX, RAT::LTE_CAT_M1, false, false) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NAX);
    }
    SECTION("EG91-NAX CAT-M1 lower bands (legacy)") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_QUECTEL_EG91_NAX, RAT::LTE_CAT_M1, false, true) == QUECTEL_NCP_BANDMASK_CAT1_1_64_EG91_NAX);
    }

    SECTION("Unknown NCP ID returns zero") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_NONE, RAT::LTE_CAT_M1, false, false) == 0);
    }
    SECTION("Non CAT-M1 RAT returns zero") {
        REQUIRE(getBandMaskByNcpIdForRAT(PLATFORM_NCP_SARA_R510, RAT::LTE, false, false) == 0);
    }
}

TEST_CASE("CellularBandMask") {
    using namespace particle;

    SECTION("Default constructor yields empty mask") {
        CellularBandMask m;
        REQUIRE(m.isEmpty());
        REQUIRE(m.low() == 0);
        REQUIRE(m.high() == 0);
    }

    SECTION("Construct from low-only uint64") {
        CellularBandMask m(0xABCD);
        REQUIRE(m.low() == 0xABCDULL);
        REQUIRE(m.high() == 0);
        CHECK_FALSE(m.isEmpty());
    }

    SECTION("Construct from low and high uint64") {
        CellularBandMask m(0x1234, 0x5678);
        REQUIRE(m.low() == 0x1234ULL);
        REQUIRE(m.high() == 0x5678ULL);
    }

    SECTION("Construct from hex string low only") {
        CellularBandMask m("B0E189F");
        REQUIRE(m.low() == UBLOX_NCP_BANDMASK_1_64_R510);
        REQUIRE(m.high() == 0);
    }

    SECTION("Construct from hex string spanning both bands") {
        CellularBandMask m("AABBCCDD11223344EEFF998877665500");
        REQUIRE(m.low() == 0xEEFF998877665500ULL);
        REQUIRE(m.high() == 0xAABBCCDD11223344ULL);
    }

    SECTION("Construct from null hex string yields empty mask") {
        CellularBandMask m(nullptr);
        REQUIRE(m.isEmpty());
    }

    SECTION("setLow and setHigh") {
        CellularBandMask m;
        m.setLow(0xDEAD);
        m.setHigh(0xBEEF);
        REQUIRE(m.low() == 0xDEADULL);
        REQUIRE(m.high() == 0xBEEFULL);
    }

    SECTION("set both bands") {
        CellularBandMask m;
        m.set(0x1111, 0x2222);
        REQUIRE(m.low() == 0x1111ULL);
        REQUIRE(m.high() == 0x2222ULL);
    }

    SECTION("setFromHexString") {
        CellularBandMask m;
        m.setFromHexString("B0E189F");
        REQUIRE(m.low() == UBLOX_NCP_BANDMASK_1_64_R510);
        REQUIRE(m.high() == 0);
    }

    SECTION("setFromHexString clears previous value") {
        CellularBandMask m(0xFFFFFFFF, 0xFFFFFFFF);
        m.setFromHexString("1");
        REQUIRE(m.low() == 1);
        REQUIRE(m.high() == 0);
    }

    SECTION("setFromDecimalStrings") {
        CellularBandMask m;
        m.setFromDecimalStrings("17701022");
        REQUIRE(m.low() == UBLOX_NCP_BANDMASK_1_64_R410);
        REQUIRE(m.high() == 0);
    }

    SECTION("setFromDecimalStrings") {
        CellularBandMask m;
        m.setFromDecimalStrings("6170");
        REQUIRE(m.low() == UBLOX_NCP_BANDMASK_1_64_R410_LEGACY);
        REQUIRE(m.high() == 0);
    }

    SECTION("setFromDecimalStrings") {
        CellularBandMask m;
        m.setFromDecimalStrings("185473183", "1048578");
        REQUIRE(m.low() == UBLOX_NCP_BANDMASK_1_64_R510);
        REQUIRE(m.high() == UBLOX_NCP_BANDMASK_65_128_R510);
    }

    SECTION("setFromDecimalStrings clears previous value") {
        CellularBandMask m(0xFFFFFFFF, 0xFFFFFFFF);
        m.setFromDecimalStrings("1");
        REQUIRE(m.low() == 1);
        REQUIRE(m.high() == 0);
    }

    SECTION("toString low only omits leading zeros") {
        CellularBandMask m(0xB0E189F);
        CString s = m.toString();
        REQUIRE(strcmp(s, "b0e189f") == 0);
    }

    SECTION("toString with high band includes full 16-char low field") {
        CellularBandMask m(0x1, 0x1);
        CString s = m.toString();
        REQUIRE(strcmp(s, "10000000000000001") == 0);
    }

    SECTION("operator== and operator!=") {
        CellularBandMask a(0x1234, 0x5678);
        CellularBandMask b(0x1234, 0x5678);
        CellularBandMask c(0xABCD, 0x5678);
        REQUIRE(a == b);
        REQUIRE(a != c);
    }

    SECTION("operator| combines bands") {
        CellularBandMask a(0x00FF, 0x0F00);
        CellularBandMask b(0xFF00, 0x00F0);
        CellularBandMask r = a | b;
        REQUIRE(r.low() == 0xFFFFULL);
        REQUIRE(r.high() == 0x0FF0ULL);
    }

    SECTION("operator& masks bands") {
        CellularBandMask a(0xFFFF, 0xFF00);
        CellularBandMask b(0x0F0F, 0xF0F0);
        CellularBandMask r = a & b;
        REQUIRE(r.low() == 0x0F0FULL);
        REQUIRE(r.high() == 0xF000ULL);
    }

    SECTION("operator|= combines in place") {
        CellularBandMask a(0x00FF, 0x0F00);
        CellularBandMask b(0xFF00, 0x00F0);
        a |= b;
        REQUIRE(a.low() == 0xFFFFULL);
        REQUIRE(a.high() == 0x0FF0ULL);
    }

    SECTION("operator&= masks in place") {
        CellularBandMask a(0xFFFF, 0xFF00);
        CellularBandMask b(0x0F0F, 0xF0F0);
        a &= b;
        REQUIRE(a.low() == 0x0F0FULL);
        REQUIRE(a.high() == 0xF000ULL);
    }
}
