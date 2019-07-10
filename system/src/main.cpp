/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 *
 * @brief   Main program body.
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "debug.h"
#include "system_event.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_cloud_internal.h"
#include "system_cloud_connection.h"
#include "system_sleep.h"
#include "system_threading.h"
#include "system_user.h"
#include "system_update.h"
#include "system_commands.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "syshealth_hal.h"
#include "watchdog_hal.h"
#include "usb_hal.h"
#include "button_hal.h"
#if HAL_PLATFORM_DCT
#include "dct_hal.h"
#endif // HAL_PLATFORM_DCT
#include "system_mode.h"
#include "rgbled.h"
#include "led_service.h"
#include "diagnostics.h"
#include "check.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_cellular_printable.h"
#include "spark_wiring_led.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_system.h"
#include "system_power.h"
#include "spark_wiring_wifi.h"

// FIXME
#include "system_openthread.h"
#include "system_control_internal.h"

#if HAL_PLATFORM_FILESYSTEM
#include "filesystem.h"
#endif /* HAL_PLATFORM_FILESYSTEM */

#if HAL_PLATFORM_LWIP
#include "ifapi.h"
#endif /* HAL_PLATFORM_LWIP */

#if HAL_PLATFORM_IFAPI
#include "system_listening_mode.h"
#endif /* HAL_PLATFORM_IFAPI */

#if HAL_PLATFORM_NCP
#include "network/ncp.h"
#endif

#if HAL_PLATFORM_MESH
#include <openthread/platform/settings.h>
#endif

#if PLATFORM_ID == 3
// Application loop uses std::this_thread::sleep_for() to workaround 100% CPU usage on the GCC platform
#include <thread>
#endif

using namespace spark;
using namespace particle;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#if defined(DEBUG_BUTTON_WD)
#define BUTTON_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define BUTTON_WD_DEBUG(x,...)
#endif

static volatile uint32_t button_timeout_start;
static volatile uint32_t button_timeout_duration;

inline void ARM_BUTTON_TIMEOUT(uint32_t dur) {
    button_timeout_start = HAL_Timer_Get_Milli_Seconds();
    button_timeout_duration = dur;
    BUTTON_WD_DEBUG("Button WD Set %d",(dur));
}
inline bool IS_BUTTON_TIMEOUT() {
    return button_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-button_timeout_start)>button_timeout_duration);
}

inline void CLR_BUTTON_TIMEOUT() {
    button_timeout_duration = 0;
    BUTTON_WD_DEBUG("Button WD Cleared, was %d",button_timeout_duration);
}

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingIWDGReload;

/**
 * KNowing the current listen mode isn't sufficient to determine the correct action (since that may or may not have changed)
 * we need also to know the listen mode at the time the button was pressed.
 */
static volatile bool wasListeningOnButtonPress;
/**
 * The lower 16-bits of the time when the button was first pressed.
 */
static volatile uint16_t pressed_time = 0;

uint16_t system_button_pushed_duration(uint8_t button, void*)
{
    if (button || network_listening(0, 0, 0))
        return 0;
    return pressed_time ? HAL_Timer_Get_Milli_Seconds()-pressed_time : 0;
}

static volatile uint8_t button_final_clicks = 0;
static volatile uint8_t button_current_clicks = 0;

/* FIXME */
static volatile bool button_cleared_credentials = false;

#if Wiring_SetupButtonUX

namespace {

// LED status blinking specified number of times
class LEDCounterStatus: public LEDStatus {
public:
    explicit LEDCounterStatus(LEDPriority priority) :
            LEDStatus(LED_PATTERN_CUSTOM, priority) {
    }

