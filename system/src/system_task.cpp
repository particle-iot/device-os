/**
 ******************************************************************************
 * @file    spark_wiring_wlan.cpp
 * @author  Satish Nair and Zachary Crockett
 * @version V1.0.0
 * @date    13-March-2013
 * @brief
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
#include "spark_wiring_system.h"
#include "spark_wiring_usbserial.h"
#include "system_task.h"
#include "system_cloud.h"
#include "system_mode.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "particle_macros.h"
#include "string.h"
#include "system_tick_hal.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "rgbled.h"

#include "spark_wiring_network.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_cloud.h"

using spark::Network;

volatile system_tick_t system_loop_total_millis = 0;

void (*announce_presence)(void);

// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

volatile uint8_t PARTICLE_LED_FADE = 1;

volatile uint8_t Particle_Error_Count;

void PARTICLE_WLAN_Setup(void (*presence_announcement_callback)(void))
{
    announce_presence = presence_announcement_callback;

#if !PARTICLE_NO_WIFI
    wlan_setup();

    /* Trigger a WLAN device */
    if (system_mode() == AUTOMATIC || system_mode()==SAFE_MODE)
    {
        network_connect(Network, 0, 0, NULL);
    }
#endif

#ifndef PARTICLE_NO_CLOUD
    //Initialize spark protocol callbacks for all System modes
    Particle_Protocol_Init();
#endif
}

static int cfod_count = 0;

/**
 * Use usb serial ymodem flasher to update firmware.
 */
void manage_serial_flasher()
{
    if(PARTICLE_FLASH_UPDATE == 3)
    {
        system_firmwareUpdate(&Serial);
    }
}

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection()
{
    if (PARTICLE_WLAN_RESET || PARTICLE_WLAN_SLEEP || WLAN_WD_TO())
    {
        if (PARTICLE_WLAN_STARTED)
        {
            DEBUG("Resetting WLAN!");
            auto was_sleeping = PARTICLE_WLAN_SLEEP;
            cloud_disconnect();
            network_off(Network, 0, 0, NULL);
            CLR_WLAN_WD();
            PARTICLE_WLAN_RESET = 0;
            PARTICLE_WLAN_STARTED = 0;
            PARTICLE_WLAN_SLEEP = was_sleeping;
            cfod_count = 0;
        }
    }
    else
    {
        if (!PARTICLE_WLAN_STARTED || (PARTICLE_CLOUD_CONNECT && !WLAN_CONNECTED))
        {
            if (!WLAN_DISCONNECT)
            {
                ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
            }
            network_connect(Network, 0, 0, NULL);
        }
    }
}

#ifndef PARTICLE_NO_CLOUD

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
    LED_SetRGBColor(RGB_COLOR_RED);

    while (Particle_Error_Count != 0)
    {
        LED_On(LED_RGB);
        HAL_Delay_Milliseconds(500);
        LED_Off(LED_RGB);
        HAL_Delay_Milliseconds(500);
        Particle_Error_Count--;
    }

    // TODO Send the Error Count to Cloud: NVMEM_Particle_File_Data[ERROR_COUNT_FILE_OFFSET]

    // Reset Error Count
    wlan_set_error_count(0);
}

void handle_cfod()
{
    if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
    {
        PARTICLE_WLAN_RESET = RESET_ON_CFOD;
        ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
    }

    if (Internet_Test() < 0)
    {
        // No Internet Connection
        if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
        {
            PARTICLE_WLAN_RESET = RESET_ON_CFOD;
            ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
        }

        Particle_Error_Count = 2;
    }
    else
    {
        // Cloud not Reachable
        Particle_Error_Count = 3;
    }
}

/**
 * Establishes a socket connection to the cloud if not already present.
 * - handles previous connection errors by flashing the LED
 * - attempts to open a socket to the cloud
 * - handles the CFOD
 *
 * On return, PARTICLE_CLOUD_SOCKETED is set to true if the socket connection was successful.
 */

void establish_cloud_connection()
{
    if (WLAN_DHCP && !PARTICLE_WLAN_SLEEP && !PARTICLE_CLOUD_SOCKETED)
    {
        if (Particle_Error_Count)
            handle_cloud_errors();

        PARTICLE_LED_FADE = 0;
        LED_SetRGBColor(RGB_COLOR_CYAN);
        if (in_cloud_backoff_period())
            return;

        LED_On(LED_RGB);
        if (Particle_Connect() >= 0)
        {
            cfod_count = 0;
            PARTICLE_CLOUD_SOCKETED = 1;
        }
        else
        {
            cloud_connection_failed();
            PARTICLE_CLOUD_SOCKETED = 0;
#if PLATFORM_ID<3
            if (!PARTICLE_WLAN_RESET)
                handle_cfod();
#endif
            wlan_set_error_count(Particle_Error_Count);
        }
    }
}

