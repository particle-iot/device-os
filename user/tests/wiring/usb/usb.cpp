#include "application.h"
#include "unit-test/unit-test.h"

namespace {

constexpr unsigned END_BEGIN_STRESS_ITERATIONS = 5;

} // namespace

// Most of this suite is driven from the host side: system control requests,
// USB descriptor reads and bus resets are served by the system regardless of
// which device-side test is running. The empty bodies below keep the runner's
// device/host test pairing intact.

test(USB_00_SystemEchoShort) {
}

test(USB_01_SystemEchoLarge) {
}

test(USB_02_SystemEchoConcurrent) {
}

test(USB_03_AppCustomEcho) {
}

test(USB_04_AppCustomEchoDeferred) {
}

test(USB_05_DiagnosticInfo) {
}

test(USB_06_RawVendorRequests) {
    // The host compares the raw SYSTEM_VERSION vendor request against this
    String version = System.version();
    assertEqual(0, pushMailboxMsg(version.c_str(), 10000));
}

test(USB_07_ServiceRequestCancellation) {
}

test(USB_08_DeviceDescriptor) {
}

test(USB_09_InterfaceLayout) {
}

test(USB_10_MsftOsStringDescriptor) {
}

test(USB_11_WcidCompatIdDescriptor) {
}

test(USB_12_EnterListeningMode) {
}

test(USB_13_DeviceObservesListeningMode) {
    assertTrue(waitFor([] { return (bool)Network.listening(); }, 15000));
}

test(USB_14_DeviceObservesNormalMode) {
    assertTrue(waitForNot([] { return (bool)Network.listening(); }, 15000));
}

test(USB_15_HostBusResetRecovery) {
}

test(USB_16_DeviceEndBeginStress) {
    // Serial.end() re-enumerates USB, which temporarily removes the control
    // interface used by the test runner. RESET_PENDING is the generic
    // notification for an expected USB detach.
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 20000));
    for (unsigned i = 0; i < END_BEGIN_STRESS_ITERATIONS; ++i) {
        Serial.end();
        assertFalse(Serial.isEnabled());
        delay(1000);
        Serial.begin();
        assertTrue(Serial.isEnabled());
        delay(1000);
    }
}

test(USB_17_FinalCdcSanity) {
    assertTrue(waitFor(Serial.isConnected, 30000));
    constexpr char data[] = "usb-final";
    assertEqual(Serial.write((const uint8_t*)data, sizeof(data) - 1), sizeof(data) - 1);
    Serial.flush();
}
