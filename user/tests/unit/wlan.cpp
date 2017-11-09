#include "hippomocks.h"
#include "spark_wiring_wifi.h"
#include "tools/catch.h"
#include <cmath>
#include "system_error.h"
#include "system_network.h"
#include <limits>

void HAL_NET_notify_connected() {
}
void HAL_NET_notify_dhcp(bool b) {
}

bool network_ready(network_handle_t network, uint32_t param1, void* reserved) {
    return false;
}

namespace {

class WlanHalMocks {
public:
    WlanHalMocks() {
        mocks_.OnCallFunc(wlan_connected_info).Do([&](void* reserved, wlan_connected_info_t* inf, void* reserved1) -> int {
            if (ret_ == SYSTEM_ERROR_NONE) {
                inf->rssi = rssi_ * 100;
                inf->snr = snr_ != std::numeric_limits<int32_t>::min() ? snr_ * 100 : snr_;

                inf->strength = std::min(std::max(2L * (rssi_ + 100), 0L), 100L) * 65535 / 100;
                if (inf->snr != std::numeric_limits<int32_t>::min()) {
                    inf->quality = std::min(std::max((long int)(inf->snr / 100 - 9), 0L), 31L) * 65535 / 31;
                } else {
                    inf->quality = std::numeric_limits<int32_t>::min();
                }
            }
            return ret_;
        });

        mocks_.OnCallFunc(network_ready).Do([&](network_handle_t network, uint32_t param1, void* reserved) -> bool {
            return network_ready_;
        });
    }

    void set(system_error_t ret, int rssi = 0, int snr = 0) {
        ret_ = ret;
        rssi_ = rssi;
        snr_ = snr;
    }

    void networkReady(bool b) {
        network_ready_ = b;
    }

private:
    MockRepository mocks_;
    int rssi_ = 0;
    int snr_ = 0;
    system_error_t ret_ = SYSTEM_ERROR_NONE;
    bool network_ready_ = false;
};

} // anonymous

TEST_CASE("WiFiSignal") {
    spark::WiFiClass WiFi;
    WlanHalMocks mocks;
    SECTION("network_ready() == false") {
        mocks.networkReady(false);
        REQUIRE(network_ready(0, 0, nullptr) == false);
        auto sig = WiFi.RSSI();
        REQUIRE(sig.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_WIFI);
        REQUIRE(sig.getStrength() < 0.0f);
        REQUIRE(sig.getQuality() < 0.0f);
        REQUIRE(sig.getStrengthValue() == 0.0f);
        REQUIRE(sig.getQualityValue() == 0.0f);
        REQUIRE(sig.rssi == 2);
        REQUIRE(sig.qual == 0);
        REQUIRE((int8_t)sig == 2);
    }

    SECTION("error reported by wlan_connected_info()") {
        mocks.networkReady(true);
        mocks.set(SYSTEM_ERROR_UNKNOWN, 0, 0);
        REQUIRE(network_ready(0, 0, nullptr) == true);
        auto sig = WiFi.RSSI();
        REQUIRE(sig.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_WIFI);
        REQUIRE(sig.getStrength() < 0.0f);
        REQUIRE(sig.getQuality() < 0.0f);
        REQUIRE(sig.getStrengthValue() == 0.0f);
        REQUIRE(sig.getQualityValue() == 0.0f);
        REQUIRE(sig.rssi == 2);
        REQUIRE(sig.qual == 0);
        REQUIRE((int8_t)sig == 2);
    }

    SECTION("only rssi is reported") {
        mocks.networkReady(true);
        REQUIRE(network_ready(0, 0, nullptr) == true);
        mocks.set(SYSTEM_ERROR_NONE, -40, std::numeric_limits<int32_t>::min());
        auto sig = WiFi.RSSI();
        REQUIRE(sig.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_WIFI);
        REQUIRE(sig.getStrength() >= 20.0f);
        REQUIRE(sig.getQuality() < 0.0f);
        REQUIRE(sig.getStrengthValue() == -40.0f);
        REQUIRE(sig.getQualityValue() == 0.0f);
        REQUIRE(sig.rssi < 0);
        REQUIRE(sig.qual == 0);
        REQUIRE((int8_t)sig == -40);
    }

    SECTION("wlan_connected_info() reports rssi and snr") {
        mocks.networkReady(true);
        REQUIRE(network_ready(0, 0, nullptr) == true);
        mocks.set(SYSTEM_ERROR_NONE, -40, 10);
        auto sig = WiFi.RSSI();
        REQUIRE(sig.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_WIFI);
        REQUIRE(sig.getStrength() >= 20.0f);
        REQUIRE(sig.getQuality() > 0.0f);
        REQUIRE(sig.getStrengthValue() == -40.0f);
        REQUIRE(sig.getQualityValue() == 10.0f);
        REQUIRE(sig.rssi < 0);
        REQUIRE(sig.qual == 10);
        REQUIRE((int8_t)sig == -40);
    }
}
