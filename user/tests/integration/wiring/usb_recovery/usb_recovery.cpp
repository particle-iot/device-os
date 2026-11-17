#include "application.h"
#include "unit-test/unit-test.h"

#if HAL_PLATFORM_RTL872X

// Recovery of the rtl872x USB driver (hal/src/rtl872x/usbd_driver.cpp) from failures that can't be produced on demand
// from the host. The device side injects the failure by writing the USB core registers directly
//
// The runner talks to the device over EP0 only while a device test is running. A device test either injects the failure
// while the runner polls it, or starts a thread that injects it after the test returns, while the host side runs

namespace {

// USB core registers (rtl8721d_usb.h)
const uintptr_t TEST_USB_DEVICE_BASE = 0x40040800;
volatile uint32_t* const TEST_USB_DCTL = (volatile uint32_t*)(TEST_USB_DEVICE_BASE + 0x04);
volatile uint32_t* const TEST_USB_DOEPMSK = (volatile uint32_t*)(TEST_USB_DEVICE_BASE + 0x14);
const uint32_t TEST_USB_DCTL_GONSTS = 1 << 3; // Global OUT NAK status
const uint32_t TEST_USB_DCTL_GINSTS = 1 << 2; // Global IN NAK status
const uint32_t TEST_USB_DCTL_SGINAK = 1 << 7; // Set global IN NAK
const uint32_t TEST_USB_DCTL_CGINAK = 1 << 8; // Clear global IN NAK
const uint32_t TEST_USB_DCTL_SGONAK = 1 << 9; // Set global OUT NAK
const uint32_t TEST_USB_DOEPMSK_STUPM = 1 << 3; // SETUP phase done

// Let the runner start polling the device test before injecting
const system_tick_t RUNNER_SETTLE_MS = 2000;

Thread* g_injectThread = nullptr;
Thread* g_cdcTxThread = nullptr;
volatile bool g_runCdcTxThread = false;

// Heartbeat at the system thread's priority (see heartbeatThread())
const system_tick_t HEARTBEAT_PERIOD_MS = 10;
// Longest a thread at that priority may go without running while the host holds the CDC port unread. The guard stops a
// storm within about 10 ms (and again each time it re-arms, about once a second); without it the storm starves these
// threads for as long as the port stays unread
const system_tick_t MAX_HEARTBEAT_GAP_MS = 250;
Thread* g_heartbeatThread = nullptr;
volatile bool g_runHeartbeat = false;
volatile system_tick_t g_heartbeatMaxGap = 0;

void maskSetupPhase() {
    // The SDK reads EP0 OUT interrupts masked with DOEPMSK and sets the mask again only on a bus reset: SETUPs reach the
    // RX FIFO but never the stack, so the host's requests go unanswered. Only the watchdog's reattach (and the bus reset
    // that follows) restores it.
    *TEST_USB_DOEPMSK &= ~TEST_USB_DOEPMSK_STUPM;
}

bool setupPhaseUnmasked() {
    // The reattach only reconnects: the host's bus reset that follows restores the mask
    return (*TEST_USB_DOEPMSK & TEST_USB_DOEPMSK_STUPM) != 0;
}

const system_tick_t WATCH_PERIOD_MS = 1000; // RtlUsbDriver::loop()
const system_tick_t GLOBAL_IN_NAK_HOLD_MS = 500; // Longer than the host's 250 ms cancel, shorter than WATCH_PERIOD_MS
system_tick_t g_watchPass = 0;

// RtlUsbDriver::watch() clears a global OUT NAK on its next pass: returns when that pass ran
system_tick_t syncWithWatch() {
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGONAK;
    waitFor([] { return !(*TEST_USB_DCTL & TEST_USB_DCTL_GONSTS); }, 2 * WATCH_PERIOD_MS);
    return millis();
}

void holdGlobalInNak() {
    // The core NAKs every IN token, so the reply armed on EP0 IN doesn't go out: the host's control read gives up and
    // leaves it armed. Held between two watch() passes so watch() doesn't flush it: only the host's next SETUP can cancel
    // it (RtlUsbDriver::cancelControlTransferOnSetup()) rather than let it go out as the next control read's data stage.
    system_tick_t start = g_watchPass;
    while (start - g_watchPass < RUNNER_SETTLE_MS) {
        start += WATCH_PERIOD_MS;
    }
    delay(start + 100 - millis());
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGINAK;
    delay(GLOBAL_IN_NAK_HOLD_MS);
    *TEST_USB_DCTL |= TEST_USB_DCTL_CGINAK;
}

void cdcTxThread() {
    static uint32_t txCounter = 0;
    const size_t LINE_SIZE = 64;

    while (g_runCdcTxThread) {
        // Drain what the host writes: a full RX buffer makes the device NAK CDC OUT, a different interrupt storm
        while (Serial.read() >= 0) {
        }
        // Don't block when the host is not reading.
        if (!Serial.isConnected()) {
            delay(10);
            continue;
        }

        if (Serial.availableForWrite() >= (int)LINE_SIZE) {
            char line[LINE_SIZE + 1] = {};
            const int n = snprintf(line, sizeof(line), "%010lu %010lu ", (unsigned long)txCounter++, (unsigned long)millis());
            memset(line + n, 'x', LINE_SIZE - 1 - n);
            line[LINE_SIZE - 1] = '\n';
            Serial.write((const uint8_t*)line, LINE_SIZE);
        } else {
            delay(1);
        }
    }
}

// Wakes every HEARTBEAT_PERIOD_MS at the system thread's priority and records the longest time between wakeups: a TX
// FIFO empty interrupt storm keeps the USB task busy and starves every thread at this priority, the system thread
// included (no cloud, no control requests), with no host traffic needed to see it
void heartbeatThread() {
    auto last = millis();
    while (g_runHeartbeat) {
        delay(HEARTBEAT_PERIOD_MS);
        const auto now = millis();
        if (now - last > g_heartbeatMaxGap) {
            g_heartbeatMaxGap = now - last;
        }
        last = now;
    }
}

} // namespace

