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

extern "C" {

#include "hw_config.h"
#include "evnt_handler.h"
#include "hci.h"
#include "wlan.h"
#include "nvmem.h"
#include "socket.h"
#include "netapp.h"
#include "security.h"

/* CC3000 EEPROM - Spark File Data Storage */
#define NVMEM_SPARK_FILE_ID			14	//Do not change this ID
#define NVMEM_SPARK_FILE_SIZE		16	//Change according to requirement
#define WLAN_PROFILE_FILE_OFFSET	0
#define WLAN_POLICY_FILE_OFFSET		1
#define WLAN_TIMEOUT_FILE_OFFSET	2
#define ERROR_COUNT_FILE_OFFSET		3

#define TIMING_SPARK_COMM_TIMEOUT	30000	//30sec

void Set_NetApp_Timeout(void);
void Clear_NetApp_Dhcp(void);
void Start_Smart_Config(void);

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length);
char *WLAN_Firmware_Patch(unsigned long *length);
char *WLAN_Driver_Patch(unsigned long *length);
char *WLAN_BootLoader_Patch(unsigned long *length);

void SPARK_WLAN_Setup(void (*presence_announcement_callback)(void));
void SPARK_WLAN_Loop(void);
void SPARK_WLAN_Timing(void);

/* Spark Cloud APIs */
extern int Spark_Connect(void);
extern int Spark_Disconnect(void);
extern int Spark_Process_API_Response(void);

extern tNetappIpconfigRetArgs ip_config;

extern __IO uint32_t TimingSparkCommTimeout;

extern uint8_t WLAN_DHCP;
extern __IO uint8_t SPARK_WLAN_RESET;
extern __IO uint8_t SPARK_WLAN_SLEEP;
extern __IO uint8_t SPARK_SOCKET_HANDSHAKE;
extern __IO uint8_t SPARK_SOCKET_CONNECTED;
extern __IO uint8_t SPARK_HANDSHAKE_COMPLETED;
extern __IO uint8_t SPARK_FLASH_UPDATE;
extern __IO uint8_t SPARK_LED_FADE;

extern uint8_t WLAN_DELETE_PROFILES;
extern uint8_t WLAN_SMART_CONFIG_START;

extern __IO uint8_t Spark_Error_Count;

}

#endif  /*__SPARK_WLAN_H*/
