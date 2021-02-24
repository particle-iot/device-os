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

#include "cellular_internal.h"
#include "cellular_hal_utilities.h"
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

using namespace particle::detail;
using namespace Catch::Matchers;

TEST_CASE("Gen 2 cellular credentials") {
    SECTION("IMSI range should default to Telefonica as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl(nullptr, nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("", nullptr)   == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("123456789012345", nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("2040", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("31041", nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("2140", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("0404", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("10410", nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("1407", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
    }

    SECTION("IMSI range should set Kore Vodafone as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl("204040000000000", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
        REQUIRE(cellular_sim_to_network_provider_impl("204045555555555", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
        REQUIRE(cellular_sim_to_network_provider_impl("204049999999999", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
    }

    SECTION("IMSI range should set Kore AT&T as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl("310410000000000", nullptr) == CELLULAR_NETPROV_KORE_ATT);
        REQUIRE(cellular_sim_to_network_provider_impl("310410555555555", nullptr) == CELLULAR_NETPROV_KORE_ATT);
        REQUIRE(cellular_sim_to_network_provider_impl("310410999999999", nullptr) == CELLULAR_NETPROV_KORE_ATT);
    }

    SECTION("IMSI range2 should set Kore AT&T as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl("310030000000000", nullptr) == CELLULAR_NETPROV_KORE_ATT);
        REQUIRE(cellular_sim_to_network_provider_impl("310030555555555", nullptr) == CELLULAR_NETPROV_KORE_ATT);
        REQUIRE(cellular_sim_to_network_provider_impl("310030999999999", nullptr) == CELLULAR_NETPROV_KORE_ATT);
    }

    SECTION("IMSI range should set Telefonica as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl("214070000000000", nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("214075555555555", nullptr) == CELLULAR_NETPROV_TELEFONICA);
        REQUIRE(cellular_sim_to_network_provider_impl("214079999999999", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    }

    SECTION("ICCID range should set Twilio as Network Provider", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl(nullptr, "89883235555555555555") == CELLULAR_NETPROV_TWILIO);
        REQUIRE(cellular_sim_to_network_provider_impl(nullptr, "89883071111111111111") == CELLULAR_NETPROV_TWILIO);
        REQUIRE(cellular_sim_to_network_provider_impl(nullptr, "89883231234567891011") == CELLULAR_NETPROV_TWILIO);
    }

    SECTION("ICCID range should set Twilio as Network Provider with a random IMSI set", "[cellular]") {
        REQUIRE(cellular_sim_to_network_provider_impl("732123200003364", "89883234500011906351") == CELLULAR_NETPROV_TWILIO);   // Twilio IMSI and Twilio ICCID
        REQUIRE(cellular_sim_to_network_provider_impl("214070000000000", "89883235555555555555") == CELLULAR_NETPROV_TWILIO);   // Kore IMSI and Twilio ICCID just in case
        REQUIRE(cellular_sim_to_network_provider_impl("310410999999999", "89883071234567891011") == CELLULAR_NETPROV_TWILIO);   // Kore IMSI and Twilio ICCID just in case
        REQUIRE(cellular_sim_to_network_provider_impl("310030999999999", "89883071234567891011") == CELLULAR_NETPROV_TWILIO);   // Kore IMSI and Twilio ICCID just in case
    }
}

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

TEST_CASE("cellular_signal()") {
    SECTION("failure to query the modem") {
        cellular_signal_t sig = {};
        sig.size = sizeof(sig);
        REQUIRE(cellular_signal_impl(&sig, false, NetStatus()) < SYSTEM_ERROR_NONE);
        REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_NONE);

        SECTION("CellularSignal") {
            CellularSignal cs;
            REQUIRE(cs.fromHalCellularSignal(sig) == true);
            REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_NONE);
            REQUIRE(cs.getStrength() < 0.0f);
            REQUIRE(cs.getStrengthValue() == 0.0f);
            REQUIRE(cs.getQuality() < 0.0f);
            REQUIRE(cs.getQualityValue() == 0.0f);
        }
    }

    SECTION("UNKNOWN") {
        NetStatus status = {};
        status.act = ACT_UNKNOWN;

        SECTION("values reported by ACT_UNKNOWN case") {
            status.rxlev = 10; // seed all values as 10 just to ensure default case is hit
            status.rxqual = 10;
            status.rscp = 10;
            status.ecno = 10;
            status.rsrp = 10;
            status.rsrq = 10;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_UNKNOWN);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_NONE);
            REQUIRE(sig.strength == 0);
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rssi == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.qual == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_NONE);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }

            SECTION("CellularSignal") {
                CellularSignal cs(sig);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_NONE);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }
    }

    SECTION("GSM") {
        NetStatus status = {};
        status.act = ACT_GSM;

        SECTION("error values reported by the modem") {
            status.rxlev = 99;
            status.rxqual = 99;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(sig.strength < 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rssi == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.ber == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("only RXLEV reported by the modem") {
            status.rxlev = 31;
            status.rxqual = 99;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(sig.strength >= 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rssi == -8000);
            REQUIRE(sig.ber == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(cs.getStrength() >= 0.0f);
                REQUIRE(cs.getStrengthValue() == -80.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("minimum RXLEV and RXQUAL") {
            status.rxlev = 0;
            status.rxqual = 0;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(sig.strength == 0);
            // Inverted
            REQUIRE(sig.quality == 65535);
            REQUIRE(sig.rssi == -11100);
            REQUIRE(sig.ber == 14);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(cs.getStrength() == 0.0f);
                REQUIRE(cs.getStrengthValue() == -111.0f);
                REQUIRE(cs.getQuality() == 100.0f);
                REQUIRE(cs.getQualityValue() == 0.14f);
            }
        }

        SECTION("middle RXLEV and RXQUAL") {
            status.rxlev = 8;
            status.rxqual = 4;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(std::abs(sig.strength - 32767) == 0);
            REQUIRE(std::abs(sig.quality - 32767) <= 6553);
            REQUIRE(sig.rssi == -10300);
            REQUIRE(sig.ber == 226);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(std::abs(cs.getStrength()) <= 50.0f);
                REQUIRE(cs.getStrengthValue() == -103.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 10.0f);
                REQUIRE(cs.getQualityValue() == 2.26f);
            }
        }

        SECTION("max RXLEV and RXQUAL") {
            status.rxlev = 63;
            status.rxqual = 7;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(sig.strength == 65535);
            // Inverted
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rssi == -4800);
            REQUIRE(sig.ber == 1810);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -48.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == 18.10f);
            }

            SECTION("CellularSignal") {
                CellularSignal cs(sig);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -48.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == 18.10f);
            }
        }
    }

    SECTION("EDGE") {
        NetStatus status = {};
        status.act = ACT_EDGE;

        SECTION("error values reported by the modem") {
            status.rxlev = 99;
            status.rxqual = 99;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(sig.strength < 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rssi == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.ber == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("only RXLEV reported by the modem") {
            status.rxlev = 31;
            status.rxqual = 99;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(sig.strength >= 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rssi == -8000);
            REQUIRE(sig.ber == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(cs.getStrength() >= 0.0f);
                REQUIRE(cs.getStrengthValue() == -80.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("minimum RXLEV and RXQUAL") {
            status.rxlev = 0;
            status.rxqual = 0;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(sig.strength == 0);
            // Inverted
            REQUIRE(sig.quality == 65535);
            REQUIRE(sig.rssi == -11100);
            REQUIRE(sig.ber == -370);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(cs.getStrength() == 0.0f);
                REQUIRE(cs.getStrengthValue() == -111.0f);
                REQUIRE(cs.getQuality() == 100.0f);
                REQUIRE(cs.getQualityValue() == -3.7f);
            }
        }

        SECTION("middle RXLEV and RXQUAL") {
            status.rxlev = 8;
            status.rxqual = 4;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(std::abs(sig.strength - 32767) == 0);
            REQUIRE(std::abs(sig.quality - 32767) <= 6553);
            REQUIRE(sig.rssi == -10300);
            REQUIRE(sig.ber == -190);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(std::abs(cs.getStrength()) <= 50.0f);
                REQUIRE(cs.getStrengthValue() == -103.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 10.0f);
                REQUIRE(cs.getQualityValue() == -1.9f);
            }
        }

        SECTION("max RXLEV and RXQUAL") {
            status.rxlev = 63;
            status.rxqual = 7;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(sig.strength == 65535);
            // Inverted
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rssi == -4800);
            REQUIRE(sig.ber == -60);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -48.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == -0.6f);
            }
        }
    }

    SECTION("UMTS") {
        NetStatus status = {};
        status.act = ACT_UTRAN;

        SECTION("error values reported by the modem") {
            status.rscp = 255;
            status.ecno = 255;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(sig.strength < 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rscp == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.ecno == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("minimum RSCP and ECNO") {
            status.rscp = 0;
            status.ecno = 0;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(sig.strength == 0);
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rscp == -12100);
            REQUIRE(sig.ecno == -2450);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(cs.getStrength() == 0.0f);
                REQUIRE(cs.getStrengthValue() == -121.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == -24.5f);
            }
        }

        SECTION("poor RSCP and ECNO") {
            status.rscp = 3;
            status.ecno = 3;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(sig.rscp == -11800);
            REQUIRE(sig.ecno == -2300);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(std::abs(cs.getStrength()) <= 25.0f);
                REQUIRE(cs.getStrengthValue() == -118.0f);
                REQUIRE(std::abs(cs.getQuality()) <= 25.0f);
                REQUIRE(cs.getQualityValue() == -23.0f);
            }
        }

        SECTION("middle RSCP and ECNO") {
            status.rscp = 16;
            status.ecno = 28;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(std::abs(sig.strength - 32767) == 0);
            REQUIRE(std::abs(sig.quality - 32767) == 0);
            REQUIRE(sig.rscp == -10500);
            REQUIRE(sig.ecno == -1050);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(std::abs(cs.getStrength()) <= 50.0f);
                REQUIRE(cs.getStrengthValue() == -105.0f);
                REQUIRE(std::abs(cs.getQuality()) <= 50.0f);
                REQUIRE(cs.getQualityValue() == -10.5f);
            }
        }

        SECTION("max RSCP and ECNO") {
            status.rscp = 96;
            status.ecno = 49;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(sig.strength == 65535);
            REQUIRE(sig.quality == 65535);
            REQUIRE(sig.rscp == -2500);
            REQUIRE(sig.ecno == 0);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -25.0f);
                REQUIRE(cs.getQuality() == 100.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }
    }

    SECTION("LTE") {
        NetStatus status = {};

        // Expected { input, output } data table
        struct DataTable { AcT input_act; hal_net_access_tech_t expected_act; };
        auto data = GENERATE( values<DataTable>({
            { AcT::ACT_LTE, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE },
        }));
        status.act = data.input_act;

        SECTION("error values reported by the modem") {
            status.rsrp = 255;
            status.rsrq = 255;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength < 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rsrp == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.rsrq == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("minimum RSRP and RSRQ") {
            status.rsrp = 0;
            status.rsrq = 0;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength == 0);
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rsrp == -14100);
            REQUIRE(sig.rsrq == -2000);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() == 0.0f);
                REQUIRE(cs.getStrengthValue() == -141.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == -20.0f);
            }
        }

        SECTION("middle RSRP and RSRQ") {
            status.rsrp = 46;
            status.rsrq = 20;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(std::abs(sig.strength - 32767) == 0);
            REQUIRE(std::abs(sig.quality - 32767) == 0);
            REQUIRE(sig.rsrp == -9500);
            REQUIRE(sig.rsrq == -1000);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(std::abs(cs.getStrength()) <= 50.0f);
                REQUIRE(cs.getStrengthValue() == -95.0f);
                REQUIRE(std::abs(cs.getQuality()) <= 50.0f);
                REQUIRE(cs.getQualityValue() == -10.0f);
            }
        }

        SECTION("max RSRP and RSRQ") {
            status.rsrp = 97;
            status.rsrq = 34;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength == 65535);
            REQUIRE(sig.quality == 65535);
            REQUIRE(sig.rsrp == -4400);
            REQUIRE(sig.rsrq == -300);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -44.0f);
                REQUIRE(cs.getQuality() == 100.0f);
                REQUIRE(cs.getQualityValue() == -3.0f);
            }
        }
    }

    SECTION("LTE_CAT_M1/LTE_CAT_NB1") {
        NetStatus status = {};

        // Expected { input, output } data table
        struct DataTable { AcT input_act; hal_net_access_tech_t expected_act; };
        auto data = GENERATE( values<DataTable>({
            { AcT::ACT_LTE_CAT_M1, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE_CAT_M1 },
            { AcT::ACT_LTE_CAT_NB1, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1 }
        }));
        status.act = data.input_act;

        SECTION("error values reported by the modem") {
            status.rsrp = 255;
            status.rsrq = 255;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength < 0);
            REQUIRE(sig.quality < 0);
            REQUIRE(sig.rsrp == std::numeric_limits<int32_t>::min());
            REQUIRE(sig.rsrq == std::numeric_limits<int32_t>::min());

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() < 0.0f);
                REQUIRE(cs.getStrengthValue() == 0.0f);
                REQUIRE(cs.getQuality() < 0.0f);
                REQUIRE(cs.getQualityValue() == 0.0f);
            }
        }

        SECTION("minimum RSRP and RSRQ") {
            status.rsrp = 0;
            status.rsrq = 0;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength == 0);
            REQUIRE(sig.quality == 0);
            REQUIRE(sig.rsrp == -14100);
            REQUIRE(sig.rsrq == -2000);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() == 0.0f);
                REQUIRE(cs.getStrengthValue() == -141.0f);
                REQUIRE(cs.getQuality() == 0.0f);
                REQUIRE(cs.getQualityValue() == -20.0f);
            }
        }

        SECTION("poor RSRP and RSRQ") {
            status.rsrp = 11;
            status.rsrq = 2;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.rsrp == -13000);
            REQUIRE(sig.rsrq == -1900);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(std::abs(cs.getStrength()) <= 25.0f);
                REQUIRE(cs.getStrengthValue() == -130.0f);
                REQUIRE(std::abs(cs.getQuality()) <= 25.0f);
                REQUIRE(cs.getQualityValue() == -19.0f);
            }
        }

        SECTION("middle RSRP and RSRQ") {
            status.rsrp = 46;
            status.rsrq = 20;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(std::abs(sig.strength - 32767) == 0);
            REQUIRE(std::abs(sig.quality - 32767) == 0);
            REQUIRE(sig.rsrp == -9500);
            REQUIRE(sig.rsrq == -1000);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(std::abs(cs.getStrength()) <= 50.0f);
                REQUIRE(cs.getStrengthValue() == -95.0f);
                REQUIRE(std::abs(cs.getQuality()) <= 50.0f);
                REQUIRE(cs.getQualityValue() == -10.0f);
            }
        }

        SECTION("max RSRP and RSRQ") {
            status.rsrp = 97;
            status.rsrq = 34;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(&sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength == 65535);
            REQUIRE(sig.quality == 65535);
            REQUIRE(sig.rsrp == -4400);
            REQUIRE(sig.rsrq == -300);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(cs.getStrength() == 100.0f);
                REQUIRE(cs.getStrengthValue() == -44.0f);
                REQUIRE(cs.getQuality() == 100.0f);
                REQUIRE(cs.getQualityValue() == -3.0f);
            }
        }
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