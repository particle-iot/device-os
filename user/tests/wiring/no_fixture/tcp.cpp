

#include "application.h"
#include "unit-test/unit-test.h"

// issue #1865 - TCPClient connect() return values
// added asserts for TCPClient::connect()

test(TCP_01_tcp_client_failed_connect_invalid_ip)
{
    TCPClient client;
    assertFalse(client.connect(IPAddress(0,0,0,0), 567));
    assertFalse(client.connected());
    client.stop();
}

test(TCP_02_tcp_client_failed_connect_invalid_fqdn)
{
    TCPClient client;
    assertFalse(client.connect("does.not.exist", 567));
    assertFalse(client.connected());
    client.stop();
}

test(TCP_03_tcp_client_success_connect_valid_ip)
{
    IPAddress ip;
    ip = Network.resolve("www.httpbin.org");
    assertTrue(ip > 0);
    TCPClient client;
    assertTrue(client.connect(ip, 80));
    assertTrue(client.connected());
    system_tick_t start = millis();
    client.stop();
    assertTrue(millis() - start < 2000UL); // ch35609 - TCP sockets should close quickly
    assertFalse(client.connected());
}

test(TCP_04_tcp_client_success_connect_valid_fqdn)
{
    TCPClient client;
    assertTrue(client.connect("www.httpbin.org", 80));
    assertTrue(client.connected());
    system_tick_t start = millis();
    client.stop();
    assertTrue(millis() - start < 2000UL); // ch35609 - TCP sockets should close quickly
    assertFalse(client.connected());
}
