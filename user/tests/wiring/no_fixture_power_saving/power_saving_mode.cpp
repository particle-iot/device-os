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

} // namespace

uint32_t measureAvgPeriodMs(pin_t pin, uint32_t wait_ms) {
    int high_pulse = 0;
    int low_pulse = 0;
    int count = 0;
    // Dummy read to sync pulse edge
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
    uint32_t s = millis();
    while (millis() - s < wait_ms) {
        // This works better without an atomic block
        // ATOMIC_BLOCK() {
            high_pulse += pulseIn(pin, HIGH);
            low_pulse += pulseIn(pin, LOW);
        // }
        count++;
    }
    high_pulse = high_pulse/count;
    low_pulse = low_pulse/count;

    return (high_pulse + low_pulse)/1000;
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

        period = measureAvgPeriodMs(CTS1, WAIT_FOR_LOW_POWER_MEAS_MS);
        if (period <= 1280 + 128 && period >= 1280 - 128) { // 1.28s +/- 10% if in low power mode
            lowPowerResult = true;
        } else {
            lowPowerResult = false;
        }

        publishResult = Particle.publish("my_event_low_power", "event data low power", PRIVATE | WITH_ACK);
        // Log.info("period: %lu, publishResult: %d", period, publishResult);
    } while (!(publishResult && lowPowerResult) && attempts++ < LOW_POWER_ATTEMPTS_MAX);
    assertMoreOrEqual(period, 1280 - 128); // 1.28s +/- 10% if in low power mode
    assertLessOrEqual(period, 1280 + 128);
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

        period = measureAvgPeriodMs(CTS1, WAIT_FOR_LOW_POWER_MEAS_MS);
        if (period <= 1280 + 128 && period >= 1280 - 128) { // 1.28s +/- 10% if in low power mode
            lowPowerResult = true;
        } else {
            lowPowerResult = false;
        }
        // Log.info("period: %lu", period);

    } while (!lowPowerResult && attempts++ < LOW_POWER_ATTEMPTS_MAX);
    assertMoreOrEqual(period, 1280 - 128); // 1.28s +/- 10% if in low power mode
    assertLessOrEqual(period, 1280 + 128);
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

test(POWER_SAVING_06_system_sleep_with_configuration_object_ultra_low_power_mode_wake_by_network) {
    if (ncpId != PLATFORM_NCP_SARA_R510) {
        skip();
        return;
    }

    Particle.connect();
    assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));

    delay(15s); // a bit of delay required to avoid premature wake, even with SystemSleepFlag::WAIT_CLOUD
    lowPowerResult = false;
    attempts = 0;
    period = 0;
    do {
        Particle.connect();
        assertTrue(waitFor(Particle.connected, CLOUD_CONNECT_TIMEOUT));
        delay(WAIT_FOR_LOW_POWER_ACTIVE_MS); // wait for UPSV=1 default delay of ~9.2s before modem drops into low power mode idle mode.

        period = measureAvgPeriodMs(CTS1, WAIT_FOR_LOW_POWER_MEAS_MS);
        if (period <= 1280 + 128 && period >= 1280 - 128) { // 1.28s +/- 10% if in low power mode
            lowPowerResult = true;
        } else {
            lowPowerResult = false;
        }
        // Log.info("period: %lu", period);

    } while (!lowPowerResult && attempts++ < LOW_POWER_ATTEMPTS_MAX);
    assertMoreOrEqual(period, 1280 - 128); // 1.28s +/- 10% if in low power mode
    assertLessOrEqual(period, 1280 + 128);

    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 30000));
    system_tick_t start = millis();
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
        .duration(60s)
        .network(NETWORK_INTERFACE_CELLULAR);
    SystemSleepResult result = System.sleep(config);

    // in sleep for 60s, should wake up after 30s due to function call

    assertEqual((int)result.wakeupReason(), (int)SystemSleepWakeupReason::BY_NETWORK);
    assertLessOrEqual(millis() - start, 50 * 1000);
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

    // See no_fixture_long_running.spec.js
}

#endif // HAL_PLATFORM_CELLULAR_LOW_POWER
