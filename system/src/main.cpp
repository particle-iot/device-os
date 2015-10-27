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
#include "syshealth_hal.h"
#include "watchdog_hal.h"
#include "usb_hal.h"
#include "system_mode.h"
#include "rgbled.h"
#include "ledcontrol.h"
#include "spark_wiring_power.h"
#include "spark_wiring_fuel.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingLED;
static volatile uint32_t TimingIWDGReload;

/**
 * KNowing the current listen mode isn't sufficient to determine the correct action (since that may or may not have changed)
 * we need also to know the listen mode at the time the button was pressed.
 */
static volatile bool wasListeningOnButtonPress;
/**
 * The lower 16-bits of the time when the button was first pushed.
 */
static volatile uint16_t buttonPushed;

uint16_t system_button_pushed_duration(uint8_t button, void*)
{
    if (button || network.listening())
        return 0;
    return buttonPushed ? HAL_Timer_Get_Milli_Seconds()-buttonPushed : 0;
}

static uint32_t prev_release_time = 0;
static uint8_t clicks = 0;

void system_handle_double_click()
{
    SYSTEM_POWEROFF = 1;
    network.connect_cancel(true, true);
}

void handle_button_click(uint16_t duration, uint32_t release_time)
{
    uint32_t start_time = release_time-duration;
    uint32_t since_prev = start_time-prev_release_time;
    bool reset = true;
    if (duration<500) {                                 // a short button press
        if (!clicks || (since_prev<1000)) {		// first click or within 1 second of the previous click
            clicks++;
            prev_release_time = release_time;
            if (clicks==2)
            {
                clicks = 0;
                prev_release_time = release_time-1000;
                system_handle_double_click();
            }
            else {
                reset = false;
            }
        }
    }
    if (reset) {
        prev_release_time = release_time-1000;	//
        clicks = 0;
    }

}

// this is called on multiple threads - ideally need a mutex
void HAL_Notify_Button_State(uint8_t button, uint8_t pressed)
{
    if (button==0)
    {
        if (pressed)
        {
            wasListeningOnButtonPress = network.listening();
            buttonPushed = HAL_Timer_Get_Milli_Seconds();
            if (!wasListeningOnButtonPress)             // start of button press
            {
                system_notify_event(button_status, 0);
            }
        }
        else
        {
            int release_time = HAL_Timer_Get_Milli_Seconds();
            uint16_t duration = release_time-buttonPushed;

            if (!network.listening()) {
                system_notify_event(button_status, duration);
                handle_button_click(duration, release_time);
            }
            buttonPushed = 0;
            if (duration>3000 && duration<8000 && wasListeningOnButtonPress && network.listening())
                network.listen(true);
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
    if (LED_RGB_IsOverRidden())
    {
#ifndef SPARK_NO_CLOUD
        if (LED_Spark_Signal != 0)
        {
            LED_Signaling_Override();
        }
#endif
    }
    else if (TimingLED != 0x00)
    {
        TimingLED--;
    }
    else if(SPARK_FLASH_UPDATE || Spark_Error_Count || network.listening())
    {
        //Do nothing
    }
    else if (SYSTEM_POWEROFF)
    {
        LED_SetRGBColor(RGB_COLOR_GREY);
        LED_On(LED_RGB);
    }
    else if(SPARK_LED_FADE && (!SPARK_CLOUD_CONNECTED || system_cloud_active()))
    {
        LED_Fade(LED_RGB);
        TimingLED = 20;//Breathing frequency kept constant
    }
    else if(SPARK_CLOUD_CONNECTED)
    {
        LED_SetRGBColor(system_mode()==SAFE_MODE ? RGB_COLOR_MAGENTA : RGB_COLOR_CYAN);
        LED_On(LED_RGB);
        SPARK_LED_FADE = 1;
    }
    else
    {
        LED_Toggle(LED_RGB);
        if(SPARK_CLOUD_SOCKETED || ( network.connected() && !network.ready()))
            TimingLED = 50;         //50ms
        else
            TimingLED = 100;        //100ms
    }

    if(SPARK_FLASH_UPDATE)
    {
#ifndef SPARK_NO_CLOUD
        if (TimingFlashUpdateTimeout >= TIMING_FLASH_UPDATE_TIMEOUT)
        {
            //Reset is the only way now to recover from stuck OTA update
            HAL_Core_System_Reset();
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
        if (network.connecting()) {
            network.connect_cancel(true, true);
        }
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
}

void manage_safe_mode()
{
    uint16_t flag = (HAL_Bootloader_Get_Flag(BOOTLOADER_FLAG_STARTUP_MODE));
    if (flag != 0xFF) { // old bootloader
        if (flag & 1) {
            set_system_mode(SAFE_MODE);
            // explicitly disable multithreading
            system_thread_set_state(spark::feature::DISABLED, NULL);
        }
    }
}

void app_loop(bool threaded)
{
    DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
    if (!threaded)
        Spark_Idle();

    static uint8_t SPARK_WIRING_APPLICATION = 0;
    if(threaded || SPARK_WLAN_SLEEP || !SPARK_CLOUD_CONNECT || SPARK_CLOUD_CONNECTED || SPARK_WIRING_APPLICATION || (system_mode()!=AUTOMATIC))
    {
        if(threaded || !SPARK_FLASH_UPDATE)
        {
            if ((SPARK_WIRING_APPLICATION != 1))
            {
                //Execute user application setup only once
                DECLARE_SYS_HEALTH(ENTERED_Setup);
                if (system_mode()!=SAFE_MODE)
                 setup();
                SPARK_WIRING_APPLICATION = 1;
            }

            //Execute user application loop
            DECLARE_SYS_HEALTH(ENTERED_Loop);
            if (system_mode()!=SAFE_MODE) {
                loop();
                DECLARE_SYS_HEALTH(RAN_Loop);
#if !MODULAR_FIRMWARE
                serialEventRun();
#endif
            }
        }
    }
}


#if PLATFORM_THREADING

// This is the application loop ActiveObject.

void app_thread_idle()
{
    app_loop(true);
}

// don't wait to get items from the queue, so the application loop is processed as often as possible
// timeout after 3000 ms to put calls into the application queue, so the system thread does not deadlock  (since the application may also
// be trying to put events in the system queue.)
ActiveObjectCurrentThreadQueue ApplicationThread(ActiveObjectConfiguration(app_thread_idle, 0, 5000));

#endif

extern "C" void system_part2_post_init() __attribute__((weak));

// this is overridden for modular firmware
void system_part2_post_init()
{
}

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
    // We have running firmware, otherwise we wouldn't have gotten here
    DECLARE_SYS_HEALTH(ENTERED_Main);
    PMIC().begin();
    FuelGauge().wakeup();

    DEBUG("Hello from Particle!");
    String s = spark_deviceID();
    INFO("Device %s started", s.c_str());

    manage_safe_mode();

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
