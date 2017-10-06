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
#include "system_sleep.h"
#include "system_threading.h"
#include "system_user.h"
#include "system_update.h"
#include "core_hal.h"
#include "delay_hal.h"
#include "syshealth_hal.h"
#include "watchdog_hal.h"
#include "usb_hal.h"
#include "system_mode.h"
#include "rgbled.h"
#include "led_service.h"
#include "spark_wiring_power.h"
#include "spark_wiring_fuel.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_cellular_printable.h"
#include "spark_wiring_led.h"

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
    if (button || network.listening())
        return 0;
    return pressed_time ? HAL_Timer_Get_Milli_Seconds()-pressed_time : 0;
}

static volatile uint8_t button_final_clicks = 0;
static volatile uint8_t button_current_clicks = 0;

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
    /*   signal strength (u-Blox Sara U2 and G3 modules)
     *   0: < -105 dBm
     *   1: < -93 dBm
     *   2: < -81 dBm
     *   3: < -69 dBm
     *   4: < - 57 dBm
     *   5: >= -57 dBm
     */
    int rssi = 0;
    int bars = 0;
#if Wiring_WiFi == 1
    rssi = WiFi.RSSI();
#elif Wiring_Cellular == 1
    CellularSignal sig = Cellular.RSSI();
    rssi = sig.rssi;
#endif
    if (rssi < 0) {
        if (rssi >= -57) bars = 5;
        else if (rssi > -68) bars = 4;
        else if (rssi > -80) bars = 3;
        else if (rssi > -92) bars = 2;
        else if (rssi > -104) bars = 1;
    }
    DEBUG("RSSI: %ddB BARS: %d\r\n", rssi, bars);

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
        system_notify_event(button_final_click, clicks);
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
        system_notify_event(button_click, button_current_clicks);
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
            wasListeningOnButtonPress = network.listening();
            pressed_time = HAL_Timer_Get_Milli_Seconds();
            if (!wasListeningOnButtonPress)             // start of button press
            {
                system_notify_event(button_status, 0);
            }
        }
        else if (pressed_time > 0)
        {
            int release_time = HAL_Timer_Get_Milli_Seconds();
            uint16_t depressed_duration = release_time - pressed_time;

            if (!network.listening()) {
                system_notify_event(button_status, depressed_duration);
                handle_button_click(depressed_duration);
            }
            pressed_time = 0;
            if (depressed_duration>3000 && depressed_duration<8000 && wasListeningOnButtonPress && network.listening()) {
                network.listen(true);
            }
        }
    }
}

#if Wiring_Cellular == 1
/* flag used to initiate system_power_management_update() from main thread */
static volatile bool SYSTEM_POWER_MGMT_UPDATE = false;

/*******************************************************************************
 * Function Name  : Power_Management_Handler
 * Description    : Sets default power management IC charging rate when USB
                    input power source changes or low battery indicated by
                    fuel gauge IC.
 * Output         : SYSTEM_POWER_MGMT_UPDATE is set to true.
 *******************************************************************************/
extern "C" void Power_Management_Handler(void)
{
    SYSTEM_POWER_MGMT_UPDATE = true;
}

void system_power_management_init()
{
    INFO("Power Management Initializing.");
    PMIC power;
    power.begin();
    power.disableWatchdog();
    power.disableDPDM();
    // power.setInputVoltageLimit(4360); // default
    power.setInputCurrentLimit(900);     // 900mA
    power.setChargeCurrent(0,0,0,0,0,0); // 512mA
    power.setChargeVoltage(4112);        // 4.112V termination voltage
    FuelGauge fuel;
    fuel.wakeup();
    fuel.setAlertThreshold(10); // Low Battery alert at 10% (about 3.6V)
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    INFO("State of Charge: %-6.2f%%", fuel.getSoC());
    INFO("Battery Voltage: %-4.2fV", fuel.getVCell());
    attachInterrupt(LOW_BAT_UC, Power_Management_Handler, FALLING);
}

