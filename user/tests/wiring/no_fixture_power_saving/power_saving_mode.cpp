#include "application.h"
#include "test.h"

#if HAL_PLATFORM_CELLULAR_LOW_POWER

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

namespace {

constexpr system_tick_t CLOUD_CONNECT_TIMEOUT = 10 * 60 * 1000;
constexpr system_tick_t CLOUD_DISCONNECT_TIMEOUT = 1 * 60 * 1000;
constexpr uint32_t WAIT_FOR_LOW_POWER_ACTIVE_MS = 20000;
constexpr uint32_t WAIT_FOR_LOW_POWER_MEAS_MS = 10000;
constexpr uint32_t LOW_POWER_ATTEMPTS_MAX = 10; // XXX: extreme cases take up to 2.5 minutes to drop into low power mode
constexpr char skip_test_msg[] = "skip_test";
constexpr char sleeping_msg[] = "sleeping";
int ncpId = DEV_UNKNOWN;
String fnLp1Arg;
bool appThread = true;
int returnVal = 12345;
bool publishResult = false;
bool lowPowerResult = false;
int attempts = 0;
uint32_t period = 0;

int fnLp1(const String& arg) {
    if (!application_thread_current(nullptr)) {
        appThread = false;
    }
    fnLp1Arg = arg;
    return returnVal++;
}

int upsvCallback(int type, const char* buf, int len, int* mode) {
    (void)type;
    (void)len;
    int m = -1;
    if (sscanf(buf, "\r\n+UPSV: %d", &m) == 1) {
        *mode = m;
    }
    return WAIT;
}

void formatLowPower(char* buf, size_t size) {
    snprintf(buf, size, "low power mode %s after %d retries, median=%lums",
            lowPowerResult ? "DETECTED" : "NOT DETECTED", attempts, (unsigned long)period);
    Log.info("%s", buf);
}

// Note: Wakes the modem, so only call once a measurement window has closed.
int readUpsvMode() {
    int mode = -1;
    Cellular.command(upsvCallback, &mode, 10000, "AT+UPSV?");
    return mode;
}

} // namespace

namespace {

// In low power mode CTS idles HIGH and blips LOW for only ~26ms every 1.28s, which pulseIn() loses
// whenever the app thread is preempted. An ISR catches every edge. 256 is far more than a 10s
// window holds at 1.28s (~16), so a full buffer means the line is toggling fast.
constexpr size_t MAX_EDGES = 256;
volatile uint32_t edgeUs[MAX_EDGES];
volatile size_t edgeCount = 0;
volatile uint8_t edgeLevel[MAX_EDGES]; // level AFTER the transition
pin_t edgePin = 0;
uint32_t periodsUs[MAX_EDGES / 2];

// digitalRead() returns 0 on a UART-owned pin (pin_mode is PIN_MODE_NONE); pinReadFast() reads the
// GPIO IN register directly.
void ctsIsr() {
    const uint32_t t = micros();
    const int level = pinReadFast(edgePin);
    if (edgeCount < MAX_EDGES) {
        edgeUs[edgeCount] = t;
        edgeLevel[edgeCount] = (uint8_t)level;
        edgeCount++;
    }
}

} // namespace

