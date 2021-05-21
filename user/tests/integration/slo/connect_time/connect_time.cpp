#include "application.h"
#include "test.h"

namespace {

// Number of connection time measurements to make. When changing this parameter, make sure to change
// the number of cloud_connect_time_from_cold_boot_XX and cloud_connect_time_from_warm_boot_XX tests
// and update the spec file accordingly
const unsigned CONNECT_COUNT = 10;

// Delay before resetting the device after a test
const system_tick_t DELAY_AFTER_TEST = 2000;

// Default timeout for waitFor()
const system_tick_t WAIT_FOR_TIMEOUT = 10 * 60 * 1000;

struct Stats {
    system_tick_t cloudConnectTimeFromColdBoot[CONNECT_COUNT];
    system_tick_t cloudConnectTimeFromWarmBoot[CONNECT_COUNT];
    system_tick_t networkColdConnectDuration[CONNECT_COUNT];
    system_tick_t networkWarmConnectDuration[CONNECT_COUNT];
    system_tick_t cloudFullHandshakeDuration[CONNECT_COUNT];
    system_tick_t cloudSessionResumeDuration[CONNECT_COUNT];
    unsigned coldBootCount;
    unsigned warmBootCount;
    bool excludeFromSloValidation;
};

retained Stats stats = {};

system_tick_t globalInitTimeFromStartup = 0;
system_tick_t setupTimeFromStartup = 0;
system_tick_t loopTimeFromStartup = 0;
bool loopCalled = false;

system_tick_t testAppInitDuration = 0;
system_tick_t testAppSetupDuration = 0;

inline NetworkClass& network() {
#if Wiring_Cellular
    return Cellular;
#elif Wiring_WiFi
    return WiFi;
#else
#error "Unsupported platform"
#endif
}

bool testCloudConnectTimeFromColdBoot() {
    if (stats.coldBootCount >= CONNECT_COUNT) {
        return false;
    }
    network().off(); // Make sure the network interface is tuned off
    waitFor(network().isOff, WAIT_FOR_TIMEOUT);
    if (!network().isOff()) {
        return false;
    }
    Particle.disconnect(CloudDisconnectOptions().clearSession(true)); // Clear the session data
    const auto t1 = millis();
    network().connect();
    waitFor(network().ready, WAIT_FOR_TIMEOUT);
    if (!network().ready()) {
        return false;
    }
    const auto t2 = millis();
    Particle.connect();
    waitFor(Particle.connected, WAIT_FOR_TIMEOUT);
    if (!Particle.connected()) {
        return false;
    }
    const auto t3 = millis();
    const auto n = stats.coldBootCount++;
    stats.cloudConnectTimeFromColdBoot[n] = t3 - t1 + setupTimeFromStartup; // As if Particle.connect() was called in setup()
    stats.networkColdConnectDuration[n] = t2 - t1;
    stats.cloudFullHandshakeDuration[n] = t3 - t2;
    delay(DELAY_AFTER_TEST);
    return true;
}

bool testCloudConnectTimeFromWarmBoot() {
    if (stats.warmBootCount >= CONNECT_COUNT) {
        return false;
    }
    if (network().ready()) {
        return false;
    }
    const auto t1 = millis();
    network().connect();
    waitFor(network().ready, WAIT_FOR_TIMEOUT);
    if (!network().ready()) {
        return false;
    }
    const auto t2 = millis();
    Particle.connect();
    waitFor(Particle.connected, WAIT_FOR_TIMEOUT);
    if (!Particle.connected()) {
        return false;
    }
    const auto t3 = millis();
    const auto n = stats.warmBootCount++;
    stats.cloudConnectTimeFromWarmBoot[n] = t3 - t1 + setupTimeFromStartup;
    stats.networkWarmConnectDuration[n] = t2 - t1;
    stats.cloudSessionResumeDuration[n] = t3 - t2;
    delay(DELAY_AFTER_TEST);
    return true;
}

template<typename T>
void serializeArrayAsJson(JSONBufferWriter* writer, const T* values, size_t size) {
    writer->beginArray();
    for (size_t i = 0; i < size; ++i) {
        writer->value(values[i]);
    }
    writer->endArray();
}

size_t serializeStatsAsJson(char* buf, size_t size) {
    JSONBufferWriter w(buf, size);
    w.beginObject();
    w.name("cloud_connect_time_from_cold_boot");
    serializeArrayAsJson(&w, stats.cloudConnectTimeFromColdBoot, CONNECT_COUNT);
    w.name("cloud_connect_time_from_warm_boot");
    serializeArrayAsJson(&w, stats.cloudConnectTimeFromWarmBoot, CONNECT_COUNT);
    w.name("network_cold_connect_duration");
    serializeArrayAsJson(&w, stats.networkColdConnectDuration, CONNECT_COUNT);
    w.name("network_warm_connect_duration");
    serializeArrayAsJson(&w, stats.networkWarmConnectDuration, CONNECT_COUNT);
    w.name("cloud_full_handshake_duration");
    serializeArrayAsJson(&w, stats.cloudFullHandshakeDuration, CONNECT_COUNT);
    w.name("cloud_session_resume_duration");
    serializeArrayAsJson(&w, stats.cloudSessionResumeDuration, CONNECT_COUNT);
    if (stats.excludeFromSloValidation) {
        w.name("exclude_from_slo_validation").value(true);
    }
    w.endObject();
    return w.dataSize();
}

} // namespace