void system_power_management_update()
{
    if (SYSTEM_POWER_MGMT_UPDATE) {
        SYSTEM_POWER_MGMT_UPDATE = false;
        PMIC power;
        power.begin();
        power.setInputCurrentLimit(900);     // 900mA
        power.setChargeCurrent(0,0,0,0,0,0); // 512mA
        static bool lowBattEventNotified = false; // Whether 'low_battery' event was generated already
        static bool wasCharging = false; // Whether the battery was charging last time when this function was called
        const uint8_t status = power.getSystemStatus();
        const bool charging = (status >> 4) & 0x03;
        if (charging && !wasCharging) { // Check if the battery has started to charge
            lowBattEventNotified = false; // Allow 'low_battery' event to be generated again
        }
        wasCharging = charging;
        FuelGauge fuel;
        bool LOWBATT = fuel.getAlert();
        if (LOWBATT) {
            fuel.clearAlert(); // Clear the Low Battery Alert flag if set
            if (!lowBattEventNotified) {
                lowBattEventNotified = true;
                system_notify_event(low_battery);
            }
        }
//        if (LOG_ENABLED(INFO)) {
//        		INFO(" %s", (LOWBATT)?"Low Battery Alert":"PMIC Interrupt");
//        }
#if defined(DEBUG_BUILD) && 0
        if (LOG_ENABLED(TRACE)) {
			uint8_t stat = power.getSystemStatus();
			uint8_t fault = power.getFault();
			uint8_t vbus_stat = stat >> 6; // 0 – Unknown (no input, or DPDM detection incomplete), 1 – USB host, 2 – Adapter port, 3 – OTG
			uint8_t chrg_stat = (stat >> 4) & 0x03; // 0 – Not Charging, 1 – Pre-charge (<VBATLOWV), 2 – Fast Charging, 3 – Charge Termination Done
			bool dpm_stat = stat & 0x08;   // 0 – Not DPM, 1 – VINDPM or IINDPM
			bool pg_stat = stat & 0x04;    // 0 – Not Power Good, 1 – Power Good
			bool therm_stat = stat & 0x02; // 0 – Normal, 1 – In Thermal Regulation
			bool vsys_stat = stat & 0x01;  // 0 – Not in VSYSMIN regulation (BAT > VSYSMIN), 1 – In VSYSMIN regulation (BAT < VSYSMIN)
			bool wd_fault = fault & 0x80;  // 0 – Normal, 1- Watchdog timer expiration
			uint8_t chrg_fault = (fault >> 4) & 0x03; // 0 – Normal, 1 – Input fault (VBUS OVP or VBAT < VBUS < 3.8 V),
													  // 2 - Thermal shutdown, 3 – Charge Safety Timer Expiration
			bool bat_fault = fault & 0x08;    // 0 – Normal, 1 – BATOVP
			uint8_t ntc_fault = fault & 0x07; // 0 – Normal, 5 – Cold, 6 – Hot
			DEBUG_D("[ PMIC STAT ] VBUS:%d CHRG:%d DPM:%d PG:%d THERM:%d VSYS:%d\r\n", vbus_stat, chrg_stat, dpm_stat, pg_stat, therm_stat, vsys_stat);
			DEBUG_D("[ PMIC FAULT ] WATCHDOG:%d CHRG:%d BAT:%d NTC:%d\r\n", wd_fault, chrg_fault, bat_fault, ntc_fault);
			delay(50);
        }
#endif
    }
}
#endif

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
        system_cloud_active();
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
    else if(network.listening() && HAL_Core_Mode_Button_Pressed(10000))
    {
        network.listen_command();
    }
    // determine if the button press needs to change the state (and hasn't done so already))
    else if(!network.listening() && HAL_Core_Mode_Button_Pressed(3000) && !wasListeningOnButtonPress)
    {
        cancel_connection(); // Unblock the system thread
        // fire the button event to the user, then enter listening mode (so no more button notifications are sent)
        // there's a race condition here - the HAL_notify_button_state function should
        // be thread safe, but currently isn't.
        HAL_Notify_Button_State(0, false);
        network.listen();
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
            system_get_flag(SYSTEM_FLAG_STARTUP_SAFE_LISTEN_MODE, &value, nullptr);
            if (value)
            {
                system_set_flag(SYSTEM_FLAG_STARTUP_SAFE_LISTEN_MODE, 0, 0);
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
#if Wiring_Cellular == 1
                system_power_management_update();
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
        default:
            break;
        }
    }
};

// Certain HAL events can be generated before app_setup_and_loop() is called. Using constructor of a
// global variable allows to register a handler for HAL events early
HALEventHandler halEventHandler;

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

#if Wiring_Cellular == 1
    system_power_management_init();
#endif

    DEBUG("Hello from Particle!");
    String s = spark_deviceID();
    INFO("Device %s started", s.c_str());

    if (LOG_ENABLED(TRACE)) {
        int reason = RESET_REASON_NONE;
        uint32_t data = 0;
        if (HAL_Core_Get_Last_Reset_Info(&reason, &data, nullptr) == 0 && reason != RESET_REASON_NONE) {
            LOG(TRACE, "Last reset reason: %d (data: 0x%02x)", reason, (unsigned)data); // TODO: Use LOG_ATTR()
        }
    }

    manage_safe_mode();

#if defined(USB_CDC_ENABLE) || defined(USB_HID_ENABLE)
    HAL_USB_Init();
#endif

#if defined (START_DFU_FLASHER_SERIAL_SPEED) || defined (START_YMODEM_FLASHER_SERIAL_SPEED)
    USB_USART_LineCoding_BitRate_Handler(system_lineCodingBitRateHandler);
#endif

    bool threaded = system_thread_get_state(NULL) != spark::feature::DISABLED &&
      (system_mode()!=SAFE_MODE);

    Network_Setup(threaded);

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
