

#include "application.h"
#include "unit-test/unit-test.h"

// issue #1865 - TCPClient connect() return values
// added asserts for TCPClient::connect()

namespace {

const auto TCP_RETRY_ATTEMPTS = 10;

} // anonymous

test(TCP_01_tcp_client_failed_connect_invalid_ip)
{
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    TCPClient client;
    assertFalse(client.connect(IPAddress(0,0,0,0), 567));
    assertFalse(client.connected());
    client.stop();
}

test(TCP_02_tcp_client_failed_connect_invalid_fqdn)
{
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    TCPClient client;
    assertFalse(client.connect("does.not.exist", 567));
    assertFalse(client.connected());
    client.stop();
}

test(TCP_03_tcp_client_success_connect_valid_ip)
{
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    IPAddress ip;
    for (int i = 0; i < TCP_RETRY_ATTEMPTS; i++) {
        ip = Network.resolve("www.httpbin.org");
        if (ip > 0) {
            break;
        }
        delay(i * 1000);
    }
    assertTrue(ip > 0);
    TCPClient client;
    int r = 0;
    for (int i = 0; i < TCP_RETRY_ATTEMPTS; i++) {
        r = client.connect(ip, 80);
        if (r && client.connected()) {
            break;
        }
    }
    assertTrue(r);
    assertTrue(client.connected());
    system_tick_t start = millis();
    client.stop();
    assertTrue(millis() - start < 2000UL); // ch35609 - TCP sockets should close quickly
    assertFalse(client.connected());
}

test(TCP_04_tcp_client_success_connect_valid_fqdn)
{
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);

    TCPClient client;
    int r = 0;
    for (int i = 0; i < TCP_RETRY_ATTEMPTS; i++) {
        r = client.connect("www.httpbin.org", 80);
        if (r && client.connected()) {
            break;
        }
    }
    assertTrue(r);
    assertTrue(client.connected());
    system_tick_t start = millis();
    client.stop();
    assertTrue(millis() - start < 2000UL); // ch35609 - TCP sockets should close quickly
    assertFalse(client.connected());
}