test(USB_RECOVERY_01_EP0_WATCHDOG_REATTACH) {
    delay(RUNNER_SETTLE_MS);
    maskSetupPhase();
    // Wait for host bus reset
    assertTrue(waitFor(setupPhaseUnmasked, 20000));
}

test(USB_RECOVERY_02_GLOBAL_NAK_CLEARED) {
    // The core NAKs every OUT packet, including the status stage of the host's control reads, so no control transfer
    // completes. Nothing in the stack clears it but RtlUsbDriver::watch().
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGONAK;
    assertTrue(waitFor([] { return !(*TEST_USB_DCTL & TEST_USB_DCTL_GONSTS); }, 3000));
    // Test global IN NAK
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGINAK;
    assertTrue(waitFor([] { return !(*TEST_USB_DCTL & TEST_USB_DCTL_GINSTS); }, 3000));
}

test(USB_RECOVERY_03_CANCEL_ON_SETUP) {
    g_watchPass = syncWithWatch();
    g_injectThread = new Thread("inject", holdGlobalInNak, OS_THREAD_PRIORITY_DEFAULT);
    assertTrue(g_injectThread);
}

test(USB_RECOVERY_04_CDC_TX_STORM_START) {
    // Test 03's inject thread ends during that test's host side
    if (g_injectThread) {
        g_injectThread->join();
        delete g_injectThread;
        g_injectThread = nullptr;
    }
    // Start a thread to continually TX CDC data IN to the host
    // The host will hold the CDC port open and keep writing to it without reading it, which on linux hosts strands a CDC
    // IN transfer and triggers the devices TX FIFO empty interrupt storm. Each CDC OUT packet
    // reopens the device TX side after the CDC TX timeout closed it, so the app keeps writing.
    // Without the TX storm fix in the driver, the USB task starves the rest of the system for as long as the host holds the port
    g_runCdcTxThread = true;
    g_cdcTxThread = new Thread("cdc_tx", cdcTxThread, OS_THREAD_PRIORITY_DEFAULT);

    // Measure if there is thread starvation on the device
    g_heartbeatMaxGap = 0;
    g_runHeartbeat = true;
    g_heartbeatThread = new Thread("heartbeat", heartbeatThread, OS_THREAD_PRIORITY_DEFAULT);
}

test(USB_RECOVERY_05_CDC_TX_STORM_TRIGGER) {
    // Host will trigger the storm while the cdc and heartbeat threads run
}

test(USB_RECOVERY_06_CDC_TX_STORM_CHECK) {
    // Join the threads
    g_runCdcTxThread = false;
    if (g_cdcTxThread) {
        g_cdcTxThread->join();
        delete g_cdcTxThread;
        g_cdcTxThread = nullptr;
    }
    g_runHeartbeat = false;
    if (g_heartbeatThread) {
        g_heartbeatThread->join();
        delete g_heartbeatThread;
        g_heartbeatThread = nullptr;
    }

    // Threads at the system thread's priority kept running while the host held the port unread
    const system_tick_t heartbeatMaxGap = g_heartbeatMaxGap;
    assertEqual(0, pushMailboxMsg(String(heartbeatMaxGap).c_str(), 5000));
    assertLessOrEqual(heartbeatMaxGap, MAX_HEARTBEAT_GAP_MS);
}

#endif // HAL_PLATFORM_RTL872X
