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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "logging.h"

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
#include "firmware_update.h"
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
#include "system_ble_prov.h"

#include "spark_wiring_network.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_cloud.h"
#include "system_threading.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_led.h"
#if HAL_PLATFORM_IFAPI
#include "system_listening_mode.h"
#endif
#include "ncp_fw_update.h"

#if HAL_PLATFORM_BLE
#include "ble_hal.h"
#include "system_control_internal.h"
#endif /* HAL_PLATFORM_BLE */

using namespace particle;
using spark::Network;

volatile system_tick_t spark_loop_total_millis = 0;

bool APPLICATION_SETUP_DONE = false;

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

namespace particle {

ISRTaskQueue SystemISRTaskQueue;

} // particle

void Network_Setup(bool threaded)
{
    network_setup(0, 0, 0);

    // don't automatically connect when threaded since we want the thread to start asap
    if ((!threaded && system_mode() == AUTOMATIC) || system_mode()==SAFE_MODE)
    {
        network_connect(0, 0, 0, 0);
    }

    //Initialize spark protocol callbacks for all System modes
    Spark_Protocol_Init();
}

int cfod_count = 0;

/**
 * Use usb serial ymodem flasher to update firmware.
 */
void manage_serial_flasher()
{
    if(SPARK_FLASH_UPDATE == 3) // FIXME: This state variable is no longer set to 3 anywhere in the code
    {
        system_firmwareUpdate(&Serial);
    }
}

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
static system_tick_t cloud_backoff_start = 0;

/**
 * Timestamp (millis) of the first failed connection attempt
 */
static system_tick_t cloud_first_failed_connection = 0;

static const system_tick_t MS_IN_MIN = 60 * 1000;
/**
 * Determines for how long we are going to attempt connecting to the cloud
 * before resetting the network interfaces.
 */
static const system_tick_t NETWORK_RESET_TIMEOUT_ON_CLOUD_CONNECTION_ERRORS = 5 * MS_IN_MIN; // 5 minutes

/**
 * The number of connection attempts.
 */
static uint8_t cloud_failed_connection_attempts = 0;

inline uint8_t in_cloud_backoff_period()
{
    return (HAL_Timer_Get_Milli_Seconds()-cloud_backoff_start)<backoff_period(cloud_failed_connection_attempts);
}

void handle_cloud_errors()
{
    if (Spark_Error_Count == 0) {
        return;
    }

    const uint8_t blinks = Spark_Error_Count;
    Spark_Error_Count = 0;

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

    if (cfod_count++ == 0) {
        cloud_first_failed_connection = HAL_Timer_Get_Milli_Seconds();
    }

    if (reset && (HAL_Timer_Get_Milli_Seconds() - cloud_first_failed_connection) >= NETWORK_RESET_TIMEOUT_ON_CLOUD_CONNECTION_ERRORS) {
        LOG(WARN, "Resetting network interfaces due to failed connection attempts for %u minutes",
                NETWORK_RESET_TIMEOUT_ON_CLOUD_CONNECTION_ERRORS / MS_IN_MIN);
        SPARK_WLAN_RESET = 1;
        cloud_first_failed_connection = 0;
        cfod_count = 0;
        cloud_failed_connection_attempts = 0;
    } else {
        auto r = Internet_Test();
        if (r != 0) {
            const char* reason = "network";
            if (r == SYSTEM_ERROR_NOT_FOUND) {
                reason = "DNS";
            } else if (r == SYSTEM_ERROR_TIMEOUT) {
                reason = "timeout";
            }
            LOG(WARN, "Internet test failed: %s", reason);
            Spark_Error_Count = 2;
        } else {
            LOG(WARN, "Internet available, cloud not reachable");
            Spark_Error_Count = 3;
        }
    }

    if (reset == 0) {
        CLR_WLAN_WD();
    }
}

