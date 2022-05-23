#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

namespace {

struct HandshakeState {
    volatile int handshakeType = -1;
    bool operator()() {
        return handshakeType != -1;
    }
    void reset() {
        handshakeType = -1;
    }
};

HandshakeState handshakeState;

}

#if HAL_PLATFORM_CLOUD_UDP
#if HAL_PLATFORM_CELLULAR

test(CLOUD_05_loss_of_cellular_network_connectivity_does_not_cause_full_handshake) {
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    auto evHandler = [](system_event_t event, int param, void* ctx) {
        if (event == cloud_status) {
            if (param == cloud_status_handshake || param == cloud_status_session_resume) {
                if (handshakeState.handshakeType == -1) {
                    handshakeState.handshakeType = param;
                }
            }
        }
    };

    handshakeState.reset();
    System.on(cloud_status, evHandler);
    SCOPE_GUARD({
        System.off(cloud_status, evHandler);
    });
    // Pull the rug, this should cause a socket error on recv/send
#if HAL_PLATFORM_NCP_AT
    // CFUN=0 is the same as CFUN=0,0 for R410.  Done this way because R510 errors with CFUN=0,0
    assertEqual((int)RESP_OK, Cellular.command("AT+CFUN=0\r\n"));
    // Force a publish just in case
    (void)Particle.publish("test", "test");
    assertEqual((int)RESP_OK, Cellular.command("AT+CFUN=1,0\r\n"));
#else
    CellularDevice devInfo = {};
    devInfo.size = sizeof(devInfo);
    assertEqual(0, cellular_device_info(&devInfo, nullptr));
    // R410 based devices don't take too well to being switched to minimum functionality mode
    // and perform a reset. Deactivating internal context or disconnecting (COPS=2) seems
    // to work better.
    if (devInfo.dev == DEV_SARA_R410) {
        assertEqual((int)RESP_OK, Cellular.command("AT+COPS=2,0\r\n"));
        // Force a publish just in case
        (void)Particle.publish("test", "test");
        assertEqual((int)RESP_OK, Cellular.command("AT+COPS=0,0\r\n"));
    } else {
        assertEqual((int)RESP_OK, Cellular.command("AT+UPSDA=0,4\r\n"));
        // Force a publish just in case
        (void)Particle.publish("test", "test");
        assertEqual((int)RESP_OK, Cellular.command("AT+UPSDA=0,3\r\n"));
    }
#endif // HAL_PLATFORM_NCP_AT
    assertTrue(waitFor(handshakeState, 120000));
    assertEqual((int)handshakeState.handshakeType, (int)cloud_status_session_resume);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

#endif // HAL_PLATFORM_CELLULAR
#endif // HAL_PLATFORM_CLOUD_UDP
