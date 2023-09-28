#include "application.h"
#include "spark_wiring_system_power.h"

extern "C" {
#include "core_portme.h"
}

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    {"app", LOG_LEVEL_ALL},
    {"sys.power", LOG_LEVEL_ALL},
    // {"ncp.rltk", LOG_LEVEL_ALL},
    // {"ncp.at", LOG_LEVEL_ALL},
    // {"hal.ble", LOG_LEVEL_ALL},
    // {"coremark", LOG_LEVEL_ALL}
});

#if PLATFORM_ID == PLATFORM_MSOM

void startupSetPMIC() {
    SystemPowerConfiguration conf;
    conf.feature(SystemPowerFeature::PMIC_DETECTION);
    System.setPowerConfiguration(conf);
}
STARTUP(startupSetPMIC());

PMIC pmic;

void printPMICStatus() {
    Log.info("PMIC config:");
    Log.info("Input current limit: %d", pmic.getInputCurrentLimit());
    Log.info("Input voltage limit: %d", pmic.getInputVoltageLimit());
    Log.info("Charging current: %d", pmic.getChargeCurrentValue());
    Log.info("Power source: %d, status: 0x%02x, fault: 0x%02x", pmic.getSystemStatus()>>6, pmic.getSystemStatus(), pmic.getFault());
}

#endif // PLATFORM_ID == PLATFORM_MSOM

#define WAIT_UNTIL(ms, condition, loop_task) ({                                 \
    system_tick_t micros = HAL_Timer_Get_Micro_Seconds();                       \
    bool res = true;                                                            \
    while (!(condition)) {                                                      \
        system_tick_t dt = (HAL_Timer_Get_Micro_Seconds() - micros);            \
        bool timeout = ((dt > (ms * 1000)) && !(condition));                    \
        if (timeout) {                                                          \
            res = false;                                                        \
            break;                                                              \
        }                                                                       \
        loop_task;                                                              \
    }                                                                           \
    res;                                                                        \
})

namespace {

class GpsLock {
    static Mutex mutex_;
public:
    GpsLock() {
        mutex_.lock();
    }
    ~GpsLock() {
        mutex_.unlock();
    }
};
Mutex GpsLock::mutex_;

static constexpr int UDP_BUF_SIZE = 256;
static int remotePort = 40000;
static int localPort = 8888;
static IPAddress remoteIP;

class UdpEchoTester {
public:
    UdpEchoTester() = default;
    ~UdpEchoTester() = default;

    void start() {
        thread_ = Thread("udp_test",
                     std::bind(&UdpEchoTester::run, this),
                     OS_THREAD_PRIORITY_DEFAULT);
    }

    void run() {
        while (1) {
    #if PLATFORM_ID == PLATFORM_MSOM
            if (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) {
                network_interface_index_ = NETWORK_INTERFACE_CELLULAR;
                network_ = &Cellular;
                Log.info("switch to Cellular");
            } else if (network_interface_index_ == NETWORK_INTERFACE_CELLULAR) {
                network_ = &WiFi;
                network_interface_index_ = NETWORK_INTERFACE_WIFI_STA;
                Log.info("switch to WiFi");
            }

            delay(1000);
            GpsLock lk;
    #endif // PLATFORM_ID == PLATFORM_MSOM
            for (int i = 0; i < UDP_BUF_SIZE; i++) {
                udpTxBuf_[i] = 'a' + random(25);
            }

            if (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) {
                WiFi.setCredentials("PCN", "makeitparticle!");
            }

            Log.info("UDP(%s) test start...",
                (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) ? "WiFi" : "Cellular");
            printPMICStatus();
            // Network on
            network_->on();
            network_->connect();

    #if PLATFORM_ID == PLATFORM_MSOM
            if (network_interface_index_ == NETWORK_INTERFACE_CELLULAR) {
                // Changing coexistence to WWAN preferred
                Cellular.command("AT+QGPSCFG=\"priority\",1");
            }
    #endif

            if (!WAIT_UNTIL(120 * 1000, network_->ready(), spark_process())) {
                Log.info("Network not ready!");
                network_->off();
                continue;
            } else {
                Log.info("Network ready: %s", network_->ready() ? "true" : "false");
            }

            if (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) {
                WiFiSignal signal = WiFi.RSSI();
                Log.info("Wi-Fi signal, strength: %.01f, quality: %.01f",
                    signal.getStrengthValue(), signal.getQualityValue());
            }

            // UDP Test
            udp_.begin(localPort, network_interface_index_);

            // Resolve the remote IP of Particle UDP echo server
            do {
                remoteIP = network_->resolve("publish-receiver-udp.particle.io");
                if (remoteIP.raw().ipv4 == 0) {
                    Log.info("Particle UDP echo server: not resolved yet...");
                } else {
                    Log.info("Particle UDP echo server: %s", remoteIP.toString().c_str());
                    break;
                }
                delay(1000);
            } while (1);

            for (int i = 0; i < 5; i++) {
                udp_.beginPacket(remoteIP, remotePort);
                udp_.write((uint8_t*)udpTxBuf_, UDP_BUF_SIZE);
                udp_.endPacket();
                delay(2000);
                if (udp_.parsePacket() > 0) {
                    memset(udpRxBuf_, 0, sizeof(udpRxBuf_));
                    int count = udp_.read(udpRxBuf_, sizeof(udpRxBuf_));
                    Log.info("UDP(%s) Test OK: rx count: %d, rx buf: %s",
                        (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) ? "WiFi" : "Cellular",
                        count,
                        udpRxBuf_);
                } else {
                    Log.info("UDP(%s) Test FAILED: no packet received!",
                        (network_interface_index_ == NETWORK_INTERFACE_WIFI_STA) ? "WiFi" : "Cellular");
                }
                delay(1000);
            }

            // Network off
            network_->disconnect();
            network_->off();
            delay(1000);
            Log.info("UDP test end");
        }
    }

private:
    NetworkClass* network_ = &WiFi;
    network_interface_index network_interface_index_ = NETWORK_INTERFACE_WIFI_STA;
    Thread thread_;
    UDP udp_;
    char udpTxBuf_[UDP_BUF_SIZE];
    char udpRxBuf_[UDP_BUF_SIZE];
};
UdpEchoTester udpEchoTester;

#if PLATFORM_ID == PLATFORM_MSOM
class GnssTester {
public:
    GnssTester() = default;
    ~GnssTester() = default;

