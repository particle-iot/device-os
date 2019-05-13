/**
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

#include "spark_wiring_platform.h"
#include "spark_wiring_system.h"
#include "spark_wiring_usbserial.h"
#include "system_task.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_cloud_connection.h"
#include "system_mode.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_update.h"
#include "spark_macros.h"
#include "string.h"
#include "core_hal.h"
#include "system_tick_hal.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "rgbled.h"
#include "service_debug.h"
#include "cellular_hal.h"
#include "system_power.h"
#include "simple_pool_allocator.h"

#include "spark_wiring_network.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_cloud.h"
#include "system_threading.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_led.h"
#include "system_commands.h"

#if HAL_PLATFORM_BLE
#include "ble_hal.h"
#include "system_control_internal.h"

using namespace particle;

#endif /* HAL_PLATFORM_BLE */

using spark::Network;
using particle::LEDStatus;
using particle::CloudDiagnostics;

volatile system_tick_t spark_loop_total_millis = 0;

// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

volatile uint8_t Spark_Error_Count;
volatile uint8_t SYSTEM_POWEROFF;
uint8_t feature_cloud_udp = 0;

static struct SetThreadCurrentFunctionPointers {
    SetThreadCurrentFunctionPointers() {
        set_thread_current_function_pointers((void*)&main_thread_current,
                                             (void*)&system_thread_current,
                                             (void*)&application_thread_current,
                                             nullptr, nullptr);
    }
} s_SetThreadCurrentFunctionPointersInitializer;

ISRTaskQueue SystemISRTaskQueue;

void Network_Setup(bool threaded)
{
#if !PARTICLE_NO_NETWORK
    network_setup(0, 0, 0);

    // don't automatically connect when threaded since we want the thread to start asap
    if ((!threaded && system_mode() == AUTOMATIC) || system_mode()==SAFE_MODE)
    {
        network_connect(0, 0, 0, 0);
    }
#endif

#ifndef SPARK_NO_CLOUD
    //Initialize spark protocol callbacks for all System modes
    Spark_Protocol_Init();
#endif

#if PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_BSOM
    system_cloud_set_inet_family_keepalive(AF_INET, HAL_PLATFORM_BORON_CLOUD_KEEPALIVE_INTERVAL, 0);
#endif // PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_BSOM
}

int cfod_count = 0;

/**
 * Use usb serial ymodem flasher to update firmware.
 */
void manage_serial_flasher()
{
    if(SPARK_FLASH_UPDATE == 3)
    {
        system_firmwareUpdate(&Serial);
    }
}

#ifndef SPARK_NO_CLOUD

namespace {

// LED status for cloud errors indication
class LEDCloudErrorStatus: public LEDStatus {
public:
    explicit LEDCloudErrorStatus(LEDPriority priority) :
            LEDStatus(LED_PATTERN_CUSTOM, priority) {
    }

    void start(uint32_t color, uint8_t count) {
        if (count > 0) {
            setActive(false);
            count_ = count;
            setColor(color);
            on(); // LED is turned on initially
            setActive(true);
        }
    }

protected:
    virtual void update(system_tick_t t) override {
        if (t >= ticks_) {
            // Change state
            switch (state_) {
            case ON:
                off(); // Turn LED off
                break;
            case OFF:
                if (--count_ > 0) {
                    on(); // Turn LED on
                } else {
                    setActive(false); // Stop indication
                }
                break;
            }
        } else {
            ticks_ -= t; // Update timing
        }
    }

private:
    enum State {
        ON,
        OFF,
    };

    State state_;
    uint16_t ticks_;
    uint8_t count_;

    void on() {
        state_ = ON;
        ticks_ = 250;
        LEDStatus::on();
    }

    void off() {
        state_ = OFF;
        ticks_ = 250;
        LEDStatus::off();
    }
};

} // namespace

/**
 * Time in millis of the last cloud connection attempt.
 * The next attempt isn't made until the backoff period has elapsed.
 */
