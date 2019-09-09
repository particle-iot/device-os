

#include "application.h"
#include "unit-test/unit-test.h"

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
    TCPClient client;
    assertTrue(client.connect(IPAddress(8,8,8,8), 53));
    assertTrue(client.connected());
    client.stop();
    assertFalse(client.connected());
}

test(TCP_04_tcp_client_success_connect_valid_fqdn)
{
    TCPClient client;
    assertTrue(client.connect("www.httpbin.org", 80));
    assertTrue(client.connected());
    client.stop();
    assertFalse(client.connected());
}