    void start() {
        pinMode(GNSS_ANT_PWR, OUTPUT);
        digitalWrite(GNSS_ANT_PWR, 1);

        thread_ = Thread("gnss_test",
                     std::bind(&GnssTester::run, this),
                     OS_THREAD_PRIORITY_DEFAULT);
    }

private:
    void run() {

        while (1) {
            delay(1000);
            GpsLock lk;
            Log.info("GPS test start...");
            // Changing coexistence to GNSS preferred
            Cellular.on();
            if (!WAIT_UNTIL(60000, Cellular.isOn(), spark_process())) {
                Log.warn("GPS test, Cellular not on!");
                Cellular.off();
                continue;
            }

            Cellular.command("AT+QGPS=0");
            // It is recommended to delay 0.5 second to transmit as it will take about 0.5 second
            // (refer to Chapter 1.1.4) to switch from GNSS to WWAN.
            Cellular.command("AT+QGPSCFG=\"priority\",0");
            delay(500);
            Cellular.command("AT+QGPS=1");
            delay(500);
            for (int i = 0; i < 5; i++) {
                if (Cellular.command(gpsAtHandler, (void*)this, 5000, "AT+QGPSGNMEA=\"GGA\"") == RESP_OK) {
                    busy_ = true;
                    if (!WAIT_UNTIL(5000, !busy_, spark_process())) {
                        Log.warn("GPS test, no GGA received!");
                        busy_ = false;
                    }
                }
                delay(1000);
            }

            Cellular.off();
            Log.info("GPS test end...");
        }
    }

    int gpsAtHandlerImpl(const char* buf, int len) {
        std::string message(buf, len);
        if (message.find("$GPGGA") != std::string::npos) {
            Log.info("GPS Test: %s", buf);
            busy_ = false;
        }

        return 0;
    }

private:
    volatile bool busy_ = false;
    static int gpsAtHandler(int type, const char* buf, int len, void* context) {
        GnssTester* tester = (GnssTester*)context;
        tester->gpsAtHandlerImpl(buf, len);
        return 0;
    }

private:
    Thread thread_;
    int gpsAtlines_;
    int publishCounter_ = 0;
    char dataBuf_[1024];
};
GnssTester GnssTester;

#endif // PLATFORM_ID == PLATFORM_MSOM

class CoremarkTester {
public:
    CoremarkTester() = default;
    ~CoremarkTester() = default;

    void setup() {

    }

    void loop() {
        // Log.info("Coremark Test: running for 10s...");
        // 4500 iterations is about enough to meet the 10 second test duration minimum
        coremark_set_iterations(random(4500, 6000));
        coremark_main();
        // Log.info("Coremark Test: end.");
    }
};
CoremarkTester coremarkTester;

} // anonymous

void platform_init() {
#if PLATFORM_ID == PLATFORM_P2
    // Disable charging
    pinMode(S6, OUTPUT);
    digitalWrite(S6, HIGH);
#else
    pmic.begin();
    pmic.setInputCurrentLimit(3000);
    pmic.setChargeCurrent(2048 + 512 + 256 + 128);
    pmic.getChargeCurrent();
    Log.info("PMIC, input current: %d, charging current: %d, power source: %d",
        pmic.getInputCurrentLimit(), pmic.getChargeCurrentValue(), pmic.getSystemStatus()>>6);
    pmic.disableCharging();
    // pmic.enableCharging();

#endif // PLATFORM_ID == PLATFORM_P2

    // Start BLE advertising, BLE will be automatically restarted after wakeup
    const char* serviceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* rxUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    const char* txUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
    BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, nullptr, nullptr);
    BLE.addCharacteristic(txCharacteristic);
    BLE.addCharacteristic(rxCharacteristic);
    SPARK_ASSERT(BLE.setDeviceName("P2 - Orson") == 0);
    SPARK_ASSERT(BLE.setAdvertisingInterval(0x20) == 0); // 20ms, in units of 0.625ms
    BleAdvertisingData advData;
    advData.appendServiceUUID(serviceUuid);
    SPARK_ASSERT(BLE.advertise(advData) == 0);
}

void setup() {
    Log.printf("\r\n\r\nApp Start! v0.2");
    Log.printf("Change Log: rebase to secure fault\r\n\r\n");

    platform_init();
    coremarkTester.setup();
    udpEchoTester.start();
#if PLATFORM_ID == PLATFORM_MSOM
    GnssTester.start();
#endif // PLATFORM_ID == PLATFORM_MSOM
}

void loop() {
    // Running coremark for 50s and then sleep for 2s
    for (int i = 0; i < 12; i++) {
        coremarkTester.loop();
    }
    Log.info("System sleep for 2s...");
    delay(100);
    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(2s));
}