static int cloud_backoff_start = 0;

/**
 * The number of connection attempts.
 */
static uint8_t cloud_failed_connection_attempts = 0;

void cloud_connection_failed()
{
    if (cloud_failed_connection_attempts<255)
        cloud_failed_connection_attempts++;
    cloud_backoff_start = HAL_Timer_Get_Milli_Seconds();
}

inline uint8_t in_cloud_backoff_period()
{
    return (HAL_Timer_Get_Milli_Seconds()-cloud_backoff_start)<backoff_period(cloud_failed_connection_attempts);
}

void handle_cloud_errors()
{
    const uint8_t blinks = Spark_Error_Count;
    Spark_Error_Count = 0;

#if PLATFORM_ID == 0
    network.set_error_count(0); // Reset Error Count
#endif /* PLATFORM_ID == 0 */

    LOG(WARN, "Handling cloud error: %d", (int)blinks);

    // cfod resets in orange since they are soft errors
    // TODO: Spark_Error_Count is never equal to 1
    static LEDCloudErrorStatus ledCloudError(LED_PRIORITY_IMPORTANT);
    ledCloudError.start(blinks > 1 ? RGB_COLOR_ORANGE : RGB_COLOR_RED, blinks);

    // TODO Send the Error Count to Cloud: NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]
}

void handle_cfod()
{
    uint8_t reset = 0;
    system_get_flag(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS, &reset, nullptr);
    if (reset && ++cfod_count >= MAX_FAILED_CONNECTS)
    {
        SPARK_WLAN_RESET = 1;
        WARN("Resetting WLAN due to %d failed connect attempts", MAX_FAILED_CONNECTS);
    }

    if (Internet_Test() < 0)
    {
        WARN("Internet Test Failed!");
        if (reset && ++cfod_count >= MAX_FAILED_CONNECTS)
        {
            SPARK_WLAN_RESET = 1;
            WARN("Resetting WLAN due to %d failed connect attempts", MAX_FAILED_CONNECTS);
        }
        Spark_Error_Count = 2;
    }
    else
    {
        WARN("Internet available, Cloud not reachable!");
        Spark_Error_Count = 3;
    }

    if (reset == 0) {
        CLR_WLAN_WD();
    }
}

/**
 * Establishes a socket connection to the cloud if not already present.
 * - handles connection errors by flashing the LED
 * - attempts to open a socket to the cloud
 * - handles the CFOD
 *
 * On return, SPARK_CLOUD_SOCKETED is set to true if the socket connection was successful.
 */

void establish_cloud_connection()
{
    if (network_ready(0, 0, 0) && !SPARK_WLAN_SLEEP && !SPARK_CLOUD_SOCKETED)
    {
        LED_SIGNAL_START(CLOUD_CONNECTING, NORMAL);
        if (in_cloud_backoff_period())
        {
            return;
        }

#if PLATFORM_ID==PLATFORM_ELECTRON_PRODUCTION
        const CellularNetProvData provider_data = cellular_network_provider_data_get(NULL);
        particle::protocol::connection_properties_t conn_prop = {0};
        conn_prop.size = sizeof(conn_prop);
        conn_prop.keepalive_source = particle::protocol::KeepAliveSource::SYSTEM;
        CLOUD_FN(spark_set_connection_property(particle::protocol::Connection::PING, (provider_data.keepalive * 1000), &conn_prop, nullptr), (void)0);
        spark_cloud_udp_port_set(provider_data.port);
#endif // PLATFORM_ID==PLATFORM_ELECTRON_PRODUCTION

        INFO("Cloud: connecting");
        const auto diag = CloudDiagnostics::instance();
        diag->status(CloudDiagnostics::CONNECTING);
        system_notify_event(cloud_status, cloud_status_connecting);
        diag->connectionAttempt();
        int connect_result = spark_cloud_socket_connect();
        if (connect_result >= 0)
        {
            cfod_count = 0;
            SPARK_CLOUD_SOCKETED = 1;
            INFO("Cloud socket connected");
            // "Connected" event is generated only after a successful handshake
        }
        else
        {
            // TODO: Update the last error diagnostic via CloudDiagnostics::lastError(). Currently,
            // the last error diagnostic is used only for communication errors since we cannot mix
            // HAL and communication error codes without specifying the category of an error
            WARN("Cloud socket connection failed: %d", connect_result);
            SPARK_CLOUD_SOCKETED = 0;

            diag->status(CloudDiagnostics::DISCONNECTED);
            // "Connecting" event should be followed by either "connected" or "disconnected" event
            system_notify_event(cloud_status, cloud_status_disconnected);

            // if the user put the networkin listening mode via the button,
            // the cloud connect may have been cancelled.
            if (SPARK_WLAN_RESET || network_listening(0, 0, 0))
            {
                return;
            }

            cloud_connection_failed();
            handle_cfod();
            /* FIXME: */
#if PLATFORM_ID == 0
            network.set_error_count(Spark_Error_Count);
#endif /* PLATFORM_ID == 0 */
        }

        // Handle errors last to ensure they are shown
        if (Spark_Error_Count > 0)
        {
            handle_cloud_errors();
        }
    }
}