    void start(uint8_t count) {
        setActive(false);
        count_ = count;
        delay(); // Delay before blinking
        setActive(true);
    }

protected:
    virtual void update(system_tick_t t) override {
        if (t >= ticks_) {
            // Change state
            switch (state_) {
            case DELAY:
                if (count_ > 0) {
                    on(); // Turn LED on
                } else {
                    setActive(false); // Stop indication
                }
                break;
            case ON:
                if (--count_ > 0) {
                    off(); // Turn LED off
                } else {
                    delay(); // Delay after blinking
                }
                break;
            case OFF:
                on();
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
        DELAY
    };

    State state_;
    uint16_t ticks_;
    uint8_t count_;

    void on() {
        state_ = ON;
        ticks_ = 50;
        setColor(0x0000ff00); // Light green
    }

    void off() {
        state_ = OFF;
        ticks_ = 350;
        setColor(0x00000a00); // Dark green
    }

    void delay() {
        state_ = DELAY;
        ticks_ = 750;
        setColor(0x00000a00); // Dark green
    }
};

} // namespace

/* displays RSSI value on system LED */
void system_display_rssi() {
    int bars = 0;
#if Wiring_WiFi == 1
    auto sig = WiFi.RSSI();
#elif Wiring_Cellular == 1
    auto sig = Cellular.RSSI();
#endif
    if (sig.getStrength() >= 0) {
        bars = std::round(sig.getStrength() / 20.0f);
    }
    DEBUG("RSSI: %ddBm BARS: %d\r\n", (int)(sig.getStrengthValue()), bars);

    static LEDCounterStatus ledCounter(LED_PRIORITY_IMPORTANT);
    ledCounter.start(bars);
}

void system_power_off() {
    LED_SIGNAL_START(POWER_OFF, CRITICAL);
    SYSTEM_POWEROFF = 1;
    cancel_connection(); // Unblock the system thread
}

void system_handle_button_clicks(bool isIsr)
{
    switch (button_final_clicks) {
    case 1: { // Single click
        if (isIsr) {
            return; // The event will be processed in the system loop
        }
        system_display_rssi();
        break;
    }
    case 2: { // Double click
        system_power_off();
        break;
    }
    default:
        break;
    }
    button_final_clicks = 0;
}

#endif // #if Wiring_SetupButtonUX

void reset_button_click()
{
    const uint8_t clicks = button_current_clicks;
    button_current_clicks = 0;
    CLR_BUTTON_TIMEOUT();
    if (clicks > 0) {
        system_notify_event(button_final_click, clicks, nullptr, nullptr, nullptr, NOTIFY_SYNCHRONOUSLY);
        button_final_clicks = clicks;
#if Wiring_SetupButtonUX
        // Certain numbers of clicks can be processed directly in ISR
        system_handle_button_clicks(HAL_IsISR());
#endif
    }
}

void handle_button_click(uint16_t depressed_duration)
{
    bool reset = true;
    if (depressed_duration < 30) { // Likely a spurious click due to switch bouncing
        reset = false;
    } else if (depressed_duration < 500) { // a short button press
        button_current_clicks++;
        if (button_current_clicks < 5) { // 5 clicks "ought to be enough for anybody"
            // If next button click doesn't come within 1 second, declare a
            // final button click.
            ARM_BUTTON_TIMEOUT(1000);
            reset = false;
        }
        system_notify_event(button_click, button_current_clicks, nullptr, nullptr, nullptr, NOTIFY_SYNCHRONOUSLY);
    }
    if (reset) {
        reset_button_click();
    }
}

// this is called on multiple threads - ideally need a mutex
void HAL_Notify_Button_State(uint8_t button, uint8_t pressed)
{
#ifdef BUTTON1_MIRROR_SUPPORTED
    if (button==0 || button == BUTTON1_MIRROR)
#else
    if (button==0)
#endif
    {
        if (pressed)
        {
            wasListeningOnButtonPress = network_listening(0, 0, 0);
            pressed_time = HAL_Timer_Get_Milli_Seconds();
            if (!wasListeningOnButtonPress)             // start of button press
            {
                system_notify_event(button_status, 0, nullptr, nullptr, nullptr, NOTIFY_SYNCHRONOUSLY);
            }
            button_cleared_credentials = false;
        }
        else if (pressed_time > 0)
        {
            int release_time = HAL_Timer_Get_Milli_Seconds();
            uint16_t depressed_duration = release_time - pressed_time;

            if (!network_listening(0, 0, 0)) {
                system_notify_event(button_status, depressed_duration, nullptr, nullptr, nullptr, NOTIFY_SYNCHRONOUSLY);
                handle_button_click(depressed_duration);
            }
            pressed_time = 0;
            if (depressed_duration>3000 && depressed_duration<8000 && wasListeningOnButtonPress && network_listening(0, 0, 0)) {
                network_listen(0, NETWORK_LISTEN_EXIT, 0);
            }
        }
    }
}

/*******************************************************************************
 * Function Name  : HAL_SysTick_Handler
 * Description    : Decrements the various Timing variables related to SysTick.
 * Input          : None
 * Output         : None.
 * Return         : None.
 ************************************************
 *******************************/
extern "C" void HAL_SysTick_Handler(void)
{
    // Update LED color
    static const uint16_t LED_UPDATE_INTERVAL = 25; // Milliseconds
    static uint16_t ledUpdateTicks = LED_UPDATE_INTERVAL;

    if (--ledUpdateTicks == 0) {
        led_update(LED_UPDATE_INTERVAL, nullptr, nullptr);
        ledUpdateTicks = LED_UPDATE_INTERVAL;
    }

    // Check cloud inactivity timeout
    static const uint16_t CLOUD_CHECK_INTERVAL = 1000; // Milliseconds
    static uint16_t cloudCheckTicks = CLOUD_CHECK_INTERVAL;

    if (--cloudCheckTicks == 0) {
#ifndef SPARK_NO_CLOUD
        system_cloud_active();
#endif // SPARK_NO_CLOUD
        cloudCheckTicks = CLOUD_CHECK_INTERVAL;
    }

    if(SPARK_FLASH_UPDATE)
    {
#ifndef SPARK_NO_CLOUD
        if (TimingFlashUpdateTimeout >= TIMING_FLASH_UPDATE_TIMEOUT)
        {
            //Reset is the only way now to recover from stuck OTA update
            HAL_Core_System_Reset_Ex(RESET_REASON_UPDATE_TIMEOUT, 0, nullptr);
        }
        else
        {
            TimingFlashUpdateTimeout++;
        }
#endif
    }
    else if(network_listening(0, 0, 0) && HAL_Core_Mode_Button_Pressed(10000) && !button_cleared_credentials)
    {
        button_cleared_credentials = true;
        network_listen_command(0, NETWORK_LISTEN_COMMAND_CLEAR_CREDENTIALS, 0);
    }
    // determine if the button press needs to change the state (and hasn't done so already))
    else if(!network_listening(0, 0, 0) && HAL_Core_Mode_Button_Pressed(3000) && !wasListeningOnButtonPress)
    {
        cancel_connection(); // Unblock the system thread
        // fire the button event to the user, then enter listening mode (so no more button notifications are sent)
        // there's a race condition here - the HAL_notify_button_state function should
        // be thread safe, but currently isn't.
        HAL_Notify_Button_State(0, false);
        network_listen(0, 0, 0);
        HAL_Notify_Button_State(0, true);
    }

#ifdef IWDG_RESET_ENABLE
    if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
    {
        TimingIWDGReload = 0;

        /* Reload WDG counter */
        HAL_Notify_WDT();
        DECLARE_SYS_HEALTH(CLEARED_WATCHDOG);
    }
    else
    {
        TimingIWDGReload++;
    }
#endif

#if HAL_PLATFORM_BUTTON_DEBOUNCE_IN_SYSTICK
    BUTTON_Timer_Handler();
#endif

    if (IS_BUTTON_TIMEOUT())
    {
        reset_button_click();
    }
}

void manage_safe_mode()
{
    uint16_t flag = (HAL_Bootloader_Get_Flag(BOOTLOADER_FLAG_STARTUP_MODE));
    if (flag != 0xFF) { // old bootloader
        if (flag & 1) {
            set_system_mode(SAFE_MODE);
            // explicitly disable multithreading
            system_thread_set_state(spark::feature::DISABLED, NULL);
            uint8_t value = 0;
            system_get_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, &value, nullptr);
            if (value)
            {
                system_set_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, 0, 0);
                // flag listening mode
                network_listen(0, 0, 0);
            }
        }
    }
}