void cloud_connection_failed()
{
    if (cloud_failed_connection_attempts<255)
        cloud_failed_connection_attempts++;
    cloud_backoff_start = HAL_Timer_Get_Milli_Seconds();

    handle_cfod();
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

        INFO("Cloud: connecting");
        const auto diag = CloudDiagnostics::instance();
        diag->status(CloudDiagnostics::CONNECTING);
        system_notify_event(cloud_status, cloud_status_connecting);
        diag->connectionAttempt();
        int connect_result = spark_cloud_socket_connect();
        if (connect_result >= 0)
        {
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
            if (connect_result == SYSTEM_ERROR_NETWORK) {
                LED_SIGNAL_STOP(CLOUD_HANDSHAKE);
                LED_SIGNAL_STOP(CLOUD_CONNECTING);
            }
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
        }

        // Handle errors last to ensure they are shown
        handle_cloud_errors();
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
        if (!SPARK_CLOUD_CONNECTED && !SPARK_CLOUD_HANDSHAKE_PENDING)
        {
            int err = 0;
            if (SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE) {
                // TODO: There's no protocol API to get the current session state, so we're running
                // one more iteration of the communication loop to make sure all handshake messages
                // have been acknowledged successfully
                if (!Spark_Communication_Loop()) {
                    err = protocol::MESSAGE_TIMEOUT;
                } else {
                    INFO("Cloud connected");
                    SPARK_CLOUD_CONNECTED = 1;
                    SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 0;
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
                    LED_SIGNAL_STOP(CLOUD_HANDSHAKE);
                }
            } else { // !SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE
                LED_SIGNAL_START(CLOUD_HANDSHAKE, NORMAL);
                err = cloud_handshake();
            }
            if (err)
            {
                if (!SPARK_WLAN_RESET && !network_listening(0, 0, 0))
                {
                    cloud_connection_failed();
                    uint32_t color = RGB_COLOR_RED;
                    if (protocol::DECRYPTION_ERROR==err)
                        color = RGB_COLOR_ORANGE;
                    else if (protocol::AUTHENTICATION_ERROR==err)
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
            } else {
                cfod_count = 0;
            }
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
        cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, CLOUD_DISCONNECT_REASON_USER);
    }
    else // cloud connection is wanted
    {
        establish_cloud_connection();

        handle_cloud_connection(force_events);
    }
}

void manage_listening_mode_flag() {
#if HAL_PLATFORM_IFAPI
    // If device is in listening mode and 'FEATURE_FLAG_DISABLE_LISTENING_MODE' is enabled,
    // make sure to come out of listening mode
    if (particle::system::ListeningModeHandler::instance()->isActive() && HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        particle::system::ListeningModeHandler::instance()->exit();
    }
#endif
}

void manage_ble_prov_mode() {
#if HAL_PLATFORM_BLE
    // Check the relevant feature flag. If it's cleared,
    // make sure to turn off prov mode, and clear all
    // its UUIDs and others
    if (system_ble_prov_get_status(nullptr) && !HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        system_ble_prov_mode(false, nullptr);
    }
#endif
}

static void process_isr_task_queue()
{
    SystemISRTaskQueue.process();
}

#if HAL_PLATFORM_SETUP_BUTTON_UX
extern void system_handle_button_clicks(bool isIsr);
#endif

