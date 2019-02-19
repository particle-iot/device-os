/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSTEM_NETWORK_INTERNAL_H
#define	SYSTEM_NETWORK_INTERNAL_H

#include <stdint.h>
#include "spark_macros.h"
#include "hal_platform.h"
#include "timer_hal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void manage_network_connection();
void manage_smart_config();
void manage_ip_config();

enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(30),
    DISCONNECT_TO_RECONNECT = S2M(2),
};

extern volatile uint8_t SPARK_WLAN_RESET;
extern volatile uint8_t SPARK_WLAN_SLEEP;
extern volatile uint8_t SPARK_WLAN_STARTED;

extern uint32_t wlan_watchdog_duration;
extern uint32_t wlan_watchdog_base;

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif


inline void ARM_WLAN_WD(uint32_t x) {
    wlan_watchdog_base = HAL_Timer_Get_Milli_Seconds();
    wlan_watchdog_duration = x;
    WAN_WD_DEBUG("WD Set %d",(x));
}
inline bool WLAN_WD_TO() {
    return wlan_watchdog_duration && ((HAL_Timer_Get_Milli_Seconds()-wlan_watchdog_base)>wlan_watchdog_duration);
}

inline void CLR_WLAN_WD() {
    wlan_watchdog_duration = 0;
    WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog_duration);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* FIXME: there should be a define that tells whether there is NetworkManager available
 * or not */
#if !HAL_PLATFORM_IFAPI

#include "system_setup.h"
#include "rgbled.h"
#include "spark_wiring_led.h"
#include "spark_wiring_ticks.h"
#include "spark_wiring_diagnostics.h"
#include "system_event.h"
#include "system_cloud_internal.h"
#include "system_network.h"
#include "system_threading.h"
#include "system_mode.h"
#include "system_power.h"

#if HAL_PLATFORM_BLE
#include "ble_hal.h"
#endif // HAL_PLATFORM_BLE

// FIXME
#include "system_control_internal.h"

using namespace particle;

// #define DEBUG_NETWORK_STATE
#ifdef DEBUG_NETWORK_STATE
class ManagedNetworkInterface;

// Helper class dumping internal state of a ManagedNetworkInterface instance
class NetworkStateLogger {
public:
    NetworkStateLogger(const ManagedNetworkInterface& nif, const char* func);
    ~NetworkStateLogger();

private:
    const ManagedNetworkInterface& nif_;
    const char* const func_;

    void dump() const;
};

#define LOG_NETWORK_STATE() \
        const NetworkStateLogger PP_CAT(_networkStateLogger_, __COUNTER__)(*this, __PRETTY_FUNCTION__)

#else // !defined(DEBUG_NETWORK_STATE)
#define LOG_NETWORK_STATE()
#endif

namespace particle {

class NetworkDiagnostics {
public:
    // Note: Use odd numbers to encode transitional states
    enum Status {
        TURNED_OFF = 0,
        TURNING_ON = 1,
        DISCONNECTED = 2,
        CONNECTING = 3,
        CONNECTED = 4,
        DISCONNECTING = 5,
        TURNING_OFF = 7
    };

    NetworkDiagnostics() :
            status_(DIAG_ID_NETWORK_CONNECTION_STATUS, DIAG_NAME_NETWORK_CONNECTION_STATUS, TURNED_OFF),
            disconnReason_(DIAG_ID_NETWORK_DISCONNECTION_REASON, DIAG_NAME_NETWORK_DISCONNECTION_REASON, NETWORK_DISCONNECT_REASON_NONE),
            disconnCount_(DIAG_ID_NETWORK_DISCONNECTS, DIAG_NAME_NETWORK_DISCONNECTS),
            connCount_(DIAG_ID_NETWORK_CONNECTION_ATTEMPTS, DIAG_NAME_NETWORK_CONNECTION_ATTEMPTS),
            lastError_(DIAG_ID_NETWORK_CONNECTION_ERROR_CODE, DIAG_NAME_NETWORK_CONNECTION_ERROR_CODE) {
    }

    NetworkDiagnostics& status(Status status) {
        status_ = status;
        return *this;
    }

