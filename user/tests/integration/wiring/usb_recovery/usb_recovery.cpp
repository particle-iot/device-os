#define PARTICLE_USE_UNSTABLE_API
#include "application.h"
#include "unit-test/unit-test.h"
#include "usb_hal.h"

#if HAL_PLATFORM_RTL872X

// Recovery of the rtl872x USB driver (hal/src/rtl872x/usbd_driver.cpp) from failures that can't be produced on demand
// from the host. The device side injects the failure by writing the USB core registers directly
//
// The runner talks to the device over EP0 only while a device test is running. The host side triggers 
// various failure states via cloud events, and then communicates with the device to verify it recovers

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

const String USB_RECOVERY_TRIGGER = "USB_RECOVERY_TRIGGER";
const String USB_RECOVERY_REPAIR_REATTACH = "USB_RECOVERY_REPAIR_REATTACH";
const String USB_RECOVERY_REPAIR = "USB_RECOVERY_REPAIR";
const String USB_RECOVERY_SETUP_CANCEL = "USB_RECOVERY_SETUP_CANCEL";
const String USB_RECOVERY_HOST_LINUX = "USB_RECOVERY_HOST_LINUX";
const String USB_RECOVERY_HOST_OTHER = "USB_RECOVERY_HOST_OTHER";

Thread* g_cdcTxThread = nullptr;
bool g_triggerFaultReceived = false;
volatile bool g_runCdcTxThread = false;

// Heartbeat at the system thread's priority (see heartbeatThread())
const system_tick_t HEARTBEAT_PERIOD_MS = 10;
// Longest a thread at that priority may go without running while the host holds the CDC port unread. The guard stops a
// storm within about 10 ms (and again each time it re-arms, about once a second); without it the storm starves these
// threads for as long as the port stays unread
const system_tick_t MAX_HEARTBEAT_GAP_MS = 1000;
Thread* g_heartbeatThread = nullptr;
volatile bool g_runHeartbeat = false;
volatile system_tick_t g_heartbeatMaxGap = 0;

bool g_hostOsReceived = false;
bool g_hostStrandsCdcIn = false;

hal_usb_recovery_stats g_before = {};

hal_usb_recovery_stats recoveryStats() {
    hal_usb_recovery_stats s = {};
    s.size = sizeof(s);
    SPARK_ASSERT(hal_usb_get_recovery_stats(&s, nullptr) == 0);
    return s;
}

void maskSetupPhase() {
    // The SDK reads EP0 OUT interrupts masked with DOEPMSK and sets the mask again only on a bus reset: SETUPs reach the
    // RX FIFO but never the stack, so the host's requests go unanswered. The watchdog's repair doesn't restore the mask;
    // only its reattach (and the bus reset that follows) does.
    *TEST_USB_DOEPMSK &= ~TEST_USB_DOEPMSK_STUPM;
}

bool setupPhaseUnmasked() {
    // The reattach only reconnects: the host's bus reset that follows restores the mask
    return (*TEST_USB_DOEPMSK & TEST_USB_DOEPMSK_STUPM) != 0;
}

void setGlobalOutNak() {
    // The core NAKs every OUT packet, including the status stage of the host's control reads, so no control transfer
    // completes. Nothing in the stack clears it but the watchdog's repair.
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGONAK;
}

// Shorter than the EP0 watchdog timeout (3 s), so the watchdog doesn't repair EP0 while the host waits
const system_tick_t GLOBAL_IN_NAK_HOLD_MS = 1000;

void holdGlobalInNak() {
    // The core NAKs every IN token, so the replies the USB task arms on EP0 IN don't go out: a host control read with a
    // short timeout gives up and leaves its reply armed. Once the NAK is cleared, nothing collects the reply until the
    // host's next SETUP, which must cancel it (RtlUsbDriver::cancelControlTransferOnSetup()) rather than let it go out
    // as the data stage of the next control read.
    *TEST_USB_DCTL |= TEST_USB_DCTL_SGINAK;
    delay(GLOBAL_IN_NAK_HOLD_MS);
    *TEST_USB_DCTL |= TEST_USB_DCTL_CGINAK;
}

bool triggerFaultReceived() {
    return g_triggerFaultReceived;
}

void cloudTriggerHandler(const char* name, const char* data, size_t size, ContentType type) {
    String payload(data, size);
    String expectedName = String(String(System.deviceID()) + "/" + USB_RECOVERY_TRIGGER);

    if (expectedName != name) {
        return;
    }

    if (payload == USB_RECOVERY_REPAIR_REATTACH || payload == USB_RECOVERY_REPAIR) {
        g_triggerFaultReceived = true;
    } else if (payload == USB_RECOVERY_SETUP_CANCEL) {
        // Sent from the host half, while no device test runs and the runner isn't using EP0
        holdGlobalInNak();
    } else if (payload == USB_RECOVERY_HOST_LINUX || payload == USB_RECOVERY_HOST_OTHER) {
        // Only Linux test hosts will trigger CDC TX storms, skip if we are not on linux host
        g_hostStrandsCdcIn = (payload == USB_RECOVERY_HOST_LINUX);
        g_hostOsReceived = true;
    }
}