int cloud_handshake()
{
	bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
    feature_cloud_udp = (uint8_t)udp;
	bool presence_announce = !udp;
	int err = Spark_Handshake(presence_announce);
	return err;
}

/**
 * Manages the handshake and cloud events when the cloud has a socket connected.
 * @param force_events
 */
void handle_cloud_connection(bool force_events)
{
    if (SPARK_CLOUD_SOCKETED)
    {
        if (!SPARK_CLOUD_CONNECTED)
        {
            LED_SIGNAL_START(CLOUD_HANDSHAKE, NORMAL);
            int err = cloud_handshake();
            if (err)
            {
                if (!SPARK_WLAN_RESET && !network_listening(0, 0, 0))
                {
                    cloud_connection_failed();
                    uint32_t color = RGB_COLOR_RED;
                    if (particle::protocol::DECRYPTION_ERROR==err)
                        color = RGB_COLOR_ORANGE;
                    else if (particle::protocol::AUTHENTICATION_ERROR==err)
                        color = RGB_COLOR_MAGENTA;
                    WARN("Cloud handshake failed, code=%d", err);
                    LEDStatus led(color, LED_PRIORITY_IMPORTANT);
                    led.setActive();
                    // delay a little to be sure the user sees the LED color, since
                    // the socket may quickly disconnect and the connection retried, turning
                    // the LED back to cyan
                    system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                    // allow time for the LED to be flashed
                    while ((HAL_Timer_Get_Milli_Seconds()-start)<250);
                }
                const auto diag = CloudDiagnostics::instance();
                diag->lastError(err);
                cloud_disconnect();
            }
            else
            {
                INFO("Cloud connected");
                SPARK_CLOUD_CONNECTED = 1;
                cloud_failed_connection_attempts = 0;
                CloudDiagnostics::instance()->status(CloudDiagnostics::CONNECTED);
                system_notify_event(cloud_status, cloud_status_connected);
                if (system_mode() == SAFE_MODE) {
/* FIXME: there should be macro that checks for NetworkManager availability */
                    // Connected to the cloud while in safe mode
#if !HAL_PLATFORM_IFAPI
                    LED_SIGNAL_START(SAFE_MODE, BACKGROUND);
#else
                    LED_SIGNAL_START(SAFE_MODE, NORMAL);
#endif /* !HAL_PLATFORM_IFAPI */
                } else {
/* FIXME: there should be macro that checks for NetworkManager availability */
#if !HAL_PLATFORM_IFAPI
                    LED_SIGNAL_START(CLOUD_CONNECTED, BACKGROUND);
#else
                    LED_SIGNAL_START(CLOUD_CONNECTED, NORMAL);
#endif /* !HAL_PLATFORM_IFAPI */
                }
                LED_SIGNAL_STOP(CLOUD_CONNECTING);
            }
            LED_SIGNAL_STOP(CLOUD_HANDSHAKE);
        }
        if (SPARK_FLASH_UPDATE || force_events || System.mode() != MANUAL || system_thread_get_state(NULL)==spark::feature::ENABLED)
        {
            Spark_Process_Events();
        }
    }
}

