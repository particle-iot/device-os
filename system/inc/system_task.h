/**
 ******************************************************************************
 * @file    spark_wiring_wlan.h
 * @author  Satish Nair and Zachary Crockett
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_wlan.c module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_WLAN_H
#define __SPARK_WLAN_H

#include "socket_hal.h"
#include "wlan_hal.h"    

extern "C" {

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInuS);
void SPARK_WLAN_Setup(void (*presence_announcement_callback)(void));

/**
 * Run background processing. This function should be called as often as possible by user code.
 * @param force_events when true, runs cloud event pump in addition to maintaining the wifi and cloud connection.
 */
void Spark_Idle_Events(bool force_events);
inline void Spark_Idle() { Spark_Idle_Events(false); }

/**
 * The old method 
 */
void SPARK_WLAN_Loop(void) __attribute__ ((deprecated("Please use Spark.process() instead.")));
inline void SPARK_WLAN_Loop(void) { Spark_Idle(); }

void SPARK_WLAN_SmartConfigProcess();

/* Spark Cloud APIs */
extern int Spark_Connect(void);
extern int Spark_Disconnect(void);
extern int Spark_Process_API_Response(void);

extern volatile uint32_t TimingFlashUpdateTimeout;

extern volatile uint8_t SPARK_WLAN_RESET;
extern volatile uint8_t SPARK_WLAN_SLEEP;
extern volatile uint8_t SPARK_WLAN_STARTED;
extern volatile uint8_t SPARK_CLOUD_CONNECT;
extern volatile uint8_t SPARK_CLOUD_SOCKETED;
extern volatile uint8_t SPARK_CLOUD_CONNECTED;
extern volatile uint8_t SPARK_FLASH_UPDATE;
extern volatile uint8_t SPARK_LED_FADE;

extern volatile uint8_t WLAN_DISCONNECT;
extern volatile uint8_t WLAN_DHCP;
extern volatile uint8_t WLAN_MANUAL_CONNECT;
extern volatile uint8_t WLAN_DELETE_PROFILES;
extern volatile uint8_t WLAN_SMART_CONFIG_START;
extern volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
extern volatile uint8_t WLAN_SERIAL_CONFIG_DONE;

extern volatile uint8_t Spark_Error_Count;
extern volatile uint8_t Cloud_Handshake_Error_Count;

extern volatile system_tick_t spark_loop_total_millis;

extern sock_handle_t sparkSocket;

void spark_delay_ms(unsigned long ms);

}

#endif  /*__SPARK_WLAN_H*/