bool waitForCloudTrigger() {
    if (!waitFor(triggerFaultReceived, 30000)) {
        return false;
    }
    // Let test runner start polling the device via EP0 control requests before turning them off
    // Otherwise the test runner side will fail before the device tests start
    delay(2000);

    g_before = recoveryStats();
    return true;
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

test(USB_RECOVERY_00_SETUP) {
    assertTrue(Particle.subscribe(System.deviceID() + "/" + USB_RECOVERY_TRIGGER, cloudTriggerHandler));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(USB_RECOVERY_01_WATCHDOG_REPAIR_AND_REATTACH_TRIGGER) {
    // Cloud sends the repair + reattach trigger
    g_triggerFaultReceived = false;
}

test(USB_RECOVERY_02_WATCHDOG_REPAIR_AND_REATTACH_CHECK) {
    assertTrue(waitForCloudTrigger());
    maskSetupPhase();
    assertTrue(waitFor([] { return recoveryStats().reattaches != g_before.reattaches; }, 15000));

    const auto after = recoveryStats();
    // The repair can't restore the mask, so the watchdog escalated to a reattach, whose bus reset did
    assertEqual(after.ep0_repairs - g_before.ep0_repairs, 1);
    assertEqual(after.ep0_reattaches - g_before.ep0_reattaches, 1);
    assertEqual(after.reattaches - g_before.reattaches, 1);

    // Wait for host bus reset
    assertTrue(waitFor(setupPhaseUnmasked, 5000));
}

test(USB_RECOVERY_03_WATCHDOG_REPAIR_ALONE_TRIGGER) {
    // Reset the trigger, let the cloud sends the repair trigger
    g_triggerFaultReceived = false;
}

test(USB_RECOVERY_04_WATCHDOG_REPAIR_ALONE_CHECK) {
    assertTrue(waitForCloudTrigger());
    setGlobalOutNak();
    assertTrue(waitFor([] { return recoveryStats().ep0_repairs != g_before.ep0_repairs; }, 15000));
    // Long enough for the watchdog to have escalated to a reattach if the repair hadn't worked
    delay(5000);

    const auto after = recoveryStats();
    // The repair cleared the global OUT NAK: no reattach needed
    assertEqual(after.ep0_repairs - g_before.ep0_repairs, 1);
    assertEqual(after.ep0_reattaches - g_before.ep0_reattaches, 0);
    assertEqual(after.reattaches - g_before.reattaches, 0);
    assertFalse(*TEST_USB_DCTL & TEST_USB_DCTL_GONSTS);
}

test(USB_RECOVERY_05_CANCEL_ON_SETUP_TRIGGER) {
    // Do nothing. Host will start the NAK trigger and poll for cancelled SETUP tokens
    g_before = recoveryStats();
}

test(USB_RECOVERY_06_CANCEL_ON_SETUP_CHECK) {
    // Check the setup counter triggered, and none of the others did
    const auto after = recoveryStats();
    
    assertEqual(after.ep0_repairs - g_before.ep0_repairs, 0);
    assertEqual(after.ep0_reattaches - g_before.ep0_reattaches, 0);
    assertEqual(after.reattaches - g_before.reattaches, 0);
    assertEqual(after.setup_cancels - g_before.setup_cancels, 1);
    assertFalse(*TEST_USB_DCTL & TEST_USB_DCTL_GINSTS);
}

test(USB_RECOVERY_07_CDC_TX_STORM_START) {
    assertTrue(waitFor([] { return g_hostOsReceived; }, 10000));
    if (!g_hostStrandsCdcIn) {
        skip();
        return;
    }

    g_before = recoveryStats();

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

test(USB_RECOVERY_08_CDC_TX_STORM_TRIGGER) {
    // Host will trigger the storm while the cdc and heartbeat threads run
    if (!g_hostStrandsCdcIn) {
        skip();
        return;
    }
}

test(USB_RECOVERY_09_CDC_TX_STORM_CHECK) {
    if (!g_hostStrandsCdcIn) {
        skip();
        return;
    }

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

    // The storm happened (the guard fired), and threads at the system thread's priority kept running while the host
    // held the port unread. Other recoveries (EP0 watchdog, reattach) are allowed
    const system_tick_t heartbeatMaxGap = g_heartbeatMaxGap;
    assertLessOrEqual(heartbeatMaxGap, MAX_HEARTBEAT_GAP_MS);
    const auto after = recoveryStats();
    assertMore(after.tx_fifo_empty_storms - g_before.tx_fifo_empty_storms, 0);
}

#endif // HAL_PLATFORM_RTL872X
