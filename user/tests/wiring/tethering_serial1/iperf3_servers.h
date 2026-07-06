#pragma once

#include <stddef.h>

// iperf3 public server list for tethering tests.
// Format per line: {"host", port},
// Keep sorted by preference (best/most reliable first).

struct Iperf3Server {
    const char* host;
    int port;
};

static constexpr Iperf3Server IPERF3_PUBLIC_SERVERS[] = {
    {"speedtest.nocix.net", 5205},
    {"lax.speedtest.is.cc", 5203},
    {"speedtest.nocix.net", 5204},
    {"spd-uswb.hostkey.com", 5201},
    {"speedtest.nocix.net", 5203},
    {"nyc.speedtest.is.cc", 5202},
    {"speedtest.nocix.net", 5202},
    {"nyc.speedtest.is.cc", 5203},
    {"speedtest.xmission.com", 5209},
    {"lax.speedtest.is.cc", 5202},
    {"speedtest.nocix.net", 5201},
    {"speedtest.xmission.com", 5201},
    {"speedtest.xmission.com", 5202},
};
static constexpr size_t IPERF3_PUBLIC_SERVERS_COUNT = sizeof(IPERF3_PUBLIC_SERVERS) / sizeof(IPERF3_PUBLIC_SERVERS[0]);