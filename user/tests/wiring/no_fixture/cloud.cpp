#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"
#if HAL_PLATFORM_IFAPI
#include "ifapi.h"
#endif // HAL_PLATFORM_IFAPI

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

// Order in which disconnect-related events were received, 0 if not received
struct DisconnectState {
    volatile int cloudDisconnecting = 0;
    volatile int cloudDisconnected = 0;
    volatile int networkDisconnecting = 0;
    volatile int seq = 0;
    void reset() {
        cloudDisconnecting = 0;
        cloudDisconnected = 0;
        networkDisconnecting = 0;
        seq = 0;
    }
};

DisconnectState disconnectState;

}

test(CLOUD_01_Particle_Connect_Does_Not_Block_In_SemiAutomatic_Mode) {
    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    // Switch to SEMI_AUTOMATIC mode
    set_system_mode(SEMI_AUTOMATIC);

    Particle.connect();
    assertFalse(Particle.connected());
    waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
}

test(CLOUD_03_Restore_System_Mode) {
    set_system_mode(AUTOMATIC);
}

#if HAL_PLATFORM_CLOUD_UDP
test(CLOUD_04_socket_errors_do_not_cause_a_full_handshake) {
    const int GET_CLOUD_SOCKET_HANDLE_INTERNAL_ID = 3;

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    sock_handle_t cloudSock = (sock_handle_t)system_internal(GET_CLOUD_SOCKET_HANDLE_INTERNAL_ID, nullptr);
    assertTrue(socket_handle_valid(cloudSock));

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
#if HAL_USE_SOCKET_HAL_POSIX
    assertEqual(0, sock_close(cloudSock));
#else
    assertEqual(0, socket_close(cloudSock));
#endif // HAL_USE_SOCKET_HAL_POSIX
    // Force a publish just in case
    (void)Particle.publish("test", "test");
    assertTrue(waitFor(handshakeState, 120000));
    assertEqual((int)handshakeState.handshakeType, (int)cloud_status_session_resume);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(CLOUD_05_loss_of_cloud_connection_network_disconnects_from_the_cloud_and_resumes_session) {
    const system_tick_t NETWORK_LOSS_DISCONNECT_TIMEOUT = 60 * 1000;
    const system_tick_t NETWORK_CONNECT_TIMEOUT = 5 * 60 * 1000;

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    auto& network = Particle.connectionInterface();
    assertNotEqual(Network, network);

    if_t iface = nullptr;
    assertEqual(0, if_get_by_index((network_interface_t)network, &iface));

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

    // Bring the interface administratively down directly through ifapi, bypassing network_disconnect(),
    // so that the cloud connection loses its network the same way as on a link loss
    assertEqual(0, if_clear_flags(iface, IFF_UP));
    SCOPE_GUARD({
        if_set_flags(iface, IFF_UP);
    });
    bool disconnected = waitFor(Particle.disconnected, NETWORK_LOSS_DISCONNECT_TIMEOUT);
    if_set_flags(iface, IFF_UP);
    assertTrue(disconnected);
    assertTrue(System.waitCondition([&network]() { return network.ready(); }, NETWORK_CONNECT_TIMEOUT));
    assertTrue(waitFor(handshakeState, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual((int)handshakeState.handshakeType, (int)cloud_status_session_resume);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(CLOUD_06_disconnecting_cloud_connection_network_disconnects_from_the_cloud_first_and_resumes_session) {
    const system_tick_t NETWORK_DISCONNECT_TIMEOUT = 60 * 1000;
    const system_tick_t NETWORK_CONNECT_TIMEOUT = 5 * 60 * 1000;

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    auto& network = Particle.connectionInterface();
    assertNotEqual(Network, network);

    auto evHandler = [](system_event_t event, int param, void* ctx) {
        if (event == cloud_status) {
            if (param == cloud_status_handshake || param == cloud_status_session_resume) {
                if (handshakeState.handshakeType == -1) {
                    handshakeState.handshakeType = param;
                }
            } else if (param == cloud_status_disconnecting && !disconnectState.cloudDisconnecting) {
                disconnectState.cloudDisconnecting = ++disconnectState.seq;
            } else if (param == cloud_status_disconnected && !disconnectState.cloudDisconnected) {
                disconnectState.cloudDisconnected = ++disconnectState.seq;
            }
        } else if (event == network_status) {
            if (param == network_status_disconnecting && !disconnectState.networkDisconnecting) {
                disconnectState.networkDisconnecting = ++disconnectState.seq;
            }
        }
    };

    handshakeState.reset();
    disconnectState.reset();
    System.on(cloud_status | network_status, evHandler);
    SCOPE_GUARD({
        System.off(cloud_status | network_status, evHandler);
    });

    // Explicit disconnect of the interface used by the cloud connection should disconnect
    // from the cloud gracefully while the interface is still up, before it is brought down
    network.disconnect();
    // Wait on the event itself, not Particle.disconnected(): the cloud flags are cleared in the system thread before
    // the events get dispatched in the application thread. Events are dispatched in order, so if network_status_disconnecting
    // was generated before cloud_status_disconnected, it's already been recorded by now
    bool disconnected = System.waitCondition([]() { return disconnectState.cloudDisconnected != 0; }, NETWORK_DISCONNECT_TIMEOUT);
    bool networkDisconnected = System.waitCondition([&network]() { return !network.ready(); }, NETWORK_DISCONNECT_TIMEOUT);
    network.connect();
    assertTrue(disconnected);
    assertTrue(networkDisconnected);
    assertNotEqual((int)disconnectState.cloudDisconnecting, 0);
    assertMore((int)disconnectState.cloudDisconnected, (int)disconnectState.cloudDisconnecting);
    // Single interface devices also go through network_status_disconnecting, which should come after the cloud disconnect
    if (disconnectState.networkDisconnecting) {
        assertMore((int)disconnectState.networkDisconnecting, (int)disconnectState.cloudDisconnected);
    }
    assertTrue(System.waitCondition([&network]() { return network.ready(); }, NETWORK_CONNECT_TIMEOUT));
    assertTrue(waitFor(handshakeState, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual((int)handshakeState.handshakeType, (int)cloud_status_session_resume);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

#endif // HAL_PLATFORM_CLOUD_UDP
