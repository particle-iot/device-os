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
#include "spark_macros.h"
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

volatile system_tick_t spark_loop_total_millis = 0;

// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

volatile uint8_t SPARK_LED_FADE = 1;

volatile uint8_t Spark_Error_Count;

void Network_Setup()
{
#if !PARTICLE_NO_NETWORK
    network.setup();

    /* Trigger a WLAN device */
    if (system_mode() == AUTOMATIC || system_mode()==SAFE_MODE)
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
            DEBUG("Resetting WLAN!");
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
        if (!SPARK_WLAN_STARTED || (SPARK_CLOUD_CONNECT && !network.connected()))
        {
            network.connect();
        }
    }
}

#ifndef SPARK_NO_CLOUD

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

    while (Spark_Error_Count != 0)
    {
        LED_On(LED_RGB);
        HAL_Delay_Milliseconds(500);
        LED_Off(LED_RGB);
        HAL_Delay_Milliseconds(500);
        Spark_Error_Count--;
    }

    // TODO Send the Error Count to Cloud: NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]

    // Reset Error Count
    wlan_set_error_count(0);
}

void handle_cfod()
{
    if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
    {
        SPARK_WLAN_RESET = RESET_ON_CFOD;
        ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
    }

    if (Internet_Test() < 0)
    {
        // No Internet Connection
        if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
        {
            SPARK_WLAN_RESET = RESET_ON_CFOD;
            ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
        }

        Spark_Error_Count = 2;
    }
    else
    {
        // Cloud not Reachable
        Spark_Error_Count = 3;
    }
}

/**
 * Establishes a socket connection to the cloud if not already present.
 * - handles previous connection errors by flashing the LED
 * - attempts to open a socket to the cloud
 * - handles the CFOD
 *
 * On return, SPARK_CLOUD_SOCKETED is set to true if the socket connection was successful.
 */

void establish_cloud_connection()
{
    if (network.ready() && !SPARK_WLAN_SLEEP && !SPARK_CLOUD_SOCKETED)
    {
        if (Spark_Error_Count)
            handle_cloud_errors();

        SPARK_LED_FADE = 0;
        LED_SetRGBColor(RGB_COLOR_CYAN);
        if (in_cloud_backoff_period())
            return;

        LED_On(LED_RGB);
        if (Spark_Connect() >= 0)
        {
            cfod_count = 0;
            SPARK_CLOUD_SOCKETED = 1;
        }
        else
        {
            if (SPARK_WLAN_RESET)
                return;
            cloud_connection_failed();
            SPARK_CLOUD_SOCKETED = 0;
            handle_cfod();
            wlan_set_error_count(Spark_Error_Count);
        }
    }
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
            int err = Spark_Handshake();
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
                Spark_Disconnect(); // clean up the socket
                while ((HAL_Timer_Get_Milli_Seconds()-start)<250);
                SPARK_CLOUD_SOCKETED = 0;

            }
            else
            {
                SPARK_CLOUD_CONNECTED = 1;
                cloud_failed_connection_attempts = 0;
            }
        }

        if (SPARK_FLASH_UPDATE || force_events || System.mode() != MANUAL)
        {
            Spark_Process_Events();
        }
    }
}

void manage_cloud_connection(bool force_events)
{
    if (SPARK_CLOUD_CONNECT == 0)
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

void Spark_Idle_Events(bool force_events/*=false*/)
{
    HAL_Notify_WDT();

    ON_EVENT_DELTA();
    spark_loop_total_millis = 0;

    manage_serial_flasher();

    manage_network_connection();

    manage_smart_config();

    manage_ip_config();

    CLOUD_FN(manage_cloud_connection(force_events), (void)0);
}


/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void system_delay_ms(unsigned long ms, bool force_no_background_loop=false)
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
            spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
            //spark_loop_total_millis is reset to 0 in Spark_Idle()
            do
            {
                //Run once if the above condition passes
                Spark_Idle();
            }
            while (SPARK_FLASH_UPDATE); //loop during OTA update
        }
    }
}

void cloud_disconnect(bool close_socket)
{
#ifndef SPARK_NO_CLOUD
    if (SPARK_CLOUD_SOCKETED || SPARK_CLOUD_CONNECTED)
    {
        if (close_socket)
            Spark_Disconnect();

        SPARK_FLASH_UPDATE = 0;
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;

        if (!network.manual_disconnect() && !network.listening())
        {
            LED_SetRGBColor(RGB_COLOR_GREEN);
            LED_On(LED_RGB);
        }
    }
    Spark_Error_Count = 0;

#endif
}