void manage_cloud_connection(bool force_events)
{
    if (spark_cloud_flag_auto_connect() == 0)
    {
        cloud_disconnect_graceful(true, CLOUD_DISCONNECT_REASON_USER);
    }
    else // cloud connection is wanted
    {
        establish_cloud_connection();

        handle_cloud_connection(force_events);
    }
}
#endif // !SPARK_NO_CLOUD

static void process_isr_task_queue()
{
    SystemISRTaskQueue.process();
}

#if Wiring_SetupButtonUX
extern void system_handle_button_clicks(bool isIsr);
#endif

void Spark_Idle_Events(bool force_events/*=false*/)
{
    HAL_Notify_WDT();

    ON_EVENT_DELTA();
    spark_loop_total_millis = 0;

    process_isr_task_queue();

    if (!SYSTEM_POWEROFF) {

#if Wiring_SetupButtonUX
        system_handle_button_clicks(false /* isIsr */);
#endif
        manage_serial_flasher();

        manage_network_connection();

        manage_smart_config();

        manage_ip_config();

        CLOUD_FN(manage_cloud_connection(force_events), (void)0);

// FIXME: there should be a separate feature macro
#if HAL_PLATFORM_FILESYSTEM
        particle::system::fetchAndExecuteCommand(millis());
#endif // HAL_PLATFORM_FILESYSTEM
    }
    else
    {
        system_pending_shutdown();
    }
#if HAL_PLATFORM_BLE
    // TODO: Process BLE channel events in a separate thread
    system::SystemControl::instance()->run();
#endif
    system_shutdown_if_needed();
}

/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void system_delay_pump(unsigned long ms, bool force_no_background_loop=false)
{
    if (ms==0) return;

    system_tick_t spark_loop_elapsed_millis = SPARK_LOOP_DELAY_MILLIS;
    spark_loop_total_millis += ms;

    system_tick_t start_millis = HAL_Timer_Get_Milli_Seconds();
    system_tick_t end_micros = HAL_Timer_Get_Micro_Seconds() + (1000*ms);

    while (1)
    {
        HAL_Notify_WDT();

        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;

        if (elapsed_millis > ms)
        {
            break;
        }
        else if (elapsed_millis >= (ms-1)) {
            // on the last millisecond, resolve using millis - we don't know how far in that millisecond had come
            // have to be careful with wrap around since start_micros can be greater than end_micros.

            for (;;)
            {
                system_tick_t delay = end_micros-HAL_Timer_Get_Micro_Seconds();
                if (delay>100000)
                    return;
                HAL_Delay_Microseconds(min(delay/2, 1u));
            }
        }
        else
        {
            HAL_Delay_Milliseconds(1);
        }

        if (SPARK_WLAN_SLEEP || force_no_background_loop)
        {
            //Do not yield for Spark_Idle()
        }
        else if ((elapsed_millis >= spark_loop_elapsed_millis) || (spark_loop_total_millis >= SPARK_LOOP_DELAY_MILLIS))
        {
        		bool threading = system_thread_get_state(nullptr);
            spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
            //spark_loop_total_millis is reset to 0 in Spark_Idle()
            do
            {
                //Run once if the above condition passes
                spark_process();
            }
            while (!threading && SPARK_FLASH_UPDATE); //loop during OTA update
        }
    }
}

/**
 * On a non threaded platform, or when called from the application thread, then
 * run the background loop so that application events are processed.
 */