bool semi_automatic_connecting(bool threaded) {
    return system_mode() == SEMI_AUTOMATIC && !threaded && spark_cloud_flag_auto_connect() && !spark_cloud_flag_connected();
}

void app_loop(bool threaded)
{
    DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
    if (!threaded)
        Spark_Idle();

    static uint8_t SPARK_WIRING_APPLICATION = 0;
    do {
    if(threaded || SPARK_WLAN_SLEEP || !spark_cloud_flag_auto_connect() || spark_cloud_flag_connected() || SPARK_WIRING_APPLICATION || (system_mode()!=AUTOMATIC))
    {
        if(threaded || !SPARK_FLASH_UPDATE)
        {
                if (semi_automatic_connecting(threaded)) {
                    break;
                }

#if HAL_PLATFORM_IFAPI
            if (!threaded && particle::system::ListeningModeHandler::instance()->isActive()) {
                break;
            }
#endif // HAL_PLATFORM_IFAPI

            if ((SPARK_WIRING_APPLICATION != 1))
            {
                //Execute user application setup only once
                DECLARE_SYS_HEALTH(ENTERED_Setup);
                if (system_mode()!=SAFE_MODE)
                 setup();
                SPARK_WIRING_APPLICATION = 1;
#if !(defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE)
                _post_loop();
#endif
                    if (semi_automatic_connecting(threaded)) {
                        break;
            }
                }

            //Execute user application loop
            DECLARE_SYS_HEALTH(ENTERED_Loop);
            if (system_mode()!=SAFE_MODE) {
                loop();
                DECLARE_SYS_HEALTH(RAN_Loop);
#if !(defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE)
                _post_loop();
#endif
            }
        }
    }
    } while(false);
#if PLATFORM_ID == 3 && SUSPEND_APPLICATION_THREAD_LOOP_COUNT
    // Suspend thread execution for some minimum time on every Nth loop iteration in order to workaround
    // 100% CPU usage on the virtual device platform
    static uint32_t loops = 0;
    if (++loops >= SUSPEND_APPLICATION_THREAD_LOOP_COUNT) {
        loops = 0;
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
#endif // PLATFORM_ID == 3
}


#if PLATFORM_THREADING

// This is the application loop ActiveObject.

void app_thread_idle()
{
    app_loop(true);
}

// don't wait to get items from the queue, so the application loop is processed as often as possible
// timeout after attempting to put calls into the application queue, so the system thread does not deadlock  (since the application may also
// be trying to put events in the system queue.)
ActiveObjectCurrentThreadQueue ApplicationThread(ActiveObjectConfiguration(app_thread_idle,
		0, /* take time */
		5000, /* put time */
		20 /* queue size */));

#endif

extern "C" void system_part2_post_init() __attribute__((weak));

// this is overridden for modular firmware
void system_part2_post_init()
{
}

void handle_out_of_memory(size_t requested) {
	static bool recurse = false;
	if (recurse) {
        PANIC(OutOfHeap,"Out Of Heap");
        abort();
	}
	else {
		recurse = true;
		system_notify_event(out_of_memory, requested, nullptr, nullptr, nullptr, NOTIFY_SYNCHRONOUSLY);
		recurse = false;
	}
}

namespace {

// LED status shown during device key generation
class LEDDeviceKeyStatus: public LEDStatus {
public:
    explicit LEDDeviceKeyStatus(LEDPriority priority) :
            LEDStatus(LED_PATTERN_BLINK, priority) {
    }

    void setActive(bool active) {
        if (active) {
            // Get base color used for the "network off" indication
            const LEDStatusData* s = led_signal_status(LED_SIGNAL_NETWORK_OFF, nullptr);
            setColor(s ? s->color : RGB_COLOR_WHITE);
        }
        LEDStatus::setActive(active);
    }
};

// Handler for HAL events
class HALEventHandler {
public:
    HALEventHandler() {
        HAL_Set_Event_Callback(handleEvent, nullptr); // Register callback
    }

private:
    static void handleEvent(int event, int flags, void* data) {
        switch (event) {
        case HAL_EVENT_GENERATE_DEVICE_KEY: {
            static LEDDeviceKeyStatus status(LED_PRIORITY_IMPORTANT);
            if (flags & HAL_EVENT_FLAG_START) {
                status.setActive(true);
            } else if (flags & HAL_EVENT_FLAG_STOP) {
                status.setActive(false);
            }
            break;
        }
        case HAL_EVENT_OUT_OF_MEMORY: {
        		handle_out_of_memory(flags);
        		break;
        }
        default:
            break;
        }
    }
};

class UptimeDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    UptimeDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_SYSTEM_UPTIME, DIAG_NAME_SYSTEM_UPTIME) {
    }

    virtual int get(IntType& val) override {
        val = System.uptime();
        return 0; // OK
    }
};

class RunTimeInfoDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    typedef IntType(*func_t)(const runtime_info_t&);
    RunTimeInfoDiagnosticData(uint16_t id, const char* name, func_t f) :
            AbstractIntegerDiagnosticData(id, name),
            f_(f) {
    }

    virtual int get(IntType& val) override {
        runtime_info_t info = {0};
        info.size = sizeof(info);
        if (HAL_Core_Runtime_Info(&info, nullptr) == 0) {
            val = f_(info);
            return SYSTEM_ERROR_NONE;
        }
        return SYSTEM_ERROR_UNKNOWN;
    }

private:
    func_t f_;
};

int resetSettingsToFactoryDefaultsIfNeeded() {
#if !defined(SPARK_NO_PLATFORM) && HAL_PLATFORM_DCT
    Load_SystemFlags();
    if (SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) != 0x0001) {
        return 0;
    }
    SYSTEM_FLAG(NVMEM_SPARK_Reset_SysFlag) = 0x0000;
    Save_SystemFlags();
#if HAL_PLATFORM_MESH
    LOG(WARN, "Resetting all settings to factory defaults");
    // Clear OpenThread settings
    otPlatSettingsInit(nullptr);
    otPlatSettingsWipe(nullptr);
    // Clear pending commands
    system::system_command_clear();