STARTUP({
    globalInitTimeFromStartup = millis();
    testAppInit();
    testAppInitDuration = millis() - globalInitTimeFromStartup;
});

void setup() {
    setupTimeFromStartup = millis();
    testAppSetup();
    testAppSetupDuration = millis() - setupTimeFromStartup;
    setupTimeFromStartup -= testAppInitDuration;
}

void loop() {
    if (!loopCalled) {
        loopTimeFromStartup = millis() - testAppInitDuration - testAppSetupDuration;
        loopCalled = true;
    }
    testAppLoop();
}

// TODO: The test runner doesn't support resetting the device in a loop from within a test
#define DEFINE_COLD_BOOT_TEST(_suffix) \
    test(cloud_connect_time_from_cold_boot_##_suffix) { \
        assertTrue(testCloudConnectTimeFromColdBoot()); \
    }

#define DEFINE_WARM_BOOT_TEST(_suffix) \
    test(cloud_connect_time_from_warm_boot_##_suffix) { \
        assertTrue(testCloudConnectTimeFromWarmBoot()); \
    }

DEFINE_COLD_BOOT_TEST(01)
DEFINE_COLD_BOOT_TEST(02)
DEFINE_COLD_BOOT_TEST(03)
DEFINE_COLD_BOOT_TEST(04)
DEFINE_COLD_BOOT_TEST(05)
DEFINE_COLD_BOOT_TEST(06)
DEFINE_COLD_BOOT_TEST(07)
DEFINE_COLD_BOOT_TEST(08)
DEFINE_COLD_BOOT_TEST(09)
DEFINE_COLD_BOOT_TEST(10)

DEFINE_WARM_BOOT_TEST(01)
DEFINE_WARM_BOOT_TEST(02)
DEFINE_WARM_BOOT_TEST(03)
DEFINE_WARM_BOOT_TEST(04)
DEFINE_WARM_BOOT_TEST(05)
DEFINE_WARM_BOOT_TEST(06)
DEFINE_WARM_BOOT_TEST(07)
DEFINE_WARM_BOOT_TEST(08)
DEFINE_WARM_BOOT_TEST(09)
DEFINE_WARM_BOOT_TEST(10)

test(publish_and_validate_stats) {
    Particle.connect();
    waitFor(Particle.connected, WAIT_FOR_TIMEOUT);
    assertTrue(Particle.connected());
#if Wiring_Cellular
    // Exclude 2G and non-production devices from the SLO validation
    CellularDevice devInfo = {};
    devInfo.size = sizeof(devInfo);
    assertEqual(cellular_device_info(&devInfo, nullptr), 0);
    if (devInfo.dev == DEV_SARA_G350 || devInfo.dev == DEV_QUECTEL_EG91_NA) {
        stats.excludeFromSloValidation = true;
    }
#endif // Wiring_Cellular
    const size_t n = serializeStatsAsJson(nullptr /* buf */, 0 /* size */);
    std::unique_ptr<char[]> buf(new(std::nothrow) char[n + 1]); // Including term. null
    assertTrue((bool)buf);
    serializeStatsAsJson(buf.get(), n);
    buf[n] = '\0';
    Particle.publish("stats", buf.get());
}
