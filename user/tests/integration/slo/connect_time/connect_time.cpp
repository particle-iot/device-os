#include "application.h"
#include "test.h"

#if Wiring_Cellular
#define Network Cellular
#elif Wiring_Wifi
#define Network WiFi
#else
#error "Unsupported platform"
#endif

namespace {

const auto TIMEOUT = 10 * 60 * 1000; // Default timeout for waitFor()

struct State {
    system_tick_t networkColdConnectTimeFromStartup;
    system_tick_t networkWarmConnectTimeFromStartup;
    system_tick_t cloudFullHandshakeDuration;
    system_tick_t cloudSessionResumeDuration;
};

retained State s = {};

system_tick_t globalInitTimeFromStartup = 0;
system_tick_t setupTimeFromStartup = 0;
system_tick_t loopTimeFromStartup = 0;
bool loopCalled = false;

system_tick_t testAppInitDuration = 0;
system_tick_t testAppSetupDuration = 0;

size_t serializeStateAsJson(char* buf, size_t size) {
    JSONBufferWriter w(buf, size);
    w.beginObject();
    w.name("network_cold_connect_time_from_startup").value((unsigned)s.networkColdConnectTimeFromStartup);
    w.name("network_warm_connect_time_from_startup").value((unsigned)s.networkWarmConnectTimeFromStartup);
    w.name("cloud_full_handshake_duration").value((unsigned)s.cloudFullHandshakeDuration);
    w.name("cloud_session_resume_duration").value((unsigned)s.cloudSessionResumeDuration);
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

test(network_connect_cold) {
    Network.off(); // Make sure the network interface is tuned off
    waitFor(Network.isOff, TIMEOUT);
    assertTrue(Network.isOff());
    const auto t1 = millis();
    Network.connect();
    waitFor(Network.ready, TIMEOUT);
    assertTrue(Network.ready());
    s.networkColdConnectTimeFromStartup = millis() - t1 + setupTimeFromStartup; // Assuming that Network.connect() is called in setup()
    // The JS part of this test will now reset the device
}

test(network_connect_warm) {
    assertFalse(Network.ready());
    const auto t1 = millis();
    Network.connect();
    waitFor(Network.ready, TIMEOUT);
    assertTrue(Network.ready());
    s.networkWarmConnectTimeFromStartup = millis() - t1 + setupTimeFromStartup;
}

test(cloud_connect_full_handshake) {
    Particle.disconnect(CloudDisconnectOptions().clearSession(true)); // Make sure the session data is cleared
    waitFor(Particle.disconnected, TIMEOUT);
    assertTrue(Particle.disconnected());
    const auto t1 = millis();
    Particle.connect();
    waitFor(Particle.connected, TIMEOUT);
    assertTrue(Particle.connected());
    s.cloudFullHandshakeDuration = millis() - t1;
}

test(cloud_connect_session_resume) {
    Particle.disconnect(CloudDisconnectOptions().graceful(true));
    waitFor(Particle.disconnected, TIMEOUT);
    assertTrue(Particle.disconnected());
    const auto t1 = millis();
    Particle.connect();
    waitFor(Particle.connected, TIMEOUT);
    assertTrue(Particle.connected());
    s.cloudSessionResumeDuration = millis() - t1;
}

test(publish_and_validate_stats) {
    const size_t n = serializeStateAsJson(nullptr /* buf */, 0 /* size */);
    std::unique_ptr<char[]> buf(new(std::nothrow) char[n + 1]); // Including term. null
    assertTrue((bool)buf);
    serializeStateAsJson(buf.get(), n);
    buf[n] = '\0';
    Particle.publish("stats", buf.get());
}
