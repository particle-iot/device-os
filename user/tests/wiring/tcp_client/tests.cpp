#include "application.h"
#include "unit-test/unit-test.h"
#include "util.h"

namespace {

using namespace particle::util;

// Maximum number of simultaneously active connections supported by the system
const unsigned MAX_CONNECTIONS = 10;

} // namespace

test(tcp_client_01_basic_echo) {
    TestClient client;
    assertTrue(client.connect());
    assertTrue(client.echo());
    client.stop();
}

test(tcp_client_02_max_connections) {
    // Establish maximum number of connections
    Vector<TestClient> clients(MAX_CONNECTIONS);
    for (TestClient& client: clients) {
        assertTrue(client.connect());
    }
    // Try to establish one more connection
    TestClient client;
    assertFalse(client.connect());
    Log.info("^^ that's ok");
    // Close one of the connections
    clients.takeLast();
    // Try once again
    assertTrue(client.connect());
}

test(tcp_client_03_long_connections_small_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.serverType(ServerType::ECHO);
    r.minPacketSize(64);
    r.maxPacketSize(512);
    r.concurrentConnections(MAX_CONNECTIONS);
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_04_long_connections_large_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.serverType(ServerType::ECHO);
    r.minPacketSize(2048); // Larger than MSS
    r.maxPacketSize(4096);
    // Limit to 3 connections to reduce heap usage
    r.concurrentConnections(std::min<unsigned>(3, MAX_CONNECTIONS));
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_05_short_connections_small_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.serverType(ServerType::ECHO);
    r.minPacketSize(64);
    r.maxPacketSize(512);
    r.packetsPerClient(3);
    r.concurrentConnections(MAX_CONNECTIONS);
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_06_short_connections_large_packets) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.serverType(ServerType::ECHO);
    r.minPacketSize(2048); // Larger than MSS
    r.maxPacketSize(4096);
    r.packetsPerClient(2);
    // Limit to 3 connections to reduce heap usage
    r.concurrentConnections(std::min<unsigned>(3, MAX_CONNECTIONS));
    r.runDuration(30000);
    assertTrue(r.run());
}

test(tcp_client_07_rx_buffers_stress_test) {
    START_MEMORY_MONITOR();
    TEST_CLIENT_RUNNER(r);
    r.serverType(ServerType::CHARGEN);
    r.packetSize(1024);
    r.packetDelay(0);
    r.concurrentConnections(1);
    r.runDuration(30000);
    assertTrue(r.run());
}