/**
 * Manages the handshake and cloud events when the cloud has a socket connected.
 * @param force_events
 */
void handle_cloud_connection(bool force_events)
{
    if (PARTICLE_CLOUD_SOCKETED)
    {
        if (!PARTICLE_CLOUD_CONNECTED)
        {
            int err = Particle_Handshake();
            if (err)
            {
                cloud_connection_failed();
                if (0 > err)
                {
                    // Wrong key error, red
                    LED_SetRGBColor(RGB_COLOR_RED);
                }
                else if (1 == err)
                {
                    // RSA decryption error, orange
                    LED_SetRGBColor(RGB_COLOR_ORANGE);
                }
                else if (2 == err)
                {
                    // RSA signature verification error, magenta
                    LED_SetRGBColor(RGB_COLOR_MAGENTA);
                }

                LED_On(LED_RGB);
                // delay a little to be sure the user sees the LED color, since
                // the socket may quickly disconnect and the connection retried, turning
                // the LED back to cyan
                system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                Particle_Disconnect(); // clean up the socket
                while ((HAL_Timer_Get_Milli_Seconds()-start)<250);
                PARTICLE_CLOUD_SOCKETED = 0;

            }
            else
            {
                PARTICLE_CLOUD_CONNECTED = 1;
                cloud_failed_connection_attempts = 0;
            }
        }

        if (PARTICLE_FLASH_UPDATE || force_events || System.mode() != MANUAL)
        {
            Particle_Process_Events();
        }
    }
}

void manage_cloud_connection(bool force_events)
{
    if (PARTICLE_CLOUD_CONNECT == 0)
    {
        cloud_disconnect();
    }
    else // cloud connection is wanted
    {
        establish_cloud_connection();

        handle_cloud_connection(force_events);
    }
}
#endif

void Particle_Idle_Events(bool force_events/*=false*/)
{
    HAL_Notify_WDT();

    ON_EVENT_DELTA();
    system_loop_total_millis = 0;

    manage_serial_flasher();

    manage_network_connection();

    manage_smart_config();

    manage_ip_config();

    CLOUD_FN(manage_cloud_connection(force_events), (void)0);
}


/*
 * @brief This should block for a certain number of milliseconds and also execute system_idle_loop
 */
void system_delay_ms(unsigned long ms)
{
    system_tick_t system_loop_elapsed_millis = PARTICLE_LOOP_DELAY_MILLIS;
    system_loop_total_millis += ms;

    system_tick_t last_millis = HAL_Timer_Get_Milli_Seconds();

    while (1)
    {
        HAL_Notify_WDT();

        system_tick_t current_millis = HAL_Timer_Get_Milli_Seconds();
        system_tick_t elapsed_millis = current_millis - last_millis;

        //Check for wrapping
        if (elapsed_millis >= 0x80000000)
        {
            elapsed_millis = last_millis + current_millis;
        }

        if (elapsed_millis >= ms)
        {
            break;
        }

        if (PARTICLE_WLAN_SLEEP)
        {
            //Do not yield for Particle_Idle()
        }
        else if ((elapsed_millis >= system_loop_elapsed_millis) || (system_loop_total_millis >= PARTICLE_LOOP_DELAY_MILLIS))
        {
            system_loop_elapsed_millis = elapsed_millis + PARTICLE_LOOP_DELAY_MILLIS;
            //system_loop_total_millis is reset to 0 in Particle_Idle()
            do
            {
                //Run once if the above condition passes
                Particle_Idle();
            }
            while (PARTICLE_FLASH_UPDATE); //loop during OTA update
        }
    }
}

void cloud_disconnect()
{
#ifndef PARTICLE_NO_CLOUD
    if (PARTICLE_CLOUD_SOCKETED || PARTICLE_CLOUD_CONNECTED)
    {
        Particle_Disconnect();

        PARTICLE_FLASH_UPDATE = 0;
        PARTICLE_CLOUD_CONNECTED = 0;
        PARTICLE_CLOUD_SOCKETED = 0;
        Particle_Error_Count = 0;

        if (!WLAN_DISCONNECT && !WLAN_SMART_CONFIG_START)
        {
            LED_SetRGBColor(RGB_COLOR_GREEN);
            LED_On(LED_RGB);
        }
    }
#endif
}
