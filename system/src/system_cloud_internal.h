/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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


#ifndef SYSTEM_CLOUD_INTERNAL_H
#define	SYSTEM_CLOUD_INTERNAL_H

#include "system_cloud.h"

/**
 * Functions for managing the cloud connection, performing cloud operations
 * and system upgrades.
 */

int Internet_Test(void);

int spark_cloud_socket_connect(void);
int spark_cloud_socket_disconnect(void);

void Spark_Protocol_Init(void);
int Spark_Handshake(bool presence_announce);
bool Spark_Communication_Loop(void);
void Multicast_Presence_Announcement(void);
void Spark_Signal(bool on, unsigned, void*);
void Spark_SetTime(unsigned long dateTime);
void Spark_Process_Events();
void Spark_Sleep();
void Spark_Wake();
extern volatile uint8_t LED_Spark_Signal;
void LED_Signaling_Override(void);

void system_set_time(time_t time, unsigned param, void* reserved);

char* bytes2hexbuf(const uint8_t* buf, unsigned len, char* output);
String bytes2hex(const uint8_t* buf, unsigned len);

uint8_t spark_cloud_socket_closed();

bool spark_function_internal(const cloud_function_descriptor* desc, void* reserved);
int call_raw_user_function(void* data, const char* param, void* reserved);

String spark_deviceID();

struct User_Var_Lookup_Table_t
{
    const void *userVar;
    Spark_Data_TypeDef userVarType;
    char userVarKey[USER_VAR_KEY_LENGTH+1];

    const void* (*update)(const char* name, Spark_Data_TypeDef varType, const void* var, void* reserved);
};


struct User_Func_Lookup_Table_t
{
    void* pUserFuncData;
    cloud_function_t pUserFunc;
    char userFuncKey[USER_FUNC_KEY_LENGTH+1];
};


User_Var_Lookup_Table_t* find_var_by_key_or_add(const char* varKey);
User_Func_Lookup_Table_t* find_func_by_key_or_add(const char* funcKey);

extern ProtocolFacade* sp;


/**
 * regular async update to check that the cloud has been serviced recently.
 * After 15 seconds of inactivity, the LED status is changed to
 * @return
 */
bool system_cloud_active();


#endif	/* SYSTEM_CLOUD_INTERNAL_H */

