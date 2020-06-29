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
#include "system_error.h"
#include "net_hal.h"
#include <limits>
#include "spark_wiring_cellular_printable.h"
#include "appender.h"

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

using namespace detail;

SCENARIO("IMSI range should default to Telefonica as Network Provider", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider(nullptr, nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("", nullptr)   == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("123456789012345", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("2040", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("31041", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("2140", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("0404", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("10410", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("1407", nullptr)  == CELLULAR_NETPROV_TELEFONICA);
}

SCENARIO("IMSI range should set Kore Vodafone as Network Provider", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider("204040000000000", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
    REQUIRE(_cellular_sim_to_network_provider("204045555555555", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
    REQUIRE(_cellular_sim_to_network_provider("204049999999999", nullptr) == CELLULAR_NETPROV_KORE_VODAFONE);
}

SCENARIO("IMSI range should set Kore AT&T as Network Provider", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider("310410000000000", nullptr) == CELLULAR_NETPROV_KORE_ATT);
    REQUIRE(_cellular_sim_to_network_provider("310410555555555", nullptr) == CELLULAR_NETPROV_KORE_ATT);
    REQUIRE(_cellular_sim_to_network_provider("310410999999999", nullptr) == CELLULAR_NETPROV_KORE_ATT);
}

SCENARIO("IMSI range should set Telefonica as Network Provider", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider("214070000000000", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("214075555555555", nullptr) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_sim_to_network_provider("214079999999999", nullptr) == CELLULAR_NETPROV_TELEFONICA);
}

SCENARIO("ICCID range should set Twilio as Network Provider", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider(nullptr, "89883235555555555555") == CELLULAR_NETPROV_TWILIO);
    REQUIRE(_cellular_sim_to_network_provider(nullptr, "89883071111111111111") == CELLULAR_NETPROV_TWILIO);
    REQUIRE(_cellular_sim_to_network_provider(nullptr, "89883231234567891011") == CELLULAR_NETPROV_TWILIO);
}

SCENARIO("ICCID range should set Twilio as Network Provider with a random IMSI set", "[cellular]") {
    REQUIRE(_cellular_sim_to_network_provider("732123200003364", "89883234500011906351") == CELLULAR_NETPROV_TWILIO);   // Twilio IMSI and Twilio ICCID
    REQUIRE(_cellular_sim_to_network_provider("214070000000000", "89883235555555555555") == CELLULAR_NETPROV_TWILIO);   // Kore IMSI and Twilio ICCID just in case
    REQUIRE(_cellular_sim_to_network_provider("310410999999999", "89883071234567891011") == CELLULAR_NETPROV_TWILIO);   // Kore IMSI and Twilio ICCID just in case
}

TEST_CASE("cellular_signal()") {
    SECTION("failure to query the modem") {
        cellular_signal_t sig = {};
        sig.size = sizeof(sig);
        REQUIRE(cellular_signal_impl(nullptr, &sig, false, NetStatus()) < SYSTEM_ERROR_NONE);
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

    SECTION("CellularSignalHal RSSI and QUAL") {
        NetStatus status = {};
        status.rssi = -50; // arbitrary non-zero values
        status.qual = 37;  //  "

        SECTION("CellularSignalHal::rssi and CellularSignalHal::qual are set to expected values") {
            CellularSignalHal signal = {};
            REQUIRE(cellular_signal_impl(&signal, nullptr, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(signal.rssi == -50);
            REQUIRE(signal.qual == 37);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_UNKNOWN);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            status.rxlev = 31;
            status.rxqual = 4;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_GSM);
            REQUIRE(std::abs(sig.strength - 32767) <= 655);
            REQUIRE(std::abs(sig.quality - 32767) <= 6553);
            REQUIRE(sig.rssi == -8000);
            REQUIRE(sig.ber == 226);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_GSM);
                REQUIRE(std::abs(cs.getStrength() - 50.0f) <= 1.0f);
                REQUIRE(cs.getStrengthValue() == -80.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 10.0f);
                REQUIRE(cs.getQualityValue() == 2.26f);
            }
        }

        SECTION("max RXLEV and RXQUAL") {
            status.rxlev = 63;
            status.rxqual = 7;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            status.rxlev = 31;
            status.rxqual = 4;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_EDGE);
            REQUIRE(std::abs(sig.strength - 32767) <= 655);
            REQUIRE(std::abs(sig.quality - 32767) <= 6553);
            REQUIRE(sig.rssi == -8000);
            REQUIRE(sig.ber == -190);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_EDGE);
                REQUIRE(std::abs(cs.getStrength() - 50.0f) <= 1.0f);
                REQUIRE(cs.getStrengthValue() == -80.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 10.0f);
                REQUIRE(cs.getQualityValue() == -1.9f);
            }
        }

        SECTION("max RXLEV and RXQUAL") {
            status.rxlev = 63;
            status.rxqual = 7;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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

        SECTION("middle RSCP and ECNO") {
            status.rscp = 48;
            status.ecno = 24;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UTRAN);
            REQUIRE(sig.strength == 32767);
            REQUIRE(std::abs(sig.quality - 32767) <= 655 * 2);
            REQUIRE(sig.rscp == -7300);
            REQUIRE(sig.ecno == -1250);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UTRAN);
                REQUIRE(std::abs(cs.getStrength() - 50.0f) <= 1.0f);
                REQUIRE(cs.getStrengthValue() == -73.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 2.0f);
                REQUIRE(cs.getQualityValue() == -12.5f);
            }
        }

        SECTION("max RSCP and ECNO") {
            status.rscp = 96;
            status.ecno = 49;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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

    SECTION("LTE/LTE_CAT_M1/LTE_CAT_NB1") {
        NetStatus status = {};

        // Expected { input, output } data table
        struct DataTable { AcT input_act; hal_net_access_tech_t expected_act; };
        auto data = GENERATE( values<DataTable>({
            { AcT::ACT_LTE, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE },
            { AcT::ACT_LTE_CAT_M1, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE_CAT_M1 },
            { AcT::ACT_LTE_CAT_NB1, hal_net_access_tech_t::NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1 }
        }));
        status.act = data.input_act;

        SECTION("error values reported by the modem") {
            status.rsrp = 255;
            status.rsrq = 255;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
            status.rsrp = 48;
            status.rsrq = 17;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == data.expected_act);
            REQUIRE(sig.strength == 32429);
            REQUIRE(std::abs(sig.quality - 32767) <= 655 * 2);
            REQUIRE(sig.rsrp == -9300);
            REQUIRE(sig.rsrq == -1150);

            SECTION("CellularSignal") {
                CellularSignal cs;
                REQUIRE(cs.fromHalCellularSignal(sig) == true);
                REQUIRE(cs.getAccessTechnology() == data.expected_act);
                REQUIRE(std::abs(cs.getStrength() - 50.0f) <= 1.0f);
                REQUIRE(cs.getStrengthValue() == -93.0f);
                REQUIRE(std::abs(cs.getQuality() - 50.0f) <= 2.0f);
                REQUIRE(cs.getQualityValue() == -11.5f);
            }
        }

        SECTION("max RSRP and RSRQ") {
            status.rsrp = 97;
            status.rsrq = 34;

            cellular_signal_t sig = {};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
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
        cs.rssi = -50;
        cs.qual = 37;

        ser.print(cs);
        // printf("%s", output);
        REQUIRE(strncmp(output, "-50,37", sizeof(output)) == 0);
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