    NetworkDiagnostics& connectionAttempt() {
        ++connCount_;
        return *this;
    }

    NetworkDiagnostics& resetConnectionAttempts() {
        connCount_ = 0;
        return *this;
    }

    NetworkDiagnostics& disconnectionReason(network_disconnect_reason reason) {
        disconnReason_ = reason;
        return *this;
    }

    NetworkDiagnostics& disconnectedUnexpectedly() {
        ++disconnCount_;
        return *this;
    }

    NetworkDiagnostics& lastError(int error) {
        lastError_ = error;
        return *this;
    }

    static NetworkDiagnostics* instance();

private:
    // Some of the diagnostic data sources use the synchronization since they can be updated from
    // the networking service thread
    AtomicEnumDiagnosticData<Status> status_;
    AtomicEnumDiagnosticData<network_disconnect_reason> disconnReason_;
    AtomicIntegerDiagnosticData disconnCount_;
    SimpleIntegerDiagnosticData connCount_;
    SimpleIntegerDiagnosticData lastError_;
};

} // namespace particle

/**
 * Internal network interface class to provide polymorphic behavior for each
 * network type.  This is not part of the dynalib so functions can freely evolve.
 */
struct NetworkInterface
{

    virtual network_interface_t network_interface()=0;
    virtual void setup()=0;

    virtual void on()=0;
    virtual void off(bool disconnect_cloud=false)=0;
    virtual void connect(bool listen_enabled=true)=0;
    virtual bool connecting()=0;
    virtual void connect_cancel(bool cancel)=0;
    /**
     * Force a manual disconnct.
     */
    virtual void disconnect(network_disconnect_reason reason) = 0;

    /**
     * @return {@code true} if this connection was manually taken down.
     */
    virtual bool manual_disconnect()=0;
    virtual void listen(bool stop=false)=0;
    virtual void listen_loop()=0;
    virtual bool listening()=0;
    virtual void set_listen_timeout(uint16_t timeout)=0;
    virtual uint16_t get_listen_timeout()=0;
    /**
     * Perform the 10sec press command, e.g. clear credentials.
     */
    virtual void listen_command()=0;
    virtual bool ready()=0;

    virtual bool clear_credentials()=0;
    virtual bool has_credentials()=0;
    virtual int set_credentials(NetworkCredentials* creds)=0;

    virtual void config_clear()=0;
    virtual void update_config(bool force=false)=0;
    virtual void* config()=0;       // not really happy about lack of type

    virtual int set_hostname(const char* hostname)=0;
    virtual int get_hostname(char* buffer, size_t buffer_len, bool noDefault=false)=0;

};


class ManagedNetworkInterface : public NetworkInterface
{
private:
    volatile uint8_t WLAN_DISCONNECT;
    volatile uint8_t WLAN_DELETE_PROFILES;
    volatile uint8_t WLAN_SMART_CONFIG_START; // Set to 'true' when listening mode is pending
    volatile uint8_t WLAN_SMART_CONFIG_ACTIVE;
    volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
    volatile uint8_t WLAN_CONNECTED;
    volatile uint8_t WLAN_CONNECTING;
    volatile uint8_t WLAN_DHCP_PENDING;
    volatile uint8_t WLAN_CAN_SHUTDOWN;
    volatile uint8_t WLAN_LISTEN_ON_FAILED_CONNECT;
#if PLATFORM_ID == 10 // Electron
    volatile uint32_t START_LISTENING_TIMER_MS = 300000UL; // 5 minute default on Electron
#else
    volatile uint32_t START_LISTENING_TIMER_MS = 0UL; // Disabled by default on Photon/P1/Core
#endif
    volatile uint32_t start_listening_timer_base;
    volatile uint32_t start_listening_timer_duration;

#ifdef DEBUG_NETWORK_STATE
    friend class NetworkStateLogger;
#endif

protected:

    volatile uint8_t WLAN_SERIAL_CONFIG_DONE;
    virtual network_interface_t network_interface() override { return 0; }
    virtual void start_listening()=0;

    void start_listening_timer_create() {
        if (START_LISTENING_TIMER_MS != 0) {
            start_listening_timer_base = HAL_Timer_Get_Milli_Seconds();
            start_listening_timer_duration = START_LISTENING_TIMER_MS;
            LOG(INFO,"Start Listening timer: created");
        }
    }

