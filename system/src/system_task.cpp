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

#include "spark_wiring_network.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_cloud.h"
#include "system_threading.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_led.h"

using spark::Network;
using particle::LEDStatus;

volatile system_tick_t spark_loop_total_millis = 0;

// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

volatile uint8_t Spark_Error_Count;
volatile uint8_t SYSTEM_POWEROFF;

static struct SetThreadCurrentFunctionPointers {
    SetThreadCurrentFunctionPointers() {
        set_thread_current_function_pointers((void*)&main_thread_current,
                                             (void*)&system_thread_current,
                                             (void*)&application_thread_current,
                                             nullptr, nullptr);
    }
} s_SetThreadCurrentFunctionPointersInitializer;
ISRTaskQueue SystemISRTaskQueue(4);

void Network_Setup(bool threaded)
{
#if !PARTICLE_NO_NETWORK
    network.setup();

    // don't automatically connect when threaded since we want the thread to start asap
    if ((!threaded && system_mode() == AUTOMATIC) || system_mode()==SAFE_MODE)
    {
        network.connect();
    }
#endif

#ifndef SPARK_NO_CLOUD
    //Initialize spark protocol callbacks for all System modes
    Spark_Protocol_Init();
#endif
}

static int cfod_count = 0;

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

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection()
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || WLAN_WD_TO())
    {
        if (SPARK_WLAN_STARTED)
        {
            WARN("Resetting WLAN due to %s", (WLAN_WD_TO()) ? "WLAN_WD_TO()":((SPARK_WLAN_RESET) ? "SPARK_WLAN_RESET" : "SPARK_WLAN_SLEEP"));
            auto was_sleeping = SPARK_WLAN_SLEEP;
            auto was_disconnected = network.manual_disconnect();
            cloud_disconnect();
            network.off();
            CLR_WLAN_WD();
            SPARK_WLAN_RESET = 0;
            SPARK_WLAN_SLEEP = was_sleeping;
            network.set_manual_disconnect(was_disconnected);
            cfod_count = 0;
        }
    }
    else
    {
        if (!SPARK_WLAN_STARTED || (spark_cloud_flag_auto_connect() && !network.connected()))
        {
            INFO("Network Connect: %s", (!SPARK_WLAN_STARTED) ? "!SPARK_WLAN_STARTED" : "SPARK_CLOUD_CONNECT && !network.connected()");
            network.connect();
        }
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
    network.set_error_count(0); // Reset Error Count

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
    if (network.ready() && !SPARK_WLAN_SLEEP && !SPARK_CLOUD_SOCKETED)
    {
        LED_SIGNAL_START(CLOUD_CONNECTING, NORMAL);
        if (in_cloud_backoff_period())
        {
            return;
        }

        INFO("Cloud: connecting");
        system_notify_event(cloud_status, cloud_status_connecting);
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
            WARN("Cloud socket connection failed: %d", connect_result);
            SPARK_CLOUD_SOCKETED = 0;

            // "Connecting" event should be followed by either "connected" or "disconnected" event
            system_notify_event(cloud_status, cloud_status_disconnected);

            // if the user put the networkin listening mode via the button,
            // the cloud connect may have been cancelled.
            if (SPARK_WLAN_RESET || network.listening())
            {
                return;
            }

            cloud_connection_failed();
            handle_cfod();
            network.set_error_count(Spark_Error_Count);
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
                if (!SPARK_WLAN_RESET && !network.listening())
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
                cloud_disconnect();
            }
            else
            {
                INFO("Cloud connected");
                SPARK_CLOUD_CONNECTED = 1;
                cloud_failed_connection_attempts = 0;
                system_notify_event(cloud_status, cloud_status_connected);
                if (system_mode() == SAFE_MODE) {
                    LED_SIGNAL_START(SAFE_MODE, BACKGROUND); // Connected to the cloud while in safe mode
                } else {
                    LED_SIGNAL_START(CLOUD_CONNECTED, BACKGROUND);
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
        cloud_disconnect_graceful();
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
extern void system_handle_button_click();
#endif

void Spark_Idle_Events(bool force_events/*=false*/)
{
    HAL_Notify_WDT();

    ON_EVENT_DELTA();
    spark_loop_total_millis = 0;

    process_isr_task_queue();

    if (!SYSTEM_POWEROFF) {

#if Wiring_SetupButtonUX
        system_handle_button_click();
#endif
        manage_serial_flasher();

        manage_network_connection();

        manage_smart_config();

        manage_ip_config();

        CLOUD_FN(manage_cloud_connection(force_events), (void)0);
    }
    else
    {
        system_pending_shutdown();
    }
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

void cloud_disconnect_graceful(bool closeSocket)
{
    cloud_disconnect(closeSocket, true);
}

void cloud_disconnect(bool closeSocket, bool graceful)
{
#ifndef SPARK_NO_CLOUD

    if (SPARK_CLOUD_SOCKETED || SPARK_CLOUD_CONNECTED)
    {
        INFO("Cloud: disconnecting");
        if (SPARK_CLOUD_CONNECTED)
        {
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
