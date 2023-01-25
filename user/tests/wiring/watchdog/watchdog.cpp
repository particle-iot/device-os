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

test(WATCHDOG_01_capabilities) {
    WatchdogInfo info;
    assertEqual(0, Watchdog.getInfo(info));
#if HAL_PLATFORM_NRF52840
    assertTrue(info.mandatoryCapabilities() == WatchdogCap::RESET);
    assertTrue(info.capabilities() == (WatchdogCap::NOTIFY | WatchdogCap::SLEEP_RUNNING | WatchdogCap::DEBUG_RUNNING));
#elif HAL_PLATFORM_RTL872X
    assertTrue(info.mandatoryCapabilities() == WatchdogCap::DEBUG_RUNNING);
    assertTrue(info.capabilities() == (WatchdogCap::RESET | WatchdogCap::NOTIFY_ONLY |
                                       WatchdogCap::RECONFIGURABLE | WatchdogCap::STOPPABLE));
#else
#error "Unsupported platform"
#endif //
}

test(WATCHDOG_02_default) {
    assertEqual(0, Watchdog.init(WatchdogConfiguration().timeout(5s)));
    WatchdogInfo info;
    assertEqual(0, Watchdog.getInfo(info));
    // FIXME?
    // assertTrue(info.state() == WatchdogState::CONFIGURED);

    assertEqual(0, Watchdog.start());
    assertEqual(0, Watchdog.getInfo(info));
    assertEqual(info.configuration().timeout(), 5000);
    assertTrue(info.state() == WatchdogState::STARTED);
    assertTrue(Watchdog.started());

    delay(3000);
    Watchdog.refresh();
    delay(3000);
    Watchdog.refresh();

    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
}

test(WATCHDOG_03_reset_reason) {
    assertTrue(System.resetReason() == RESET_REASON_WATCHDOG);
}

// TODO: more cases e.g. reconfiguration, System.reset() on nRF52840 disabling watchdong on boot etc