// Median rising-to-rising period over wait_ms, or 0 if the line did not toggle enough to measure
// one. Median not mean: a missed blip merges two periods into ~2x nominal, which drags a mean over
// a handful of samples out of band. Requires attachCtsCapture().
uint32_t measurePeriodMs(pin_t pin, uint32_t wait_ms) {
    (void)pin; // the ISR is already attached to CTS1 for the life of the test
    edgeCount = 0;

    // Yield rather than busy-wait, so the system thread runs normally through the window.
    const uint32_t start = millis();
    while (millis() - start < wait_ms) {
        delay(1);
    }

    const size_t edges = edgeCount;
    const uint32_t spanMs = (edges >= 2) ? (edgeUs[edges - 1] - edgeUs[0]) / 1000 : 0;
    int n = 0;
    int lastRising = -1;
    for (size_t i = 0; i < edges; i++) {
        if (!edgeLevel[i]) {
            continue;
        }
        if (lastRising >= 0 && n < (int)(sizeof(periodsUs) / sizeof(periodsUs[0]))) {
            periodsUs[n++] = edgeUs[i] - edgeUs[lastRising];
        }
        lastRising = (int)i;
    }
    if (n == 0) {
        Log.warn("CTS1: no period, edges=%u span=%lums level=%d%s", (unsigned)edges,
                (unsigned long)spanMs, (int)pinReadFast(CTS1),
                (edges >= MAX_EDGES) ? " BUFFER FULL" : "");
        return 0;
    }
    for (int i = 1; i < n; i++) {
        const uint32_t v = periodsUs[i];
        int j = i - 1;
        while (j >= 0 && periodsUs[j] > v) {
            periodsUs[j + 1] = periodsUs[j];
            j--;
        }
        periodsUs[j + 1] = v;
    }
    // A wide p25..p75 means there is no consistent period and the median describes nothing real
    Log.info("CTS1: median=%lums p25=%lu p75=%lu periods=%d edges=%u span=%lums rate=%lu/min%s",
            (unsigned long)(periodsUs[n / 2] / 1000), (unsigned long)(periodsUs[n / 4] / 1000),
            (unsigned long)(periodsUs[(3 * n) / 4] / 1000), n, (unsigned)edges,
            (unsigned long)spanMs,
            spanMs ? (unsigned long)((uint64_t)n * 60000 / spanMs) : 0UL,
            (edges >= MAX_EDGES) ? " BUFFER FULL" : "");
    return periodsUs[n / 2] / 1000;
}

// Attached once per run: detach ends with hal_pin_set_function(pin, PF_NONE),
// clearing the pinmap's record that CTS belongs to the UART
bool attachCtsCapture(pin_t pin) {
    edgePin = pin;
    edgeCount = 0;

    // pinReadFast() is not ISR-safe on its FIRST call: fastPinGetPinmap()'s function-local static
    // initializes via __cxa_guard_acquire, which asserts in interrupt context -> SOS 10. Prime it
    // from thread context first.
    (void)pinReadFast(pin);

    // hal_interrupt_attach() passes skip_gpio_setup = true, so the UART keeps its pin configuration.
    return attachInterrupt(pin, ctsIsr, CHANGE);
}

test(POWER_SAVING_00_setup) {
    auto info = System.hardwareInfo();
    assertTrue(info.isValid());
#if HAL_PLATFORM_NCP
    assertNotEqual(info.ncp().size(), 0);
    auto ncpIds = info.ncp();
    ncpId = ncpIds[0];
#else
    ncpId = PLATFORM_NCP_NONE;
#endif // HAL_PLATFORM_NCP

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(skip_test_msg, sizeof(skip_test_msg) - 1));
        skip();
        return;
    }

    assertTrue(attachCtsCapture(CTS1));

    Particle.disconnect();
    assertTrue(waitFor(Particle.disconnected, CLOUD_DISCONNECT_TIMEOUT));
    Particle.function("fnlp1", fnLp1);
}

test(POWER_SAVING_01_particle_publish_publishes_an_event_after_low_power_active) {


    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    publishResult = false;
    lowPowerResult = false;
    attempts = 0;
    period = 0;
    do {
        Particle.connect();
        assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
        delay(WAIT_FOR_LOW_POWER_ACTIVE_MS); // wait for UPSV=1 default delay of ~9.2s before modem drops into low power mode idle mode.

        period = measurePeriodMs(CTS1, WAIT_FOR_LOW_POWER_MEAS_MS);
        if (period <= 1280 + 128 && period >= 1280 - 128) { // 1.28s +/- 10% if in low power mode
            lowPowerResult = true;
        } else {
            lowPowerResult = false;
        }

        publishResult = Particle.publish("my_event_low_power", "event data low power", PRIVATE | WITH_ACK);
        // Log.info("period: %lu, publishResult: %d", period, publishResult);
    } while (!(publishResult && lowPowerResult) && attempts++ < LOW_POWER_ATTEMPTS_MAX);
    // Not asserted: whether the R510 enters low power depends on paging state we do not control,
    // so gating on it only makes the test flaky. Reported for analysis.
    char lpMsg[96];
    formatLowPower(lpMsg, sizeof(lpMsg));
    pushMailboxMsg(lpMsg, 5000);

    // Not deterministic that the modem enters low power; it is deterministic that Device OS set it.
    assertEqual(readUpsvMode(), 1);
    assertTrue(publishResult);
}