void Spark_Idle_Events(bool force_events/*=false*/)
{
    ON_EVENT_DELTA();
    spark_loop_total_millis = 0;

    process_isr_task_queue();

    if (!SYSTEM_POWEROFF) {

#if HAL_PLATFORM_SETUP_BUTTON_UX
        system_handle_button_clicks(false /* isIsr */);
#endif
        manage_serial_flasher();

        manage_network_connection();

        manage_smart_config();

        manage_ip_config();

        manage_cloud_connection(force_events);

        system::FirmwareUpdate::instance()->process();

        if (system_mode() != SAFE_MODE) {
            manage_listening_mode_flag();
        }

#if HAL_PLATFORM_NCP_FW_UPDATE
        services::SaraNcpFwUpdate::instance()->process();
#endif
    }
    else
    {
        system_pending_shutdown(RESET_REASON_USER);
    }
#if HAL_PLATFORM_BLE
    // TODO: Process BLE channel events in a separate thread
    system::SystemControl::instance()->run();
    if (system_mode() != SAFE_MODE) {
        manage_ble_prov_mode();
    }
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

    // Ensure that RTOS vTaskDelay(0) is called to force a reschedule to avoid task starvation in tight delay(1) loops
    HAL_Delay_Milliseconds(0);

    while (1)
    {
        system_tick_t elapsed_millis = HAL_Timer_Get_Milli_Seconds() - start_millis;

        if (elapsed_millis > ms)
        {
            break;
        }
        else if (elapsed_millis >= (ms-1))
        {
            // on the last millisecond, resolve using micros - we don't know how far in that millisecond had come
            // have to be careful with wrap around since start_micros can be greater than end_micros.
            for (;;)
            {
                system_tick_t delay = end_micros - HAL_Timer_Get_Micro_Seconds();
                if (delay > 100000) {
                    return;
                }
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

    if (!force_no_background_loop)
    {
        // Always pump the system thread at least once
        spark_process();
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

void cloud_disconnect(unsigned flags, cloud_disconnect_reason cloudReason, network_disconnect_reason networkReason,
        System_Reset_Reason resetReason, unsigned sleepDuration)
{
    if (SPARK_CLOUD_SOCKETED || SPARK_CLOUD_CONNECTED)
    {
        INFO("Cloud: disconnecting");
        const auto diag = CloudDiagnostics::instance();
        if (SPARK_CLOUD_CONNECTED)
        {
            diag->resetConnectionAttempts();
            if (cloudReason != CLOUD_DISCONNECT_REASON_NONE) {
                diag->disconnectionReason(cloudReason);
                if (cloudReason == CLOUD_DISCONNECT_REASON_ERROR) {
                    diag->disconnectedUnexpectedly();
                }
            }
            diag->status(CloudDiagnostics::DISCONNECTING);
            // "Disconnecting" event is generated only for a successfully established connection (including handshake)
            system_notify_event(cloud_status, cloud_status_disconnecting);
        }

        // Get disconnection options
        const auto opts = CloudConnectionSettings::instance()->takePendingDisconnectOptions();
        const bool graceful = (flags & CLOUD_DISCONNECT_GRACEFULLY) && opts.graceful();
        if (SPARK_CLOUD_CONNECTED) {
            if (graceful) {
                // Notify the cloud that we're about to disconnect
                spark_disconnect_command cmd = {};
                cmd.size = sizeof(cmd);
                cmd.cloud_reason = cloudReason;
                cmd.network_reason = networkReason;
                cmd.reset_reason = resetReason;
                cmd.sleep_duration = sleepDuration;
                cmd.timeout = opts.timeout();
                const int r = spark_protocol_command(spark_protocol_instance(), ProtocolCommands::DISCONNECT, 0, &cmd);
                if (r != protocol::NO_ERROR) {
                    LOG(WARN, "cloud_disconnect(): DISCONNECT command failed: %d", r);
                }
            } else {
                spark_protocol_command(spark_protocol_instance(), ProtocolCommands::TERMINATE, 0, nullptr);
            }
        }
        if (opts.clearSession()) {
            clearSessionData();
        }
        if (!(flags & CLOUD_DISCONNECT_DONT_CLOSE)) {
            spark_cloud_socket_disconnect(graceful);
        }

        // Note: Invoking the protocol's DISCONNECT or TERMINATE command cancels the ongoing firmware
        // if the device is being updated OTA, so there's no need to explicitly cancel the update here
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_HANDSHAKE_PENDING = 0;
        SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 0;
        SPARK_CLOUD_SOCKETED = 0;

        LED_SIGNAL_STOP(CLOUD_CONNECTED);
        LED_SIGNAL_STOP(CLOUD_HANDSHAKE);
        LED_SIGNAL_STOP(CLOUD_CONNECTING);

        INFO("Cloud: disconnected");
        diag->status(CloudDiagnostics::DISCONNECTED);
        system_notify_event(cloud_status, cloud_status_disconnected);
    }
    Spark_Error_Count = 0;  // this is also used for CFOD/WiFi reset, and blocks the LED when set.
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
#if HAL_PLATFORM_NRF52840
	invokeEventHandler(handlerInfoSize, handlerInfo, event_name, event_data, reserved);
	return SYSTEM_ERROR_NONE;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // HAL_PLATFORM_NRF52840
}

void* system_internal(int item, void* reserved)
{
    switch (item) {
#if PLATFORM_THREADING
    case 0: {
        return &ApplicationThread;
    }
    case 1: {
        return &SystemThread;
    }
    case 2: {
        return mutex_usb_serial();
    }
#endif
    case 3: {
        return reinterpret_cast<void*>(system_cloud_get_socket_handle());
    }
    default:
        return nullptr;
    }
}
