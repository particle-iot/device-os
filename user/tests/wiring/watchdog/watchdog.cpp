/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"


static void startWatchdog(const WatchdogConfiguration& config) {
    WatchdogInfo info;
    assertEqual(0, Watchdog.init(config));
    assertEqual(0, Watchdog.start());
    assertEqual(0, Watchdog.getInfo(info));
    assertTrue(info.state() == WatchdogState::STARTED);
}

static void checkState(WatchdogState state) {
    WatchdogInfo info;
    assertEqual(0, Watchdog.getInfo(info));
    assertTrue(info.state() == state);
}

test(WATCHDOG_00_setup_disconnect_power_off_ncp) {
    // Ensure NCP is already off for test timing to work properly.
    // sleep STOP mode can "pause" the WDT countdown, but if the NCP is on
    // when sleep API's are called, extra time will be need to disconnect
    // and power off before sleeping.
    Particle.disconnect();
    waitFor(Particle.disconnected, 60000);
    assertTrue(Particle.disconnected());
#if Wiring_Cellular
    Cellular.disconnect();
    waitForNot(Cellular.ready, 60000);
    Cellular.off();
    waitFor(Cellular.isOff, 120000);
    assertTrue(Cellular.isOff());
#elif Wiring_WiFi
    WiFi.disconnect();
    waitForNot(WiFi.ready, 60000);
    WiFi.off();
    waitFor(WiFi.isOff, 120000);
    assertTrue(WiFi.isOff());
#endif
}

test(WATCHDOG_01_capabilities) {
    WatchdogInfo info;
    assertEqual(0, Watchdog.getInfo(info));
#if HAL_PLATFORM_NRF52840
    assertTrue(info.mandatoryCapabilities() == WatchdogCap::RESET);
    assertTrue(info.capabilities() == (WatchdogCap::NOTIFY | WatchdogCap::SLEEP_RUNNING | WatchdogCap::DEBUG_RUNNING));
#else
#error "Unsupported platform"
#endif //
}

test(WATCHDOG_02_default_1) {
    WatchdogInfo info;

    assertEqual(0, Watchdog.init(WatchdogConfiguration().timeout(5s)));
    checkState(WatchdogState::CONFIGURED);

    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));

    assertEqual(0, Watchdog.start());
    assertEqual(0, Watchdog.getInfo(info));
    assertEqual(info.configuration().timeout(), 5000);
    assertTrue(info.state() == WatchdogState::STARTED);
    assertTrue(Watchdog.started());

    delay(3000);
    Watchdog.refresh();
    delay(3000);
    Watchdog.refresh();
    delay(10000);
    // This should not be reachable
    assertFalse(true);
}

test(WATCHDOG_02_default_2) {
    checkState(WatchdogState::DISABLED);
    assertEqual((int)System.resetReason(), (int)RESET_REASON_WATCHDOG);
}

#if (PLATFORM_ID == PLATFORM_TRACKER)
test(WATCHDOG_03_stopped_in_hibernate_mode_1) {
    startWatchdog(WatchdogConfiguration().timeout(5s));
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::HIBERNATE).duration(10s));
}

test(WATCHDOG_03_stopped_in_hibernate_mode_2) {
    assertTrue(System.resetReason() == RESET_REASON_POWER_MANAGEMENT);
    checkState(WatchdogState::DISABLED);
}
#endif

test(WATCHDOG_04_continue_running_after_waking_up_from_none_hibernate_mode_1) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    startWatchdog(WatchdogConfiguration().timeout(5s).capabilities(WatchdogCap::RESET)); // SLEEP_RUNNING is cleared
    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(10s));
    Watchdog.refresh();
}

test(WATCHDOG_04_continue_running_after_waking_up_from_none_hibernate_mode_2) {
    checkState(WatchdogState::STARTED);
    Watchdog.refresh();
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    delay(10000);
    // This should be unreachable, we should get a watchdog reset before the delay expires
    assertFalse(true);
}

test(WATCHDOG_04_continue_running_after_waking_up_from_none_hibernate_mode_3) {
    assertTrue(System.resetReason() == RESET_REASON_WATCHDOG);
    checkState(WatchdogState::DISABLED);
}

test(WATCHDOG_05_system_reset_will_disable_watchdog_1) {
    startWatchdog(WatchdogConfiguration().timeout(5s));
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 1000));
    System.reset();
}

test(WATCHDOG_05_system_reset_will_disable_watchdog_2) {
    assertTrue(System.resetReason() == RESET_REASON_USER);
    checkState(WatchdogState::DISABLED);
}

#if HAL_PLATFORM_NRF52840

test(WATCHDOG_06_default_running_in_none_hibernate_mode_1) {
    startWatchdog(WatchdogConfiguration().timeout(5s));
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::STOP).duration(10s));
}

test(WATCHDOG_06_default_running_in_none_hibernate_mode_2) {
    assertTrue(System.resetReason() == RESET_REASON_WATCHDOG);
    checkState(WatchdogState::DISABLED);
}

static retained uint32_t magick = 0;

test(WATCHDOG_07_notify_1) {
    magick = 0;
    Watchdog.onExpired([](){
        magick = 0xdeadbeef;
    });
    startWatchdog(WatchdogConfiguration().timeout(5s).capabilities(WatchdogCap::NOTIFY));
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
}

test(WATCHDOG_07_notify_2) {
    assertTrue(System.resetReason() == RESET_REASON_WATCHDOG);
    assertEqual(magick, 0xdeadbeef);
}

#endif // HAL_PLATFORM_NRF52840