void system_delay_ms(unsigned long ms, bool force_no_background_loop=false)
{
	// if not threading, or we are the application thread, then implement delay
	// as a background message pump

    if ((!PLATFORM_THREADING || APPLICATION_THREAD_CURRENT()) && !HAL_IsISR())
    {
    		system_delay_pump(ms, force_no_background_loop);
    }
    else
    {
        HAL_Delay_Milliseconds(ms);
    }
}

void cloud_disconnect_graceful(bool closeSocket, cloud_disconnect_reason reason)
{
    cloud_disconnect(closeSocket, true, reason);
}

void cloud_disconnect(bool closeSocket, bool graceful, cloud_disconnect_reason reason)
{
#ifndef SPARK_NO_CLOUD

    if (SPARK_CLOUD_SOCKETED || SPARK_CLOUD_CONNECTED)
    {
        INFO("Cloud: disconnecting");
        const auto diag = CloudDiagnostics::instance();
        if (SPARK_CLOUD_CONNECTED)
        {
            diag->resetConnectionAttempts();
            if (reason != CLOUD_DISCONNECT_REASON_NONE) {
                diag->disconnectionReason(reason);
                if (reason == CLOUD_DISCONNECT_REASON_ERROR) {
                    diag->disconnectedUnexpectedly();
                }
            }
            diag->status(CloudDiagnostics::DISCONNECTING);
            // "Disconnecting" event is generated only for a successfully established connection (including handshake)
            system_notify_event(cloud_status, cloud_status_disconnecting);
        }

        if (closeSocket)
            spark_cloud_socket_disconnect(graceful);

        SPARK_FLASH_UPDATE = 0;
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;

        LED_SIGNAL_STOP(CLOUD_CONNECTED);
        LED_SIGNAL_STOP(CLOUD_HANDSHAKE);
        LED_SIGNAL_STOP(CLOUD_CONNECTING);

        INFO("Cloud: disconnected");
        diag->status(CloudDiagnostics::DISCONNECTED);
        system_notify_event(cloud_status, cloud_status_disconnected);
    }
    Spark_Error_Count = 0;  // this is also used for CFOD/WiFi reset, and blocks the LED when set.

#endif
}

uint8_t application_thread_current(void* reserved)
{
    return APPLICATION_THREAD_CURRENT();
}

uint8_t system_thread_current(void* reserved)
{
    return SYSTEM_THREAD_CURRENT();
}

uint8_t main_thread_current(void* reserved)
{
#if PLATFORM_THREADING == 1
    static std::thread::id _thread_id = std::this_thread::get_id();
    return _thread_id == std::this_thread::get_id();
#else
    return true;
#endif
}

uint8_t application_thread_invoke(void (*callback)(void* data), void* data, void* reserved)
{
    // FIXME: We need a way to report an error back to caller, if asynchronous function call can't
    // be scheduled for some reason
    APPLICATION_THREAD_CONTEXT_ASYNC_RESULT(application_thread_invoke(callback, data, reserved), 0);
    callback(data);
    return 0;
}

void cancel_connection()
{
    // Cancel current network connection attempt
    network_connect_cancel(0, 1, 0, 0);
    // Abort cloud connection
    Spark_Abort();
}

namespace {

// Memory pool for small and short-lived allocations
SimpleAllocedPool g_memPool(512);

} // namespace

void* system_pool_alloc(size_t size, void* reserved) {
    void *ptr = nullptr;
    ATOMIC_BLOCK() {
        ptr = g_memPool.allocate(size);
    }
    return ptr;
}

void system_pool_free(void* ptr, void* reserved) {
    ATOMIC_BLOCK() {
        g_memPool.deallocate(ptr);
    }
}

int system_invoke_event_handler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* event_data, void* reserved)
{
#if HAL_PLATFORM_MESH
	invokeEventHandler(handlerInfoSize, handlerInfo, event_name, event_data, reserved);
	return SYSTEM_ERROR_NONE;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // HAL_PLATFORM_MESH
}