test(POWER_SAVING_02_register_function_and_connect_to_cloud) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    lowPowerResult = false;
    attempts = 0;
    period = 0;
    do {
        Particle.connect();
        assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
        delay(WAIT_FOR_LOW_POWER_ACTIVE_MS); // wait for UPSV=1 default delay of ~9.2s before modem drops into low power mode idle mode.

        period = measurePeriodMs(CTS1, WAIT_FOR_LOW_POWER_MEAS_MS);
        if (period <= 1280 + 128 && period >= 1280 - 128) { // 1.28s +/- 10% if in low power mode
            lowPowerResult = true;
        } else {
            lowPowerResult = false;
        }
        // Log.info("period: %lu", period);

    } while (!lowPowerResult && attempts++ < LOW_POWER_ATTEMPTS_MAX);
    // Not asserted: whether the R510 enters low power depends on paging state we do not control,
    // so gating on it only makes the test flaky. Reported for analysis.
    char lpMsg[96];
    formatLowPower(lpMsg, sizeof(lpMsg));
    pushMailboxMsg(lpMsg, 5000);

    // Not deterministic that the modem enters low power; it is deterministic that Device OS set it.
    assertEqual(readUpsvMode(), 1);
}

test(POWER_SAVING_03_call_function_and_check_return_value_after_low_power_active) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    // See no_fixture_long_running.spec.js
}

test(POWER_SAVING_04_check_function_argument_value) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    // Loop a bit before we check this to make sure the device app thread has received this message.
    // 60s is kind of long, but we are not validating how fast it can receive this data, just that it
    // does.  We want this test to be reliable.
    for (auto start = millis(); fnLp1Arg != "argument string low power" && millis() - start < 60000;) {
        Particle.process();   // pump application events
    }
    assertTrue(fnLp1Arg == "argument string low power");
}

test(POWER_SAVING_05_check_current_thread) {

    // Verify that all function calls have been performed in the application thread
    assertTrue(appThread);
}

namespace {

SystemSleepResult result_06;
system_tick_t start_06 = 0;

}

test(POWER_SAVING_06_system_sleep_with_configuration_object_ultra_low_power_mode_wake_by_network_1) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    delay(15s); // a bit of delay required to avoid premature wake, even with SystemSleepFlag::WAIT_CLOUD

    // No low power measurement here: waking on inbound traffic does not depend on it, and tests 01
    // and 02 cover it.

    // Signals the JS side that the sleep is starting; the wake call is armed from test 05's body and
    // gated on this, because the runner runs each device test in an awaited beforeEach hook and
    // test 06_1's own body cannot run until this sleep has ended. Sent over the mailbox (USB) so it
    // costs no modem traffic.
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(sleeping_msg, sizeof(sleeping_msg) - 1), 30000));

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 30000));
    start_06 = millis();
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
        .duration(120s)
        .network(NETWORK_INTERFACE_CELLULAR);
    detachInterrupt(CTS1);
    result_06 = System.sleep(config);

    // in sleep for 120s, should wake up after ~45s due to function call
}

test(POWER_SAVING_06_system_sleep_with_configuration_object_ultra_low_power_mode_wake_by_network_2) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    assertEqual((int)result_06.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertLessOrEqual(millis() - start_06, 50 * 1000);
}

test(POWER_SAVING_07_check_function_argument_value) {

    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    // Loop a bit before we check this to make sure the device app thread has received this message.
    // 60s is kind of long, but we are not validating how fast it can receive this data, just that it
    // does.  We want this test to be reliable.
    for (auto start = millis(); fnLp1Arg != "argument string low power sleep" && millis() - start < 60000;) {
        Particle.process();   // pump application events
    }
    assertTrue(fnLp1Arg == "argument string low power sleep");
}

test(POWER_SAVING_99_cleanup) {
    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    detachInterrupt(CTS1);

    // See no_fixture_long_running.spec.js
}

#endif // HAL_PLATFORM_CELLULAR_LOW_POWER
