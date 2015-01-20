/**
  Copyright (c) 2013-2015 Spark Labs, Inc.  All rights reserved.

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


#ifndef CLOUD_H
#define	CLOUD_H

#include "spark_wiring_stream.h"

class SparkProtocol;

extern "C" {
/**
 * Functions for managing the cloud connection, performing cloud operations
 * and system upgrades.
 */

int Internet_Test(void);

int Spark_Connect(void);
int Spark_Disconnect(void);

void Spark_Protocol_Init(void);
int Spark_Handshake(void);
bool Spark_Communication_Loop(void);
void Multicast_Presence_Announcement(void);
void Spark_Signal(bool on);
void Spark_SetTime(unsigned long dateTime);
void Spark_Process_Events();
void Spark_Prepare_To_Save_File(uint32_t sFlashAddress, uint32_t fileSize);
void Spark_Prepare_For_Firmware_Update(void);
void Spark_Finish_Firmware_Update(void);
uint16_t Spark_Save_Firmware_Chunk(unsigned char *buf, uint32_t bufLen);

extern volatile uint8_t LED_Spark_Signal;
void LED_Signaling_Override(void);


typedef enum
{
	BOOLEAN = 1, INT = 2, STRING = 4, DOUBLE = 9
} Spark_Data_TypeDef;

typedef enum
{
	PUBLIC = 0, PRIVATE = 1
} Spark_Event_TypeDef;

void spark_variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType);
void spark_function(const char *funcKey, int (*pFunc)(String paramString));
void spark_process(void);
void spark_connect(void);
void spark_disconnect(void);
SparkProtocol* spark_protocol_instance(void);

String bytes2hex(const uint8_t* buf, unsigned len);
String spark_deviceID(void);

#define SPARK_BUF_LEN			        600

//#define SPARK_SERVER_IP			        "54.235.79.249"
#define SPARK_SERVER_PORT		        5683

#define SPARK_LOOP_DELAY_MILLIS		        1000    //1sec
#define SPARK_RECEIVE_DELAY_MILLIS              10      //10ms

#define TIMING_FLASH_UPDATE_TIMEOUT             30000   //30sec

#define USER_VAR_MAX_COUNT		        10
#define USER_VAR_KEY_LENGTH		        12

#define USER_FUNC_MAX_COUNT		        4
#define USER_FUNC_KEY_LENGTH		        12
#define USER_FUNC_ARG_LENGTH		        64

#define USER_EVENT_NAME_LENGTH		        64
#define USER_EVENT_DATA_LENGTH		        64

}

#endif	/* CLOUD_H */

