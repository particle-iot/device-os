/**
 ******************************************************************************
 * @file    spark_utilities.h
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_utilities.c module
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
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "main.h"
#include "spark_wiring_string.h"

#define BYTE_N(x,n)					(((x) >> n*8) & 0x000000FF)

#define SPARK_BUF_LEN				600

//#define SPARK_SERVER_IP				"54.235.79.249"
#define SPARK_SERVER_PORT			5683

#define USER_VAR_MAX_COUNT			10
#define USER_VAR_KEY_LENGTH			12

#define USER_FUNC_MAX_COUNT			4
#define USER_FUNC_KEY_LENGTH		12
#define USER_FUNC_ARG_LENGTH		64

#define USER_EVENT_MAX_COUNT		3
#define USER_EVENT_NAME_LENGTH		16
#define USER_EVENT_RESULT_LENGTH	64

typedef enum
{
	SLEEP_MODE_WLAN = 0, SLEEP_MODE_DEEP = 1
} Spark_Sleep_TypeDef;

typedef enum
{
	BOOLEAN = 1, INT = 2, STRING = 4, DOUBLE = 9
} Spark_Data_TypeDef;

class RGBClass {
private:
	static bool _control;
public:
	static bool controlled(void);
	static void control(bool);
	static void color(int, int, int);
  static void brightness(uint8_t);
};

class SparkClass {
public:
	static void variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType);
	static void function(const char *funcKey, int (*pFunc)(String paramString));
	static void event(const char *eventName, char *eventResult);
	static void sleep(Spark_Sleep_TypeDef sleepMode, long seconds);
	static void sleep(long seconds);
	static bool connected(void);
	static int connect(void);
	static int disconnect(void);
	static String deviceID(void);
};

extern RGBClass RGB;
extern SparkClass Spark;

extern __IO uint8_t LED_Spark_Signal;

#ifdef __cplusplus
extern "C" {
#endif

int Internet_Test(void);

int Spark_Connect(void);
int Spark_Disconnect(void);
void Spark_ConnectAbort_WLANReset(void);

void Spark_Protocol_Init(void);
int Spark_Handshake(void);
bool Spark_Communication_Loop(void);
void Multicast_Presence_Announcement(void);
void Spark_Signal(bool on);

int userVarType(const char *varKey);
void *getUserVar(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString);

void userEventSend(void);

long socket_connect(long sd, const sockaddr *addr, long addrlen);

void setup() __attribute__ ((weak));
void loop() __attribute__ ((weak));

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif  /* __SPARK_UTILITIES_H */