#if HAL_PLATFORM_WIFI
    // Clear WiFi credentials
    WifiNetworkManager::clearNetworkConfig();
#endif // HAL_PLATFORM_WIFI
#if HAL_PLATFORM_CELLULAR
    // Clear cellular credentials
    CellularNetworkManager::clearNetworkConfig();
#endif // HAL_PLATFORM_CELLULAR
    // Copy device keys
    std::unique_ptr<char[]> devPrivKey(new(std::nothrow) char[DCT_ALT_DEVICE_PRIVATE_KEY_SIZE]);
    std::unique_ptr<char[]> devPubKey(new(std::nothrow) char[DCT_ALT_DEVICE_PUBLIC_KEY_SIZE]);
    CHECK_TRUE(devPrivKey && devPubKey, SYSTEM_ERROR_NO_MEMORY);
    CHECK(dct_read_app_data_copy(DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, devPrivKey.get(), DCT_ALT_DEVICE_PRIVATE_KEY_SIZE));
    CHECK(dct_read_app_data_copy(DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET, devPubKey.get(), DCT_ALT_DEVICE_PUBLIC_KEY_SIZE));
    // Clear DCT and restore device keys
    CHECK(dct_clear());
    CHECK(dct_write_app_data(devPrivKey.get(), DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, DCT_ALT_DEVICE_PRIVATE_KEY_SIZE));
    CHECK(dct_write_app_data(devPubKey.get(), DCT_ALT_DEVICE_PUBLIC_KEY_OFFSET, DCT_ALT_DEVICE_PUBLIC_KEY_SIZE));
    // Restore default server key and address
    CHECK(dct_write_app_data(backup_udp_public_server_key, DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, backup_udp_public_server_key_size));
    CHECK(dct_write_app_data(backup_udp_public_server_address, DCT_ALT_SERVER_ADDRESS_OFFSET, backup_udp_public_server_address_size));
#endif // HAL_PLATFORM_MESH
#endif // !defined(SPARK_NO_PLATFORM) && HAL_PLATFORM_DCT
    return 0;
}

// Certain HAL events can be generated before app_setup_and_loop() is called. Using constructor of a
// global variable allows to register a handler for HAL events early
HALEventHandler g_halEventHandler;

UptimeDiagnosticData g_uptimeDiagData;

RunTimeInfoDiagnosticData g_totalRamDiagData(DIAG_ID_SYSTEM_TOTAL_RAM, DIAG_NAME_SYSTEM_TOTAL_RAM,
    [](const runtime_info_t& info) -> RunTimeInfoDiagnosticData::IntType {
        return info.total_init_heap;
    }
);