    void start_listening_timer_update(uint16_t timeout) {
        if (ManagedNetworkInterface::listening()) {
            if (START_LISTENING_TIMER_MS != 0) {
                start_listening_timer_create();
            }
            else {
                start_listening_timer_destroy();
            }
        }
    }

    bool is_start_listening_timeout()
    {
        return start_listening_timer_duration && ((HAL_Timer_Get_Milli_Seconds()-start_listening_timer_base)>start_listening_timer_duration);
    }

    void start_listening_timeout()
    {
        if (ManagedNetworkInterface::listening()) {
            ManagedNetworkInterface::listen(true);
            LOG(INFO,"Start listening timer: timeout");
        }
    }

    void start_listening_timer_destroy(void)
    {
        if (start_listening_timer_duration) {
            start_listening_timer_duration = 0UL;
            LOG(INFO,"Start listening timer: destroyed");
        }
    }

    template<typename T> void start_listening(SystemSetupConsole<T>& console)
    {
        LOG_NETWORK_STATE();
        WLAN_SMART_CONFIG_ACTIVE = 1;
        WLAN_SMART_CONFIG_FINISHED = 0;
        WLAN_SERIAL_CONFIG_DONE = 0;
        bool wlanStarted = SPARK_WLAN_STARTED;

        cloud_disconnect(true, false, CLOUD_DISCONNECT_REASON_LISTENING);

        if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
            LED_SIGNAL_START(LISTENING_MODE, NORMAL);
        } else {
            // Using critical priority here, since in a single-threaded configuration the listening
            // mode blocks an application code from running
            LED_SIGNAL_START(LISTENING_MODE, CRITICAL);
        }

#if HAL_PLATFORM_BLE
        // Start advertising
        ble_start_advert(nullptr);
#endif // HAL_PLATFORM_BLE

        on_start_listening();
        start_listening_timer_create();

        const uint32_t start = millis();
        uint32_t loop = start;
        system_notify_event(wifi_listen_begin, 0);

        /* Wait for SmartConfig/SerialConfig to finish */
        while (WLAN_SMART_CONFIG_ACTIVE && !WLAN_SMART_CONFIG_FINISHED && !WLAN_SERIAL_CONFIG_DONE)
        {
            if (WLAN_DELETE_PROFILES)
            {
                // Get base color used for the listening mode indication
                const LEDStatusData* status = led_signal_status(LED_SIGNAL_LISTENING_MODE, nullptr);
                LEDStatus led(status ? status->color : RGB_COLOR_BLUE, LED_PRIORITY_CRITICAL);
                led.setActive();
                int toggle = 25;
                while (toggle--)
                {
                    led.toggle();
                    HAL_Delay_Milliseconds(50);
                }
                if (!network_clear_credentials(0, 0, NULL, NULL) || network_has_credentials(0, 0, NULL)) {
                    led.setColor(RGB_COLOR_RED);
                    led.on();

                    int toggle = 25;
                    while (toggle--)
                    {
                        led.toggle();
                        HAL_Delay_Milliseconds(50);
                    }
                }
                system_notify_event(network_credentials, network_credentials_cleared);
                WLAN_DELETE_PROFILES = 0;
            }
            else
            {
                uint32_t now = millis();
                if ((now-loop)>1000) {
                    loop = now;
                    system_notify_event(wifi_listen_update, now-start);
                }
                console.loop();
            }
#if PLATFORM_THREADING
            SystemISRTaskQueue.process();
            if (!APPLICATION_THREAD_CURRENT()) {
                SystemThread.process();
            }
#endif
            if (is_start_listening_timeout()) {
                start_listening_timeout();
            }
#if HAL_PLATFORM_BLE
            // TODO: Process BLE channel events in a separate thread
            system::SystemControl::instance()->run();
#endif
        // while (network_listening(0, 0, NULL))
        } start_listening_timer_destroy(); // immediately destroy timer if we are on our way out

#if HAL_PLATFORM_BLE
        // Stop advertising
        ble_stop_advert(nullptr);
#endif // HAL_PLATFORM_BLE

