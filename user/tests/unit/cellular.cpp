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

#undef WARN
#undef INFO
#include "catch.hpp"

using namespace detail;

SCENARIO("IMSI range should default to Telefonica as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider(NULL) == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("")   == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("123456789012345") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("2040")  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("31041") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("2140")  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("0404")  == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("10410") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("1407")  == CELLULAR_NETPROV_TELEFONICA);
}

SCENARIO("IMSI range should set Kore Vodafone as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider("204040000000000") == CELLULAR_NETPROV_KORE_VODAFONE);
    REQUIRE(_cellular_imsi_to_network_provider("204045555555555") == CELLULAR_NETPROV_KORE_VODAFONE);
    REQUIRE(_cellular_imsi_to_network_provider("204049999999999") == CELLULAR_NETPROV_KORE_VODAFONE);
}

SCENARIO("IMSI range should set Kore AT&T as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider("310410000000000") == CELLULAR_NETPROV_KORE_ATT);
    REQUIRE(_cellular_imsi_to_network_provider("310410555555555") == CELLULAR_NETPROV_KORE_ATT);
    REQUIRE(_cellular_imsi_to_network_provider("310410999999999") == CELLULAR_NETPROV_KORE_ATT);
}

SCENARIO("IMSI range should set Telefonica as Network Provider", "[cellular]") {
    REQUIRE(_cellular_imsi_to_network_provider("214070000000000") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("214075555555555") == CELLULAR_NETPROV_TELEFONICA);
    REQUIRE(_cellular_imsi_to_network_provider("214079999999999") == CELLULAR_NETPROV_TELEFONICA);
}

TEST_CASE("cellular_signal()") {
    SECTION("failure to query the modem") {
        cellular_signal_t sig = {0};
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

    SECTION("GSM") {
        NetStatus status;
        memset(&status, 0, sizeof(status));
        status.act = ACT_GSM;

        SECTION("error values reported by the modem") {
            status.rxlev = 99;
            status.rxqual = 99;

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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
        }
    }

    SECTION("EDGE") {
        NetStatus status;
        memset(&status, 0, sizeof(status));
        status.act = ACT_EDGE;

        SECTION("error values reported by the modem") {
            status.rxlev = 99;
            status.rxqual = 99;

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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

            cellular_signal_t sig = {0};
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
        NetStatus status;
        memset(&status, 0, sizeof(status));
        status.act = ACT_UTRAN;

        SECTION("error values reported by the modem") {
            status.rscp = 255;
            status.ecno = 255;

            cellular_signal_t sig = {0};
            sig.size = sizeof(sig);
            REQUIRE(cellular_signal_impl(nullptr, &sig, true, status) == SYSTEM_ERROR_NONE);
            REQUIRE(sig.rat == NET_ACCESS_TECHNOLOGY_UMTS);
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
            status.rscp = -5;
            status.ecno = 0;

            cellular_signal_t sig = {0};
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
            status.rscp = 43;
            status.ecno = 24;

            cellular_signal_t sig = {0};
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
            status.rscp = 91;
            status.ecno = 49;

            cellular_signal_t sig = {0};
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
}