RunTimeInfoDiagnosticData g_usedRamDiagData(DIAG_ID_SYSTEM_USED_RAM, DIAG_NAME_SYSTEM_USED_RAM,
    [](const runtime_info_t& info) -> RunTimeInfoDiagnosticData::IntType {
        return (info.total_init_heap - info.freeheap);
    }
);

} // namespace

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void app_setup_and_loop(void)
{
    system_part2_post_init();
    HAL_Core_Init();
    main_thread_current(NULL);
    // We have running firmware, otherwise we wouldn't have gotten here
    DECLARE_SYS_HEALTH(ENTERED_Main);

    LED_SIGNAL_START(NETWORK_OFF, BACKGROUND);

    // Reset all persistent settings to factory defaults if necessary
    resetSettingsToFactoryDefaultsIfNeeded();

    system_power_management_init();

    // Start the diagnostics service
    diag_command(DIAG_SERVICE_CMD_START, nullptr, nullptr);

    DEBUG("Hello from Particle!");
    String s = spark_deviceID();
    INFO("Device %s started", s.c_str());

#if HAL_PLATFORM_FILESYSTEM
    filesystem_dump_info(filesystem_get_instance(nullptr));
#endif /* HAL_PLATFORM_FILESYSTEM */

    if (LOG_ENABLED(TRACE)) {
        int reason = RESET_REASON_NONE;
        uint32_t data = 0;
        if (HAL_Core_Get_Last_Reset_Info(&reason, &data, nullptr) == 0 && reason != RESET_REASON_NONE) {
            LOG(TRACE, "Last reset reason: %d (data: 0x%02x)", reason, (unsigned)data); // TODO: Use LOG_ATTR()
        }
    }

#if HAL_PLATFORM_BLE
    // FIXME: Move BLE and Thread initialization to an appropriate place
    ble_init(nullptr);
#endif // HAL_PLATFORM_BLE

#if HAL_PLATFORM_LWIP
    if_init();
#endif /* HAL_PLATFORM_LWIP */

#if HAL_PLATFORM_OPENTHREAD
    system::threadInit();
#endif /* HAL_PLATFORM_OPENTHREAD */

#if SYSTEM_CONTROL_ENABLED
    system::SystemControl::instance()->init();
#endif // SYSTEM_CONTROL_ENABLED

    manage_safe_mode();

#if defined(USB_CDC_ENABLE) || defined(USB_HID_ENABLE)
    HAL_USB_Init();
#endif

#if defined (START_DFU_FLASHER_SERIAL_SPEED) || defined (START_YMODEM_FLASHER_SERIAL_SPEED)
    USB_USART_LineCoding_BitRate_Handler(system_lineCodingBitRateHandler);
#endif

    bool threaded = system_thread_get_state(NULL) != spark::feature::DISABLED &&
      (system_mode()!=SAFE_MODE);

    // Checks for bootloader update applied from DFU to OTA region + special OTA flag of 0xA5
    // In that case, HAL_UPDATE_APPLIED is returned and a reset is required to ensure we don't
    // remain in Safe Mode due to bootloader dependency checks.  HAL_UPDATE_APPLIED_PENDING_RESTART won't
    // be returned when updating the bootloader, but we check for it just in case so we can reset if necessary.
    hal_update_complete_t pendingUpdateResult = HAL_FLASH_ApplyPendingUpdate(nullptr /*module*/, false /*dryRun*/, nullptr /*reserved*/);
    if (pendingUpdateResult == HAL_UPDATE_APPLIED_PENDING_RESTART || pendingUpdateResult == HAL_UPDATE_APPLIED) {
        // the regular OTA update delays 100 milliseconds so maintaining the same behavior.
        HAL_Delay_Milliseconds(100);
        HAL_Core_System_Reset_Ex(RESET_REASON_UPDATE, 0, nullptr);
    }

    Network_Setup(threaded);    // todo - why does this come before system thread initialization?

#if PLATFORM_THREADING
    if (threaded)
    {
        SystemThread.start();
        ApplicationThread.start();
    }
    else
    {
        SystemThread.setCurrentThread();
        ApplicationThread.setCurrentThread();
    }
#endif
    if(!threaded) {
        /* Main loop */
        while (1) {
            app_loop(false);
        }
    }
}

#ifdef USE_FULL_ASSERT

/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