        LED_SIGNAL_STOP(LISTENING_MODE);

        WLAN_LISTEN_ON_FAILED_CONNECT = on_stop_listening() && wlanStarted;

        on_finalize_listening(WLAN_SMART_CONFIG_FINISHED);

        system_notify_event(wifi_listen_end, millis()-start);

        WLAN_SMART_CONFIG_ACTIVE = 0;
        if (has_credentials()) {
            connect();
        }
        else if (!wlanStarted) {
            off();
        }
    }

    virtual void on_start_listening()=0;
    /**
     *
     * @param setup
     * @return true if network credentials were configured.
     */
    virtual bool on_stop_listening()=0;

    virtual void on_setup_cleanup()=0;

    virtual void connect_init()=0;
    virtual int connect_finalize()=0;
    virtual void disconnect_now()=0;

    virtual int on_now()=0;
    virtual void off_now()=0;

    /**
     *
     * @param external_process_complete If some external process triggered exit of listen mode.
     */
    virtual void on_finalize_listening(bool external_process_complete)=0;

    virtual void config_hostname() {
        char hostname[33] = {0};
        if (get_hostname(hostname, sizeof(hostname), true) || strlen(hostname) == 0)
        {
            String deviceId = spark_deviceID();
            set_hostname(deviceId.c_str());
        }
    }

