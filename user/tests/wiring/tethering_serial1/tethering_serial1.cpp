/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses>
 */

#include "application.h"
#include "iperf/iperf.h"
#include "unit-test/unit-test.h"
#include "iperf3_servers.h"
#include "test_suite.h"

namespace {
particle::IperfServer iperf;

String runDeviceIperf(bool udp, bool reverse, int64_t bitrate) {
    // Gate on absolute bytes moved, NOT rate: a collapse delivers essentially
    // nothing (~a few KB / a handful of packets) no matter the link speed, while
    // even a very slow but working link moves far more over a 20s run. Using a
    // rate floor would wrongly flag a genuinely slow link as a collapse.
    const int64_t MIN_BYTES = 32 * 1024; // ~a slow link still clears this; a collapse won't
    String best;
    for (size_t i = 0; i < IPERF3_PUBLIC_SERVERS_COUNT; i++) {
        String json;
        particle::IperfClient client;
        if (udp) {
            client.udp();
        }
        if (reverse) {
            client.reverse();
        }
        if (bitrate) {
            client.bitrate(bitrate);
        }
        client.time(20).jsonOutput(true).quiet()
            .network(TestSuite::instance()->network())
            .onResults([&json](const char* j) { json = j ? j : ""; });
        LOG(INFO, "iperf: trying server %s", IPERF3_PUBLIC_SERVERS[i].host);
        // run()'s return code is advisory only: the control connection often dies
        // at the final results exchange (it sits idle on the cellular link for the
        // whole test and gets dropped), making run() return an error even though
        // the data phase completed fine and the JSON contains the real byte
        // counts. Judge the attempt by the bytes in the JSON (MIN_BYTES below),
        // not by the control channel's fate.
        int r = client.run(IPERF3_PUBLIC_SERVERS[i].host, IPERF3_PUBLIC_SERVERS[i].port);
        if (json.length() == 0) {
            LOG(WARN, "iperf: no JSON output (r=%d) on %s, trying next server", r, IPERF3_PUBLIC_SERVERS[i].host);
            continue;
        }
        best = json;
        // Device-side bytes moved: received (reverse) or sent (forward).
        auto v = particle::Variant::fromJSON(json.c_str());
        int64_t bytes = v.get("end").get(reverse ? "sum_received" : "sum_sent").get("bytes").toInt64();
        LOG(INFO, "iperf: device-side %s %lld bytes on %s", reverse ? "rx" : "tx",
                (long long)bytes, IPERF3_PUBLIC_SERVERS[i].host);
        if (bytes >= MIN_BYTES) {
            return json;
        }
        LOG(WARN, "iperf: collapsed (%lld bytes) on %s, trying next server", (long long)bytes,
                IPERF3_PUBLIC_SERVERS[i].host);
    }
    return best;
}
}

test(01_TETHERING_SERIAL1_setup_and_bind) {
    // M-HAT: enable channel 2 of the UART switch to route Serial1 (TX/RX/CTS/RTS)
    // through to the Particle Debugger USB-serial bridge.
    pinMode(A0, OUTPUT);
    digitalWrite(A0, HIGH);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 60000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    Tether.bind(TetherSerialConfig().baudrate(921600).serial(Serial1));
    Tether.on();
    Tether.connect();

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    // Publish modem info to mailbox so the host-side fixture can look up
    // the correct download speed budget per modem variant.
    CellularSignal sig = Cellular.RSSI();
    int modemBaud = 0;
    Cellular.command(+[](int type, const char* buf, int len, int* baud) -> int {
        if (type == TYPE_PLUS && baud) {
            sscanf(buf, "\r\n+IPR: %d", baud);
        }
        return WAIT;
    }, &modemBaud, 10000, "AT+IPR?\r\n");
    Variant info;
    auto ncps = System.hardwareInfo().ncp();
    info.set("ncpId", ncps.isEmpty() ? 0 : (int)ncps[0]);
    info.set("rat", (int)sig.getAccessTechnology());
    info.set("strength", sig.getStrength());
    info.set("quality", sig.getQuality());
    info.set("modemBaud", modemBaud);
    assertEqual(0, pushMailboxMsg(info.toJSON(), 5000));
}

test(07_TETHERING_SERIAL1_start_iperf_server) {
    // JSON results are collected and reported. On Gen 3 (nRF52840) the iperf port
    // is built with IPERF_JSON_NO_INTERVALS (iperf_config.h) so the server keeps
    // only the totals (json_end) instead of accumulating a cJSON object per
    // interval, which would exhaust the ~22 KB heap in ~10 s and collapse the run.
    iperf.quiet().jsonOutput(true).start();
}

test(08_TETHERING_SERIAL1_iperf3_udp_sustain_down) {
}

test(09_TETHERING_SERIAL1_iperf3_udp_sustain_down_small) {
}

test(10_TETHERING_SERIAL1_iperf3_udp_sustain_up) {
}

test(11_TETHERING_SERIAL1_iperf3_udp_sustain_up_small) {
}

test(12_TETHERING_SERIAL1_iperf3_tcp_down) {
}

test(13_TETHERING_SERIAL1_iperf3_tcp_up) {
}

test(14_TETHERING_SERIAL1_stop_iperf_server) {
    iperf.stop();
}

test(15_TETHERING_SERIAL1_download_speeds) {
}

test(16_TETHERING_SERIAL1_upload_speeds) {
}

test(17_TETHERING_SERIAL1_iperf3_udp_full_path_down) {
    LOG(INFO, "Device iperf3 UDP full-path down starting");
    String json = runDeviceIperf(/*udp*/true, /*reverse*/true, /*bitrate*/1000000);
    if (json.length() == 0) {
        json = "{}";
    }
    pushMailboxMsg(json, 10000);
}

test(18_TETHERING_SERIAL1_iperf3_udp_full_path_up) {
    LOG(INFO, "Device iperf3 UDP full-path up starting");
    String json = runDeviceIperf(/*udp*/true, /*reverse*/false, /*bitrate*/1000000);
    if (json.length() == 0) {
        json = "{}";
    }
    pushMailboxMsg(json, 10000);
}

test(19_TETHERING_SERIAL1_iperf3_tcp_full_path_down) {
    LOG(INFO, "Device iperf3 TCP full-path down starting");
    String json = runDeviceIperf(/*udp*/false, /*reverse*/true, /*bitrate*/0);
    if (json.length() == 0) {
        json = "{}";
    }
    pushMailboxMsg(json, 10000);
}

test(20_TETHERING_SERIAL1_iperf3_tcp_full_path_up) {
    LOG(INFO, "Device iperf3 TCP full-path up starting");
    String json = runDeviceIperf(/*udp*/false, /*reverse*/false, /*bitrate*/0);
    if (json.length() == 0) {
        json = "{}";
    }
    pushMailboxMsg(json, 10000);
}

test(21_TETHERING_SERIAL1_final_report) {
    // Host-side only: the JS fixture aggregates the results and runs the
    // cross-test sanity checks.
}
