#include "application.h"
#include "unit-test/unit-test.h"

#include "inet_hal.h"

const uint32_t spark_io = (62u<<24) | (116u << 16) | (130u<<8) | 8;
const uint32_t device_spark_io = (54u<<24) | (208u << 16) | (229u<<8) | 4u;
const char* spark_io_str = "spark.io";

test(WLAN_Test1_Lookup_IP_From_Hostname)
{
    char hostname[25];
    strcpy(hostname, spark_io_str);
    uint32_t ip;
    int result = inet_gethostbyname(hostname, 25, &ip);
    assertEqual(ip, spark_io);
    assertMoreOrEqual(result, 0); // cc3000 returns >=0 on success
}

test(WLAN_Test2_Ping_To_Symmetrical_Address)
{
    WiFi.connect();
    const int tries = 5;
    int count = WiFi.ping(IPAddress(8,8,8,8), tries);
    assertEqual(count, tries);
}

test(WLAN_Test3_Ping_To_Asymmetrical_Address)
{
    WiFi.connect();
    const int tries = 5;
    int count = WiFi.ping(IPAddress(62, 116, 130, 8), tries);
    assertEqual(count, tries);
}

// todo - duplicated test, determine if this is necessary... ping by hostname doesn't appear to be supported
#if 0
test(WLAN_Test4_Ping_By_Hostname)
{
    WiFi.connect();
    const int tries = 5;
    int count = WiFi.ping(IPAddress(62, 116, 130, 8), tries);
    assertEqual(count, tries);
}
#endif

// todo - this doesn't work, it works fine cross compiled in the unit tests suite
#if 0
test(IPAddress_Construct_From_Uint32)
{
    IPAddress ip(device_spark_io);

    assertEqual(ip[3], 4);
    assertEqual(ip[2], 229);
    assertEqual(ip[1], 208);
    assertEqual(ip[0], 54);
}
#endif

test(WLAN_Test5_IPAddress_Construct_From_Octets)
{
    IPAddress ip(54, 208, 229, 4);

    assertEqual(ip[0], 54);
    assertEqual(ip[1], 208);
    assertEqual(ip[2], 229);
    assertEqual(ip[3], 4);
}

test(WLAN_Test6_IPAddress_Construct_Octets_Equal_Uint32)
{
    IPAddress ip(1,2,3,4);

    assertTrue(ip==0x01020304);
}

test(WLAN_Test7_IPAddress_Construct_Uin32_Equal_Octets)
{
    IPAddress ip(0x01020304);

    assertEqual(ip[0], 1);
    assertEqual(ip[1], 2);
    assertEqual(ip[2], 3);
    assertEqual(ip[3], 4);
}