public:

    virtual void get_ipconfig(IPConfig* config)=0;

    virtual void set_error_count(unsigned count)=0;

    bool manual_disconnect() override
    {
        return WLAN_DISCONNECT;
    }

    void set_manual_disconnect(bool disconnect)
    {
        WLAN_DISCONNECT = disconnect;
    }

    void listen(bool stop=false) override
    {
        LOG_NETWORK_STATE();
        if (stop) {
            WLAN_LISTEN_ON_FAILED_CONNECT = 0;  // ensure a failed wifi connection attempt doesn't bring the device back to listening mode
            WLAN_SMART_CONFIG_START = 0; // Cancel pending transition to listening mode
            WLAN_SMART_CONFIG_ACTIVE = 0; // Break current listening loop
        } else if (!WLAN_SMART_CONFIG_ACTIVE) {
            WLAN_SMART_CONFIG_START = 1;
        }
    }

    void listen_command() override
    {
        WLAN_DELETE_PROFILES = 1;
    }

    bool listening() override
    {
        return (WLAN_SMART_CONFIG_START || WLAN_SMART_CONFIG_ACTIVE);
    }

    void set_listen_timeout(uint16_t timeout) override {
        START_LISTENING_TIMER_MS = timeout * 1000UL;
        start_listening_timer_update(timeout);
    }

    uint16_t get_listen_timeout() override {
        return START_LISTENING_TIMER_MS/1000UL;
    }

    void connect(bool listen_enabled=true) override
    {
        LOG_NETWORK_STATE();
        // INFO("ready(): %d; connecting(): %d; listening(): %d; WLAN_SMART_CONFIG_START: %d", (int)ready(), (int)connecting(),
        //        (int)listening(), (int)WLAN_SMART_CONFIG_START);
        if (!ready() && !connecting() && !listening() && !WLAN_SMART_CONFIG_START) // Don't try to connect if listening mode is active or pending
        {
            bool was_sleeping = SPARK_WLAN_SLEEP;

            on(); // activate WiFi

            WLAN_DISCONNECT = 0;
            connect_init();
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;

            if (!has_credentials())
            {
                if (listen_enabled) {
                    listen();
                }
                else if (was_sleeping) {
                    disconnect();
                }
            }
            else
            {
                config_hostname();
                LED_SIGNAL_START(NETWORK_CONNECTING, NORMAL);
                WLAN_CONNECTING = 1;
                INFO("ARM_WLAN_WD 1");
                ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);    // reset the network if it doesn't connect within the timeout
                const auto diag = NetworkDiagnostics::instance();
                diag->status(NetworkDiagnostics::CONNECTING);
                system_notify_event(network_status, network_status_connecting);
                diag->connectionAttempt();
                const int ret = connect_finalize();
                if (ret != 0) {
                    diag->lastError(ret);
                }
            }
        }
    }

    void disconnect(network_disconnect_reason reason = NETWORK_DISCONNECT_REASON_NONE) override
    {
        LOG_NETWORK_STATE();
        if (SPARK_WLAN_STARTED)
        {
            const bool was_connected = WLAN_CONNECTED;
            const bool was_connecting = WLAN_CONNECTING;
            WLAN_DISCONNECT = 1; //Do not ARM_WLAN_WD() in WLAN_Async_Callback()
            WLAN_CONNECTING = 0;
            WLAN_CONNECTED = 0;
            WLAN_DHCP_PENDING = 0;

            cloud_disconnect(true, false, CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT);
            const auto diag = NetworkDiagnostics::instance();
            if (was_connected) {
                diag->resetConnectionAttempts();
                if (reason != NETWORK_DISCONNECT_REASON_NONE) {
                    diag->disconnectionReason(reason);
                    if (reason == NETWORK_DISCONNECT_REASON_ERROR || reason == NETWORK_DISCONNECT_REASON_RESET) {
                        diag->disconnectedUnexpectedly();
                    }
                }
                diag->status(NetworkDiagnostics::DISCONNECTING);
                // "Disconnecting" event is generated only for a successfully established connection
                system_notify_event(network_status, network_status_disconnecting);
            }
            disconnect_now();
            config_clear();
            if (was_connected || was_connecting) {
                diag->status(NetworkDiagnostics::DISCONNECTED);
                system_notify_event(network_status, network_status_disconnected);
            }
            LED_SIGNAL_STOP(NETWORK_CONNECTED);
            LED_SIGNAL_STOP(NETWORK_DHCP);
            LED_SIGNAL_STOP(NETWORK_CONNECTING);
        }
    }

    bool ready() override
    {
        return (SPARK_WLAN_STARTED && WLAN_CONNECTED);
    }

    bool connecting() override
    {
        return (SPARK_WLAN_STARTED && WLAN_CONNECTING);
    }

    void on() override
    {
        LOG_NETWORK_STATE();
        if (!SPARK_WLAN_STARTED)
        {
            const auto diag = NetworkDiagnostics::instance();
            diag->status(NetworkDiagnostics::TURNING_ON);
            system_notify_event(network_status, network_status_powering_on);
            config_clear();
            const int ret = on_now();
            if (ret != 0) {
                diag->lastError(ret);
            }
            update_config(true);
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;
            LED_SIGNAL_START(NETWORK_ON, BACKGROUND);
            diag->status(NetworkDiagnostics::DISCONNECTED);
            system_notify_event(network_status, network_status_on);
        }
    }

    void off(bool disconnect_cloud=false) override
    {
        LOG_NETWORK_STATE();
        if (SPARK_WLAN_STARTED)
        {
            disconnect(NETWORK_DISCONNECT_REASON_NETWORK_OFF);

            const auto diag = NetworkDiagnostics::instance();
            diag->status(NetworkDiagnostics::TURNING_OFF);
            system_notify_event(network_status, network_status_powering_off);
            off_now();

            SPARK_WLAN_SLEEP = 1;
#ifndef SPARK_NO_CLOUD
            if (disconnect_cloud) {
                spark_cloud_flag_disconnect();
            }
#endif
            SPARK_WLAN_STARTED = 0;
            WLAN_DHCP_PENDING = 0;
            WLAN_CONNECTED = 0;
            WLAN_CONNECTING = 0;
            WLAN_SERIAL_CONFIG_DONE = 1;
            LED_SIGNAL_START(NETWORK_OFF, BACKGROUND);
            diag->status(NetworkDiagnostics::TURNED_OFF);
            system_notify_event(network_status, network_status_off);
        }
    }

    void notify_listening_complete()
    {
        LOG_NETWORK_STATE();
        WLAN_SMART_CONFIG_FINISHED = 1;
    }

    void notify_connected()
    {
        LOG_NETWORK_STATE();
        /* If DHCP has completed, don't re-arm WD due to spurious notify_connected()
         * from WICED on loss of internet and reconnect
         */
        if (!WLAN_DISCONNECT && !WLAN_CONNECTED)
        {
            WLAN_DHCP_PENDING = 1;
            INFO("ARM_WLAN_WD 2");
            ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
            LED_SIGNAL_START(NETWORK_DHCP, NORMAL);
        }
    }

    void notify_disconnected()
    {
        LOG_NETWORK_STATE();
        // Don't close the socket on the callback since this causes a lockup on the Core
        cloud_disconnect(false, false, CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT);
        if (WLAN_CONNECTING || WLAN_CONNECTED) {
            // This code is executed only in case of an unsolicited disconnection, since the disconnect() method
            // resets the WLAN_CONNECTING and WLAN_CONNECTED flags prior to closing the connection
            const auto diag = NetworkDiagnostics::instance();
            if (WLAN_CONNECTED) {
                diag->disconnectionReason(NETWORK_DISCONNECT_REASON_ERROR);
                diag->disconnectedUnexpectedly();
                // "Disconnecting" event is generated only for a successfully established connection
                system_notify_event(network_status, network_status_disconnecting);
            }
            diag->status(NetworkDiagnostics::DISCONNECTED);
            // "Connecting" event should be always followed by either "connected" or "disconnected" event
            system_notify_event(network_status, network_status_disconnected);
        }
        // Do not enable WLAN watchdog if WiFi.disconnect() has been called or smart config is active
        if (!WLAN_DISCONNECT && !WLAN_SMART_CONFIG_ACTIVE) {
            INFO("ARM_WLAN_WD 3");
            ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
            // Keep blinking green if automatic reconnection is pending
        } else {
            LED_SIGNAL_STOP(NETWORK_CONNECTING);
        }

        LED_SIGNAL_STOP(NETWORK_CONNECTED);
        LED_SIGNAL_STOP(NETWORK_DHCP);

        WLAN_CONNECTED = 0;
        WLAN_CONNECTING = 0;
        WLAN_DHCP_PENDING = 0;
    }

    void notify_dhcp(bool dhcp)
    {
        LOG_NETWORK_STATE();
        WLAN_CONNECTING = 0;
        WLAN_DHCP_PENDING = 0;
        LED_SIGNAL_STOP(NETWORK_DHCP);
        const auto diag = NetworkDiagnostics::instance();
        if (dhcp)
        {
            // notify_dhcp() is called even in case of static IP configuration, so here we notify
            // final connection state for both dynamic and static IP configurations
            INFO("CLR_WLAN_WD 1, DHCP success");
            CLR_WLAN_WD();
#if PLATFORM_ID != 0
            /* XXX: this causes a deadlock on Core */
            update_config(true);
#endif /* PLATFORM_ID != 0 */
            WLAN_CONNECTED = 1;
            WLAN_LISTEN_ON_FAILED_CONNECT = false;
            LED_SIGNAL_START(NETWORK_CONNECTED, BACKGROUND);
            LED_SIGNAL_STOP(NETWORK_CONNECTING);
            diag->status(NetworkDiagnostics::CONNECTED);
            system_notify_event(network_status, network_status_connected);
        }
        else
        {
            config_clear();
            if (WLAN_LISTEN_ON_FAILED_CONNECT) {
                LED_SIGNAL_STOP(NETWORK_CONNECTING);
                listen();
            } else {
                INFO("DHCP fail, ARM_WLAN_WD 4");
                ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
            }
            diag->status(NetworkDiagnostics::DISCONNECTED);
            // "Connecting" event should be always followed by either "connected" or "disconnected" event
            system_notify_event(network_status, network_status_disconnected);
        }
    }

    void notify_can_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 1;
    }

    void notify_cannot_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 0;
    }


    void listen_loop() override
    {
        if (WLAN_SMART_CONFIG_START)
        {
            WLAN_SMART_CONFIG_START = 0;
            start_listening();
        }

        // Complete Smart Config Process:
        // 1. if smart config is done
        // 2. CC3000 established AP connection
        // 3. DHCP IP is configured
        // then send mDNS packet to stop external SmartConfig application
        if ((WLAN_SMART_CONFIG_FINISHED == 1) && (WLAN_CONNECTED == 1))
        {
            on_setup_cleanup();
            WLAN_SMART_CONFIG_FINISHED = 0;
        }
    }
};

