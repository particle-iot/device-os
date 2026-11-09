/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "socket_hal.h"
#include "random.h"
#include "scope_guard.h"

#define MAX_CELLULAR_OFF_TIME (3*60*1000)

#if Wiring_Cellular == 1

/**
 * Returns current modem type:
 * DEV_UNKNOWN, DEV_SARA_G350, DEV_SARA_U260, DEV_SARA_U270, DEV_SARA_U201, DEV_SARA_R410, DEV_SARA_R510
 */
int cellular_modem_type() {
    CellularDevice device = {};
    device.size = sizeof(device);
    cellular_device_info(&device, NULL);

    return device.dev;
}

/* Scenario: The device will connect to the Cloud even when all
 *           TCP socket types are consumed
 *
 * Given the device is currently disconnected from the Cloud
 * When all available TCP sockets are consumed
 * And the device attempts to connect to the Cloud
 * Then the device overcomes this socket obstacle and connects to the Cloud
 */
void disconnect_from_cloud(system_tick_t timeout, bool detach = false)
{
    Particle.disconnect();
    waitFor(Particle.disconnected, timeout);

    Cellular.disconnect();
    // Avoids some sort of race condition in AUTOMATIC mode
    delay(1000);

    if (detach) {
        Cellular.command(timeout, "AT+COPS=2,2\r\n");
    }
}
void connect_to_cloud(system_tick_t timeout)
{
    // Ask for the network by name first
    // Particle.connect() only raises the cloud flag, it will not bring the interface back up
    const system_tick_t start = millis();
    Cellular.connect();
    waitFor(Cellular.ready, timeout);

    Particle.connect();
    // Share one timeout budget across both waits
    const system_tick_t elapsed = millis() - start;
    waitFor(Particle.connected, elapsed < timeout ? timeout - elapsed : 1000);
}

namespace {
void checkIPAddress(const char* name, const IPAddress& address)
{
    if (address.version()==0 || address[0]==0)
    {
        Serial.print("address failed:");
        Serial.println(name);
        assertNotEqual(address.version(), 0);
        assertNotEqual(address[0], 0);
    }
}
} // anonymous

test(CELLULAR_01_local_ip_cellular_config)
{
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    checkIPAddress("local", Cellular.localIP());
}

test(CELLULAR_02_resolve) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    checkIPAddress("www.particle.io", Cellular.resolve("www.particle.io"));
}

test(CELLULAR_03_resolve) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    IPAddress addr = Cellular.resolve("this.is.not.a.real.host");
    assertEqual(addr, 0);
}

test(CELLULAR_04_ping) {
    assertTrue(Cellular.ready());
    IPAddress addr;
    for (int i = 0; i < 5; i++) {
        addr = Cellular.resolve("dns.google");
        if (addr) {
            break;
        }
    }
    assertTrue((bool)addr);
    assertMoreOrEqual(Cellular.ping(addr, 5), 2);
    assertMoreOrEqual(Cellular.ping("dns.google", 5), 2);
}