extern ManagedNetworkInterface& network;

template <typename Config, typename C>
class ManagedIPNetworkInterface : public ManagedNetworkInterface
{
    Config ip_config;

public:

    void get_ipconfig(IPConfig* config) override
    {
        update_config(true);
        memcpy(config, this->config(), config->size);
    }

    void update_config(bool force=false) override
    {
        // todo - IPv6 may not set this field.
        bool fetched_config = ip_config.nw.aucIP.ipv4!=0;
        if (ready() || force)
        {
            if (!fetched_config || force)
            {
                memset(&ip_config, 0, sizeof(ip_config));
                ip_config.size = sizeof(ip_config);
                reinterpret_cast<C*>(this)->fetch_ipconfig(&ip_config);
            }
        }
        else if (fetched_config)
        {
            config_clear();
        }
    }

    void config_clear() override
    {
        memset(&ip_config, 0, sizeof(ip_config));
    }

    void* config() override  { return &ip_config; }

};

#ifdef DEBUG_NETWORK_STATE
#define NETWORK_STATE_PRINTF(...) \
        do { \
            LOG_PRINTF_C(TRACE, "system.network.state", ##__VA_ARGS__); \
        } while (false)

inline NetworkStateLogger::NetworkStateLogger(const ManagedNetworkInterface& nif, const char* func) :
        nif_(nif),
        func_(func) {
    NETWORK_STATE_PRINTF("-> %s\r\n", func_);
    dump();
}

inline NetworkStateLogger::~NetworkStateLogger() {
    NETWORK_STATE_PRINTF("<- %s\r\n", func_);
    dump();
}

inline void NetworkStateLogger::dump() const {
    NETWORK_STATE_PRINTF("WLAN_DISCONNECT: %d\r\n", (int)nif_.WLAN_DISCONNECT);
    NETWORK_STATE_PRINTF("WLAN_DELETE_PROFILES: %d\r\n", (int)nif_.WLAN_DELETE_PROFILES);
    NETWORK_STATE_PRINTF("WLAN_SMART_CONFIG_START: %d\r\n", (int)nif_.WLAN_SMART_CONFIG_START);
    NETWORK_STATE_PRINTF("WLAN_SMART_CONFIG_ACTIVE: %d\r\n", (int)nif_.WLAN_SMART_CONFIG_ACTIVE);
    NETWORK_STATE_PRINTF("WLAN_SMART_CONFIG_FINISHED: %d\r\n", (int)nif_.WLAN_SMART_CONFIG_FINISHED);
    NETWORK_STATE_PRINTF("WLAN_CONNECTED: %d\r\n", (int)nif_.WLAN_CONNECTED);
    NETWORK_STATE_PRINTF("WLAN_CONNECTING: %d\r\n", (int)nif_.WLAN_CONNECTING);
    NETWORK_STATE_PRINTF("WLAN_DHCP_PENDING: %d\r\n", (int)nif_.WLAN_DHCP_PENDING);
    NETWORK_STATE_PRINTF("WLAN_CAN_SHUTDOWN: %d\r\n", (int)nif_.WLAN_CAN_SHUTDOWN);
    NETWORK_STATE_PRINTF("WLAN_LISTEN_ON_FAILED_CONNECT: %d\r\n", (int)nif_.WLAN_LISTEN_ON_FAILED_CONNECT);
    // Global flags
    NETWORK_STATE_PRINTF("SPARK_WLAN_RESET: %d\r\n", (int)SPARK_WLAN_RESET);
    NETWORK_STATE_PRINTF("SPARK_WLAN_SLEEP: %d\r\n", (int)SPARK_WLAN_SLEEP);
    NETWORK_STATE_PRINTF("SPARK_WLAN_STARTED: %d\r\n", (int)SPARK_WLAN_STARTED);
    NETWORK_STATE_PRINTF("wlan_watchdog_duration: %d\r\n", (int)wlan_watchdog_duration);
    NETWORK_STATE_PRINTF("--------------------------------\r\n");
}

#undef NETWORK_STATE_PRINTF
#endif // defined(DEBUG_NETWORK_STATE)

#endif /* !HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_NETWORK_INTERNAL_H */