// Collects cellular signal strength and quality and validates Accesstechnology (RAT)
test(CELLULAR_05_sigstr_is_valid) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());
    CellularSignal s;
    bool values_in_range = false;
    const int num_retries = 10;
    // Checks 10 times with 500ms gap to get valid signal strength
    for (int x = 0; x < num_retries; x++) {
        s = Cellular.RSSI();
        // Verify that strength and quality values are in range for the given AccessTechnology
        switch (s.getAccessTechnology()) {
            case NET_ACCESS_TECHNOLOGY_GSM:     // GSM strength [-111, -48] and quality [0.14%, 18.10%]
                if ((s.getStrengthValue() <= -48.0f && s.getStrengthValue() >= -111.0f)
                    && (s.getQualityValue() <= 18.1f && s.getQualityValue() >= 0.14f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                out->printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_EDGE:     // EDGE strength [-111, -48] and quality [-3.70, -0.60]
                if ((s.getStrengthValue() <= -48.0f && s.getStrengthValue() >= -111.0f)
                    && (s.getQualityValue() <= -0.6f && s.getQualityValue() >= -3.7f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                out->printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_UTRAN:     // UTRAN strength [-121, -25] and quality [-24.5, 0]
                if ((s.getStrengthValue() <= -25.0f && s.getStrengthValue() >= -121.0f)
                    && (s.getQualityValue() <= -0.0f && s.getQualityValue() >= -25.4f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                out->printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            case NET_ACCESS_TECHNOLOGY_LTE:
            case NET_ACCESS_TECHNOLOGY_LTE_CAT_M1:  // LTE CAT-M1 strength [-141, -44] and quality [-20, -3]
            case NET_ACCESS_TECHNOLOGY_LTE_CAT_NB1:
                if ((s.getStrengthValue() <= -44.0f && s.getStrengthValue() >= -141.0f)
                    && (s.getQualityValue() <= -3.0f && s.getQualityValue() >= -20.0f)) {
                        values_in_range = true;
                        x = num_retries;
                        break;
                }
                out->printlnf("StrV: %.2f, QualV: %.2f, RAT: %d", s.getStrengthValue(), s.getQualityValue(), s.getAccessTechnology());
                break;
            default:
                break;
        }
        delay(500);
    }
    assertFalse((s.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_UNKNOWN) || (s.getAccessTechnology() == NET_ACCESS_TECHNOLOGY_NONE));
    assertTrue(values_in_range);
}

test(CELLULAR_06_on_off_validity_check) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Cellular.isOn());
    assertFalse(Cellular.isOff());

    Particle.disconnect();
    waitFor(Particle.disconnected, 30000);
    assertTrue(Cellular.isOn());
    assertFalse(Cellular.isOff());

    Cellular.disconnect();
    waitFor([]{ return !Cellular.ready(); }, 5*60*1000); // accounting for some long reg times?
    assertTrue(Cellular.isOn());
    assertFalse(Cellular.isOff());

    int ret = Cellular.command("AT\r\n");
    assertEqual(ret, (int)RESP_OK);

    Cellular.off();
    waitFor(Cellular.isOff, MAX_CELLULAR_OFF_TIME);
    assertTrue(Cellular.isOff());
    assertFalse(Cellular.isOn());

    ret = Cellular.command("AT\r\n");
    assertNotEqual(ret, (int)RESP_OK);

    Cellular.on();
    waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Cellular.isOn());
    assertFalse(Cellular.isOff());

    ret = Cellular.command("AT\r\n");
    assertEqual(ret, (int)RESP_OK);
}

test(CELLULAR_07_urcs) {

#if HAL_PLATFORM_GEN == 3
    assertEqual(Cellular.command("AT\r\n"), (int)RESP_OK);
    int modem_type = -1;
#if HAL_PLATFORM_NCP
    modem_type = cellular_modem_type();
#endif // HAL_PLATFORM_NCP

    // Ensure we are disconnected, bad things can happen if messages are tx/rx
    // on the cloud connection when the AT command channel is suspended.
    if (Particle.connected()) {
        Particle.disconnect();
        waitFor(Particle.disconnected, 30000);
    }

    delay(100);
    SCOPE_GUARD({
        cellular_urcs(true, nullptr);
        // cellular_urcs() is async and may take a while, delay just in case
        delay(1000);
        connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    });

    assertEqual(cellular_urcs(false, nullptr), (int)SYSTEM_ERROR_NONE);
    // cellular_urcs() is async and may take a while, delay just in case
    delay(1000);

    if (modem_type != DEV_QUECTEL_BG95_S5 && modem_type != DEV_QUECTEL_BG95_M5) {
        assertNotEqual(Cellular.command("AT\r\n"), (int)RESP_OK);
    } else {
        // TODO: Pull in addUrcHandler code from FOTA update PR,
        //       monitor for +CGEV which happens right after AT+CFUN=4
        //       Will need to run AT+CFUN=4, AT+CFUN=1 and look for absense of +CGEV
        //
        // For now we'll test AT commands work
        assertEqual(Cellular.command("AT\r\n"), (int)RESP_OK);
    }

    assertEqual(cellular_urcs(true, nullptr), (int)SYSTEM_ERROR_NONE);
    // cellular_urcs() is async and may take a while, delay just in case
    delay(1000);
    if (modem_type != DEV_QUECTEL_BG95_S5 && modem_type != DEV_QUECTEL_BG95_M5) {
        assertEqual(Cellular.command("AT\r\n"), (int)RESP_OK);
    } else {
        // TODO: Pull in addUrcHandler code from FOTA update PR,
        //       monitor for +CGEV which happens right after AT+CFUN=4
        //       Will need to run AT+CFUN=4, AT+CFUN=1, and look for presense of +CGEV
        //
        // For now we'll test AT commands work
        assertEqual(Cellular.command("AT\r\n"), (int)RESP_OK);
    }
#else
#error "Unsupported platform"
#endif
}

test(MDM_01_socket_writes_with_length_more_than_1023_work_correctly) {

#if HAL_PLATFORM_NCP
    // CH32073
    if (cellular_modem_type() == DEV_SARA_R410) {
        skip();
        return;
    }
#endif // HAL_PLATFORM_NCP

    // https://github.com/spark/firmware/issues/1104

    const int dataSize = 1024;
    const int bufferSize = 2048;
    auto randData = std::make_unique<char[]>(dataSize);
    assertTrue((bool)randData);

    Random rand;
    rand.genBase32(randData.get(), dataSize);

    static const char requestFormat[] =
        "POST /post HTTP/1.1\r\n"
        "Host: httpbin.org\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/form-data; boundary=-------------aaaaaaaa\r\n"
        "Content-Length: 1124\r\n"
        "\r\n"
        "---------------aaaaaaaa\r\n"
        "Content-Disposition: form-data; name=\"field\"\r\n"
        "\r\n"
        "%s\r\n"
        "---------------aaaaaaaa--\r\n";

    auto request = std::make_unique<char[]>(bufferSize);
    assertTrue((bool)request);
    memset(request.get(), 0, bufferSize);

    snprintf(request.get(), bufferSize, requestFormat, randData.get());

    const int requestSize = strlen(request.get());

    Cellular.connect();
    waitFor(Cellular.ready, 120000);

    TCPClient c;
    int res = c.connect("httpbin.org", 80);
    (void)res;

    int sz = c.write((const uint8_t*)request.get(), requestSize);
    assertEqual(sz, requestSize);

    auto responseBuf = std::make_unique<char[]>(bufferSize);
    assertTrue((bool)responseBuf);
    memset(responseBuf.get(), 0, bufferSize);
    int responseSize = 0;
    uint32_t mil = millis();
    while(1) {
        while (c.available() && responseSize < bufferSize) {
            responseBuf.get()[responseSize++] = c.read();
        }
        if (!c.connected() && c.available() <= 0)
            break;
        if (millis() - mil >= 20000UL) {
            if (c.connected()) {
                c.stop();
            }
            break;
        }
    }

    bool contains = false;
    if (responseSize > 0 && !c.connected()) {
        contains = strstr(responseBuf.get(), randData.get()) != nullptr;
        if (!contains) {
            // A workaround for httpbin.org sometimes returning an error
            if (strstr(responseBuf.get(), "HTTP/1.1") == responseBuf.get()) {
                int responseCode = -1;
                if (sscanf(responseBuf.get(), "HTTP/1.1 %d", &responseCode) == 1 &&
                        responseCode >= 200 && responseCode < 600) {
                    // FIXME: assume everything went fine
                    contains = true;
                }
            }
        }
    }

    assertTrue(contains);
}

namespace {

// The modem echoes the line back, so this gets a long reply from every modem
const size_t LONG_COMMAND_SIZE = 480;
const unsigned LONG_COMMAND_COUNT = 20;

// Lets the modem finish anything it was already sending
const system_tick_t RTS_SETTLE_MS = 3000;
// Short enough that the mux keepalive doesn't time out
const system_tick_t RTS_HOLD_MS = 10000;
const system_tick_t RTS_REPLY_MS = 10000;

// PIN_INVALID where we don't know the pin
pin_t cellularRtsPin() {
#if PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER
    return RTS1;
#elif PLATFORM_ID == PLATFORM_MSOM
    return RTS2;
#else
    return PIN_INVALID;
#endif
}

// These ignore RTS once CMUX is running (seen on the BG96 and BG95-M5)
bool ignoresRtsWithMux(int modemType) {
    switch (modemType) {
    case DEV_QUECTEL_BG96:
    case DEV_QUECTEL_BG95_M1:
    case DEV_QUECTEL_BG95_MF:
    case DEV_QUECTEL_BG95_M6:
    case DEV_QUECTEL_BG95_M5:
    case DEV_QUECTEL_BG95_S5:
        return true;
    default:
        return false;
    }
}

int ifcCallback(int type, const char* buf, int len, bool* hwFlowControl) {
    if (strstr(buf, "+IFC: 2,2")) {
        *hwFlowControl = true;
    }
    return WAIT;
}

int iprCallback(int type, const char* buf, int len, unsigned* baud) {
    sscanf(buf, "\r\n+IPR: %u", baud);
    return WAIT;
}

// Returns how many commands got a reply, stopping at the first one that didn't
unsigned sendLongCommands(const char* cmd, const char* what) {
    unsigned i = 0;
    int r = RESP_OK;
    for (; i < LONG_COMMAND_COUNT; i++) {
        r = Cellular.command(10000, "%s\r\n", cmd);
        if (r != RESP_ERROR && r != RESP_OK) {
            break;
        }
    }
    if (i < LONG_COMMAND_COUNT) {
        Log.error("%s: long command %u of %u %s (%d)", what, i + 1, LONG_COMMAND_COUNT,
                r == WAIT ? "timed out" : "failed", r);
    }
    return i;
}

// Counts the lines the AT parser receives
class AtLineCounter: public LogHandler {
public:
    AtLineCounter() :
            LogHandler(LOG_LEVEL_NONE, { { "ncp.at", LOG_LEVEL_ALL } }) {
        LogManager::instance()->addHandler(this);
    }

    ~AtLineCounter() {
        LogManager::instance()->removeHandler(this);
    }

    unsigned lines() const {
        return lines_;
    }

    // For the error log
    const char* firstLine() const {
        return firstLine_;
    }

protected:
    void logMessage(const char* msg, LogLevel level, const char* category, const LogAttributes& attr) override {
        if (strncmp(msg, "< ", 2) != 0) {
            return;
        }
        if (lines_++ == 0) {
            strlcpy(firstLine_, msg + 2, sizeof(firstLine_));
        }
    }

private:
    volatile unsigned lines_ = 0;
    char firstLine_[64] = {};
};

struct HeldCommand {
    const char* cmd;
    volatile int result;
    volatile bool returned;
};

} // namespace

test(MDM_02_at_commands_with_long_response_are_correctly_parsed_and_flow_controlled) {
    // Some modems run without flow control on purpose, e.g. older SARA-R410 firmware
    bool hwFlowControl = false;
    assertEqual((int)RESP_OK, Cellular.command(ifcCallback, &hwFlowControl, 10000, "AT+IFC?\r\n"));
    const pin_t rts = cellularRtsPin();
    if (!hwFlowControl || rts == PIN_INVALID || ignoresRtsWithMux(cellular_modem_type())) {
        skip();
        return;
    }
    unsigned baud = 0;
    assertEqual((int)RESP_OK, Cellular.command(iprCallback, &baud, 10000, "AT+IPR?\r\n"));
    assertMore(baud, 0u);

    // Not a real command, the modem echoes it back and answers ERROR
    char cmd[LONG_COMMAND_SIZE + 1] = "AT+PFLOWCONTROL=\"";
    size_t n = strlen(cmd);
    for (; n < LONG_COMMAND_SIZE - 1; n++) {
        cmd[n] = "0123456789ABCDEF"[n % 16];
    }
    cmd[n++] = '"';
    cmd[n] = '\0';

    // A failure here isn't about flow control
    assertEqual(sendLongCommands(cmd, "Baseline"), LONG_COMMAND_COUNT);

    // Keep cloud traffic out of the way while RTS is held
    Particle.disconnect();
    waitFor(Particle.disconnected, 30000);

    // Hold RTS from a GPIO. The UART keeps receiving, so anything the modem sends still shows up
    hal_usart_begin_config(HAL_PLATFORM_CELLULAR_SERIAL, baud, SERIAL_8N1 | SERIAL_FLOW_CONTROL_CTS, nullptr);
    pinMode(rts, OUTPUT);
    digitalWrite(rts, HIGH);
    SCOPE_GUARD({
        hal_usart_begin_config(HAL_PLATFORM_CELLULAR_SERIAL, baud, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS, nullptr);
    });
    delay(RTS_SETTLE_MS);

    AtLineCounter counter;
    HeldCommand held = { cmd, RESP_OK, false };
    Thread sender("held_cmd", [](void* arg) {
        auto h = (HeldCommand*)arg;
        h->result = Cellular.command(RTS_HOLD_MS + RTS_REPLY_MS + 10000, "%s\r\n", h->cmd);
        h->returned = true;
    }, &held);

    delay(RTS_HOLD_MS);
    const unsigned linesWhileHeld = counter.lines();
    const bool returnedWhileHeld = held.returned;
    if (linesWhileHeld > 0 || returnedWhileHeld) {
        Log.error("Modem sent %u lines with RTS held off, first: %s", linesWhileHeld, counter.firstLine());
    }

    digitalWrite(rts, LOW);
    for (const auto start = millis(); !held.returned && millis() - start < RTS_REPLY_MS;) {
        delay(10);
    }
    const bool returnedAfterRelease = held.returned;
    const int result = held.result;
    const unsigned linesAfter = counter.lines();
    if (!returnedAfterRelease && linesAfter == 0) {
        Log.error("No reply after RTS was released");
    } else if (!returnedAfterRelease || result != RESP_ERROR) {
        Log.error("Reply did not match (%d) after %u lines, first: %s", result, linesAfter,
                counter.firstLine());
    }
    sender.join();

    // Separate so the report shows which one failed
    assertEqual(linesWhileHeld, 0u);
    assertFalse(returnedWhileHeld);
    assertTrue(returnedAfterRelease);
    assertEqual(result, (int)RESP_ERROR);
    // Proves the counter was working
    assertMore(linesAfter, 0u);
}

test(MDM_03_restore_cloud_connection) {
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
}


namespace {
const system_tick_t CELLULAR_LOCK_HOLD_MS = 10000;

volatile bool sCellularLockHeld = false;
volatile bool sCellularLockReleased = false;
} // anonymous

test(CELLULAR_08_device_info_not_blocked_by_client_lock) {
    if (system_thread_get_state(nullptr) != spark::feature::ENABLED) {
        skip();
        return;
    }

    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    CellularDevice live = {};
    live.size = sizeof(live);
    assertEqual(cellular_device_info(&live, nullptr), 0);
    assertTrue(strlen(live.imei) > 0);

    Particle.disconnect();
    waitFor(Particle.disconnected, 30000);
    Cellular.off();
    waitFor(Cellular.isOff, MAX_CELLULAR_OFF_TIME);
    assertTrue(Cellular.isOff());

    sCellularLockHeld = false;
    sCellularLockReleased = false;
    {
        Thread holder("cellLockHolder", [] {
            Cellular.lock();
            sCellularLockHeld = true;
            delay(CELLULAR_LOCK_HOLD_MS);
            Cellular.unlock();
            sCellularLockReleased = true;
        });
        assertTrue(holder.isValid());
        SCOPE_GUARD({
            holder.join();
        });
        assertTrue(waitFor([] { return (bool)sCellularLockHeld; }, 5000));

        CellularDevice cached = {};
        cached.size = sizeof(cached);
        const system_tick_t start = millis();
        assertEqual(cellular_device_info(&cached, nullptr), 0);
        assertLess(millis() - start, 1000);
        assertEqual(strcmp(cached.imei, live.imei), 0);
        assertEqual(strcmp(cached.iccid, live.iccid), 0);
    }
    assertTrue((bool)sCellularLockReleased);

    Cellular.on();
    waitFor(Cellular.isOn, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    connect_to_cloud(HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
}

#endif // Wiring_Cellular == 1
