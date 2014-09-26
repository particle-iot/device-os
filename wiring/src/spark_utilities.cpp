/**
 ******************************************************************************
 * @file    spark_utilities.cpp
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * @brief   
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
#include "spark_utilities.h"
#include "spark_wiring.h"
#include "spark_wiring_network.h"
#include "spark_flasher_ymodem.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "core_subsys_hal.h"
#include "deviceid_hal.h"
#include "ota_flash_hal.h"
#include "string.h"
#include <stdarg.h>

using namespace spark;

volatile uint32_t TimingFlashUpdateTimeout;

SparkProtocol spark_protocol;

sock_handle_t sparkSocket = SOCKET_INVALID;
sockaddr tSocketAddr;

//char digits[] = "0123456789";

extern uint8_t LED_RGB_BRIGHTNESS;

// LED_Signaling_Override
volatile uint8_t LED_Spark_Signal;
const uint32_t VIBGYOR_Colors[] = {
  0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000};
const int VIBGYOR_Size = sizeof(VIBGYOR_Colors) / sizeof(uint32_t);
int VIBGYOR_Index;

uint8_t User_Var_Count;
uint8_t User_Func_Count;

struct User_Var_Lookup_Table_t
{
	void *userVar;
	char userVarKey[USER_VAR_KEY_LENGTH];
	Spark_Data_TypeDef userVarType;
} User_Var_Lookup_Table[USER_VAR_MAX_COUNT];

struct User_Func_Lookup_Table_t
{
	int (*pUserFunc)(String userArg);
	char userFuncKey[USER_FUNC_KEY_LENGTH];
	char userFuncArg[USER_FUNC_ARG_LENGTH];
	int userFuncRet;
	bool userFuncSchedule;
} User_Func_Lookup_Table[USER_FUNC_MAX_COUNT];

/*
static unsigned char uitoa(unsigned int cNum, char *cString);
static unsigned int atoui(char *cString);
static uint8_t atoc(char data);
*/

/*
static uint16_t atoshort(char b1, char b2);
static unsigned char ascii_to_char(char b1, char b2);

static void str_cpy(char dest[], char src[]);
static int str_cmp(char str1[], char str2[]);
static int str_len(char str[]);
static void sub_str(char dest[], char src[], int offset, int len);
*/

SystemClass System;
RGBClass RGB;
SparkClass Spark;

System_Mode_TypeDef SystemClass::_mode = AUTOMATIC;

SystemClass::SystemClass()
{
}

SystemClass::SystemClass(System_Mode_TypeDef mode)
{
  switch(mode)
  {
    case AUTOMATIC:
      _mode = AUTOMATIC;
      SPARK_CLOUD_CONNECT = 1;
      break;

    case SEMI_AUTOMATIC:
      _mode = SEMI_AUTOMATIC;
      SPARK_CLOUD_CONNECT = 0;
      SPARK_WLAN_SLEEP = 1;
      break;

    case MANUAL:
      _mode = MANUAL;
      SPARK_CLOUD_CONNECT = 0;
      SPARK_WLAN_SLEEP = 1;
      break;
  }
}

System_Mode_TypeDef SystemClass::mode(void)
{
  return _mode;
}

void SystemClass::serialSaveFile(Stream *serialObj, uint32_t sFlashAddress)
{
  Serial_Flash_Update(serialObj, sFlashAddress);
  SPARK_FLASH_UPDATE = 0;
  TimingFlashUpdateTimeout = 0;
}

void SystemClass::serialFirmwareUpdate(Stream *serialObj)
{
  bool status = Serial_Flash_Update(serialObj, HAL_OTA_FlashAddress());
  if(status == true)
  {
    serialObj->println("Restarting system to apply firmware update...");
    delay(100);
    HAL_FLASH_End();
  }
  else
  {
    SPARK_FLASH_UPDATE = 0;
    TimingFlashUpdateTimeout = 0;
  }
}

void SystemClass::factoryReset(void)
{
  //This method will work only if the Core is supplied
  //with the latest version of Bootloader
  HAL_Core_Factory_Reset();
}

void SystemClass::bootloader(void)
{
  //The drawback here being it will enter bootloader mode until firmware
  //is loaded again. Require bootloader changes for proper working.
  HAL_Core_Enter_Bootloader();
}

void SystemClass::reset(void)
{
  HAL_Core_System_Reset();
}

bool RGBClass::_control = false;

bool RGBClass::controlled(void)
{
	return _control;
}

void RGBClass::control(bool override)
{
	if(override == _control)
		return;
	else if (override)
		LED_Signaling_Start();
	else
		LED_Signaling_Stop();

	_control = override;
}

void RGBClass::color(uint32_t rgb) {
    color((rgb>>16)&0xFF, (rgb>>8)&0xFF, (rgb)&0xFF);
}

void RGBClass::color(int red, int green, int blue)
{
        if (true != _control)
                return;

        LED_SetSignalingColor(red << 16 | green << 8 | blue);
        LED_On(LED_RGB);
}

void RGBClass::brightness(uint8_t brightness, bool update)
{
	LED_SetBrightness(brightness);
    if (_control && update)
        LED_On(LED_RGB);
}

void SparkClass::variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType)
{
  if (NULL != userVar && NULL != varKey)
  {
    if (User_Var_Count == USER_VAR_MAX_COUNT)
      return;

    for (int i = 0; i < User_Var_Count; i++)
    {
      if (User_Var_Lookup_Table[i].userVar == userVar &&
          (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH)))
      {
        return;
      }
    }

    User_Var_Lookup_Table[User_Var_Count].userVar = userVar;
    User_Var_Lookup_Table[User_Var_Count].userVarType = userVarType;
    memset(User_Var_Lookup_Table[User_Var_Count].userVarKey, 0, USER_VAR_KEY_LENGTH);
    memcpy(User_Var_Lookup_Table[User_Var_Count].userVarKey, varKey, USER_VAR_KEY_LENGTH);
    User_Var_Count++;
  }
}

void SparkClass::function(const char *funcKey, int (*pFunc)(String paramString))
{
	int i = 0;
	if(NULL != pFunc && NULL != funcKey)
	{
		if(User_Func_Count == USER_FUNC_MAX_COUNT)
			return;

		for(i = 0; i < User_Func_Count; i++)
		{
			if(User_Func_Lookup_Table[i].pUserFunc == pFunc && (0 == strncmp(User_Func_Lookup_Table[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH)))
			{
				return;
			}
		}

		User_Func_Lookup_Table[User_Func_Count].pUserFunc = pFunc;
		memset(User_Func_Lookup_Table[User_Func_Count].userFuncArg, 0, USER_FUNC_ARG_LENGTH);
		memset(User_Func_Lookup_Table[User_Func_Count].userFuncKey, 0, USER_FUNC_KEY_LENGTH);
		memcpy(User_Func_Lookup_Table[User_Func_Count].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH);
		User_Func_Lookup_Table[User_Func_Count].userFuncSchedule = false;
		User_Func_Count++;
	}
}

void SparkClass::publish(const char *eventName)
{
  spark_protocol.send_event(eventName, NULL, 60, EventType::PUBLIC);
}

void SparkClass::publish(const char *eventName, const char *eventData)
{
  spark_protocol.send_event(eventName, eventData, 60, EventType::PUBLIC);
}

void SparkClass::publish(const char *eventName, const char *eventData, int ttl)
{
  spark_protocol.send_event(eventName, eventData, ttl, EventType::PUBLIC);
}

void SparkClass::publish(const char *eventName, const char *eventData, int ttl, Spark_Event_TypeDef eventType)
{
  spark_protocol.send_event(eventName, eventData, ttl, (eventType ? EventType::PRIVATE : EventType::PUBLIC));
}

void SparkClass::publish(String eventName)
{
  publish(eventName.c_str());
}

void SparkClass::publish(String eventName, String eventData)
{
  publish(eventName.c_str(), eventData.c_str());
}

void SparkClass::publish(String eventName, String eventData, int ttl)
{
  publish(eventName.c_str(), eventData.c_str(), ttl);
}

void SparkClass::publish(String eventName, String eventData, int ttl, Spark_Event_TypeDef eventType)
{
  publish(eventName.c_str(), eventData.c_str(), ttl, eventType);
}

bool SparkClass::subscribe(const char *eventName, EventHandler handler)
{
  bool success = spark_protocol.add_event_handler(eventName, handler);
  if (success)
  {
    success = spark_protocol.send_subscription(eventName, SubscriptionScope::FIREHOSE);
  }
  return success;
}

bool SparkClass::subscribe(const char *eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope)
{
  bool success = spark_protocol.add_event_handler(eventName, handler);
  if (success)
  {
    success = spark_protocol.send_subscription(eventName, SubscriptionScope::MY_DEVICES);
  }
  return success;
}

bool SparkClass::subscribe(const char *eventName, EventHandler handler, const char *deviceID)
{
  bool success = spark_protocol.add_event_handler(eventName, handler);
  if (success)
  {
    success = spark_protocol.send_subscription(eventName, deviceID);
  }
  return success;
}

bool SparkClass::subscribe(String eventName, EventHandler handler)
{
  return subscribe(eventName.c_str(), handler);
}

bool SparkClass::subscribe(String eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope)
{
  return subscribe(eventName.c_str(), handler, scope);
}

bool SparkClass::subscribe(String eventName, EventHandler handler, String deviceID)
{
  return subscribe(eventName.c_str(), handler, deviceID.c_str());
}

void SparkClass::syncTime(void)
{
  spark_protocol.send_time_request();
}

void SparkClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds)
{
#if defined (SPARK_RTC_ENABLE)
        HAL_RTC_Set_Alarm((uint32_t)seconds);

	switch(sleepMode)
	{
	case SLEEP_MODE_WLAN:
		WiFi.off();
		break;

	case SLEEP_MODE_DEEP:
	        HAL_Core_Enter_Standby_Mode();
		break;
	}
#endif
}

void SparkClass::sleep(long seconds)
{
	SparkClass::sleep(SLEEP_MODE_WLAN, seconds);
}

void SparkClass::sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
  HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode);
}

void SparkClass::sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
#if defined (SPARK_RTC_ENABLE)
  HAL_RTC_Set_Alarm((uint32_t)seconds);

  sleep(wakeUpPin, edgeTriggerMode);
#endif
}

inline uint8_t isSocketClosed()
{
  uint8_t closed  = socket_active_status(sparkSocket)==SOCKET_STATUS_INACTIVE;

  if(closed)
  {
      DEBUG("get_socket_active_status(sparkSocket=%d)==SOCKET_STATUS_INACTIVE", sparkSocket);
  }
  if(closed && sparkSocket != SOCKET_INVALID)
  {
      DEBUG("!!!!!!closed && sparkSocket(%d) != SOCKET_INVALID", sparkSocket);
  }
  if(sparkSocket == SOCKET_INVALID)
    {
      DEBUG("sparkSocket == SOCKET_INVALID");
      closed = true;
    }
  return closed;
}

bool SparkClass::connected(void)
{
	if(SPARK_CLOUD_SOCKETED && SPARK_CLOUD_CONNECTED)
		return true;
	else
		return false;
}

void SparkClass::connect(void)
{
	//Schedule Spark's cloud connection and handshake
        WiFi.connect();
	SPARK_CLOUD_CONNECT = 1;
}

void SparkClass::disconnect(void)
{
	//Schedule Spark's cloud disconnection
	SPARK_CLOUD_CONNECT = 0;
}

void SparkClass::process(void)
{
#ifdef SPARK_WLAN_ENABLE
  if (SPARK_CLOUD_SOCKETED && !Spark_Communication_Loop())
  {
    SPARK_FLASH_UPDATE = 0;
    SPARK_CLOUD_CONNECTED = 0;
    SPARK_CLOUD_SOCKETED = 0;
  }
#endif
}

inline void concat_nibble(String& result, uint8_t nibble) 
{
    char hex_digit = nibble + 48;
    if (57 < hex_digit)
        hex_digit += 39;
    result.concat(hex_digit);
}

String bytes2hex(const uint8_t* buf, unsigned len) 
{    
    String result;	
	for (unsigned i = 0; i < len; ++i)
	{
        concat_nibble(result, (buf[i] >> 4));
        concat_nibble(result, (buf[i] & 0xF));
	}
	return result;
}


String SparkClass::deviceID(void)
{
    unsigned len = HAL_device_ID(NULL, 0);
	uint8_t id[len];
    HAL_device_ID(id, len);
    return bytes2hex(id, len);
}    
 

// Returns number of bytes sent or -1 if an error occurred
int Spark_Send(const unsigned char *buf, int buflen)
{
  if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
  {
    DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
    //break from any blocking loop
    return -1;
  }

  // send returns negative numbers on error
  int bytes_sent = socket_send(sparkSocket, buf, buflen);
  return bytes_sent;
}

// Returns number of bytes received or -1 if an error occurred
int Spark_Receive(unsigned char *buf, int buflen)
{
  if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed())
  {
    //break from any blocking loop
    DEBUG("SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || isSocketClosed()");
    return -1;
  }
  return socket_receive(sparkSocket, buf, buflen, 0);  
}

void begin_flash_file(int flashType, unsigned int sFlashAddress, unsigned int fileSize) 
{
  RGB.control(true);
  RGB.color(RGB_COLOR_MAGENTA);
  SPARK_FLASH_UPDATE = flashType;
  TimingFlashUpdateTimeout = 0;
  HAL_FLASH_Begin(sFlashAddress, fileSize);  
}

void Spark_Prepare_To_Save_File(unsigned int sFlashAddress, unsigned int fileSize)
{
    begin_flash_file(2, sFlashAddress, fileSize);
}

void Spark_Prepare_For_Firmware_Update(void)
{
    begin_flash_file(1, HAL_OTA_FlashAddress(), HAL_OTA_FlashLength());
}

void Spark_Finish_Firmware_Update(void)
{
  if (SPARK_FLASH_UPDATE == 2)
  {
    SPARK_FLASH_UPDATE = 0;
    TimingFlashUpdateTimeout = 0;
  }
  else
  {
    //Reset the system to complete the OTA update
    HAL_FLASH_End();
  }
  RGB.control(false);
}

uint16_t Spark_Save_Firmware_Chunk(unsigned char *buf, long unsigned int buflen)
{
  uint16_t chunkUpdatedIndex;
  TimingFlashUpdateTimeout = 0;
  chunkUpdatedIndex = HAL_FLASH_Update(buf, buflen);
  LED_Toggle(LED_RGB);
  return chunkUpdatedIndex;
}

int numUserFunctions(void)
{
  return User_Func_Count;
}

void copyUserFunctionKey(char *destination, int function_index)
{
  memcpy(destination,
         User_Func_Lookup_Table[function_index].userFuncKey,
         USER_FUNC_KEY_LENGTH);
}

int numUserVariables(void)
{
  return User_Var_Count;
}

void copyUserVariableKey(char *destination, int variable_index)
{
  memcpy(destination,
         User_Var_Lookup_Table[variable_index].userVarKey,
         USER_VAR_KEY_LENGTH);
}

SparkReturnType::Enum wrapVarTypeInEnum(const char *varKey)
{
  switch (userVarType(varKey))
  {
    case 1:
      return SparkReturnType::BOOLEAN;
      break;

    case 4:
      return SparkReturnType::STRING;
      break;

    case 9:
      return SparkReturnType::DOUBLE;
      break;

    case 2:
    default:
      return SparkReturnType::INT;
  }
}

void Spark_Protocol_Init(void)
{
  if (!spark_protocol.is_initialized())
  {
    SparkCallbacks callbacks;
    callbacks.send = Spark_Send;
    callbacks.receive = Spark_Receive;
    callbacks.prepare_to_save_file = Spark_Prepare_To_Save_File;
    callbacks.prepare_for_firmware_update = Spark_Prepare_For_Firmware_Update;
    callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
    callbacks.calculate_crc = HAL_Core_Compute_CRC32;
    callbacks.save_firmware_chunk = Spark_Save_Firmware_Chunk;
    callbacks.signal = Spark_Signal;
    callbacks.millis = HAL_Timer_Get_Milli_Seconds;
    callbacks.set_time = Time.setTime;

    SparkDescriptor descriptor;
    descriptor.num_functions = numUserFunctions;
    descriptor.copy_function_key = copyUserFunctionKey;
    descriptor.call_function = userFuncSchedule;
    descriptor.num_variables = numUserVariables;
    descriptor.copy_variable_key = copyUserVariableKey;
    descriptor.variable_type = wrapVarTypeInEnum;
    descriptor.get_variable = getUserVar;
    descriptor.was_ota_upgrade_successful = HAL_OTA_Flashed_GetStatus;
    descriptor.ota_upgrade_status_sent = HAL_OTA_Flashed_ResetStatus;

    unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
    unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];

    SparkKeys keys;
    keys.server_public = pubkey;
    keys.core_private = private_key;

    HAL_FLASH_Read_ServerPublicKey(pubkey);
    HAL_FLASH_Read_CorePrivateKey(private_key);

    uint8_t id_length = HAL_device_ID(NULL, 0);
    uint8_t id[id_length];
    HAL_device_ID(id, id_length);
    spark_protocol.init((const char*)id, keys, callbacks, descriptor);
  }
}

int Spark_Handshake(void)
{
  Spark_Protocol_Init();
  spark_protocol.reset_updating();
  int err = spark_protocol.handshake();

  Multicast_Presence_Announcement();
  spark_protocol.send_time_request();

  char patchstr[8];  
  if (!core_read_subsystem_version(patchstr, 8)) {
      Spark.publish("spark/" SPARK_SUBSYSTEM_EVENT_NAME, patchstr, 60, PRIVATE);
  }
  return err;
}

// Returns true if all's well or
//         false on error, meaning we're probably disconnected
bool Spark_Communication_Loop(void)
{
  return spark_protocol.event_loop();
}

void Multicast_Presence_Announcement(void)
{
  long multicast_socket = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 > multicast_socket)
    return;

  unsigned char announcement[19];
  uint8_t id_length = HAL_device_ID(NULL, 0);
  uint8_t id[id_length];
  HAL_device_ID(id, id_length);
  spark_protocol.presence_announcement(announcement, (const char *)id);

  // create multicast address 224.0.1.187 port 5683
  sockaddr addr;
  addr.sa_family = AF_INET;
  addr.sa_data[0] = 0x16; // port MSB
  addr.sa_data[1] = 0x33; // port LSB
  addr.sa_data[2] = 0xe0; // IP MSB
  addr.sa_data[3] = 0x00;
  addr.sa_data[4] = 0x01;
  addr.sa_data[5] = 0xbb; // IP LSB

  //why loop here? Uncommenting this leads to SOS(HardFault Exception) on local cloud
  //for (int i = 3; i > 0; i--)
  {
    socket_sendto(multicast_socket, announcement, 19, 0, &addr, sizeof(sockaddr));
  }

  socket_close(multicast_socket);
}

/* This function MUST NOT BlOCK!
 * It will be executed every 1ms if LED_Signaling_Start() is called
 * and stopped as soon as LED_Signaling_Stop() is called */
void LED_Signaling_Override(void)
{
  static uint8_t LED_Signaling_Timing =0;
  if (0 < LED_Signaling_Timing)
  {
    --LED_Signaling_Timing;
  }
  else
  {
    LED_SetSignalingColor(VIBGYOR_Colors[VIBGYOR_Index]);
    LED_On(LED_RGB);

    LED_Signaling_Timing = 100;	// 100 ms

    ++VIBGYOR_Index;
    if(VIBGYOR_Index >= VIBGYOR_Size)
    {
      VIBGYOR_Index = 0;
    }
  }
}

void Spark_Signal(bool on)
{
  if (on)
  {
    LED_Signaling_Start();
    LED_Spark_Signal = 1;
  }
  else
  {
    LED_Signaling_Stop();
    LED_Spark_Signal = 0;
  }
}

int Internet_Test(void)
{
    long testSocket;
    sockaddr testSocketAddr;
    int testResult = 0;
    DEBUG("socket");
    testSocket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DEBUG("socketed testSocket=%d",testSocket);


    if (testSocket < 0)
    {
        return -1;
    }

	// the family is always AF_INET
    testSocketAddr.sa_family = AF_INET;

	// the destination port: 53
    testSocketAddr.sa_data[0] = 0;
    testSocketAddr.sa_data[1] = 53;

    // the destination IP address: 8.8.8.8
    testSocketAddr.sa_data[2] = 8;
    testSocketAddr.sa_data[3] = 8;
    testSocketAddr.sa_data[4] = 8;
    testSocketAddr.sa_data[5] = 8;

    uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    DEBUG("connect");
    testResult = socket_connect(testSocket, &testSocketAddr, sizeof(testSocketAddr));
    DEBUG("connected testResult=%d",testResult);
    SPARK_WLAN_SetNetWatchDog(ot);

#if defined(SEND_ON_CLOSE)
    DEBUG("send");
    char c = 0;
    int rc = send(testSocket, &c,1, 0);
    DEBUG("send %d",rc);
#endif
    DEBUG("Close");
    socket_close(testSocket);

    //if connection fails, testResult returns -1
    return testResult;
}

// Same return value as connect(), -1 on error
int Spark_Connect(void)
{
  DEBUG("sparkSocket Now =%d",sparkSocket);

  // Close Original

  Spark_Disconnect();

  sparkSocket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  DEBUG("socketed sparkSocket=%d",sparkSocket);

  if (sparkSocket < 0)
  {
    return -1;
  }

  // the family is always AF_INET
  tSocketAddr.sa_family = AF_INET;

  // the destination port
  tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
  tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);

  ServerAddress server_addr;
  HAL_FLASH_Read_ServerAddress(&server_addr);

  uint32_t ip_addr = 0;

  switch (server_addr.addr_type)
  {
    case IP_ADDRESS:
      ip_addr = server_addr.ip;
      break;

    default:
    case INVALID_INTERNET_ADDRESS:
    {
      const char default_domain[] = "device.spark.io";
      // Make sure we copy the NULL terminator, so subsequent strlen() calls on server_addr.domain return the correct length
      memcpy(server_addr.domain, default_domain, strlen(default_domain) + 1);
      // and fall through to domain name case
    }

    case DOMAIN_NAME:
      // CC3000 unreliability workaround, usually takes 2 or 3 attempts
      int attempts = 10;
      while (!ip_addr && 0 < --attempts)
      {
        inet_gethostbyname(server_addr.domain, strnlen(server_addr.domain, 126), &ip_addr);
      }
  }

  if (!ip_addr)
  {
    // final fallback
    ip_addr = (54 << 24) | (208 << 16) | (229 << 8) | 4;
  }

  tSocketAddr.sa_data[2] = BYTE_N(ip_addr, 3);
  tSocketAddr.sa_data[3] = BYTE_N(ip_addr, 2);
  tSocketAddr.sa_data[4] = BYTE_N(ip_addr, 1);
  tSocketAddr.sa_data[5] = BYTE_N(ip_addr, 0);

  uint32_t ot = SPARK_WLAN_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
  DEBUG("connect");
  int rv = socket_connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));
  DEBUG("connected connect=%d",rv);
  SPARK_WLAN_SetNetWatchDog(ot);
  return rv;
}

int Spark_Disconnect(void)
{
  int retVal= 0;
  DEBUG("");
  if (sparkSocket >= 0)
  {
#if defined(SEND_ON_CLOSE)
      DEBUG("send");
      char c = 0;
      int rc = send(sparkSocket, &c,1, 0);
      DEBUG("send %d",rc);
#endif
      DEBUG("Close");
      retVal = socket_close(sparkSocket);
      DEBUG("Closed retVal=%d", retVal);
      sparkSocket = SOCKET_INVALID;
  }
  return retVal;
}

int userVarType(const char *varKey)
{
	for (int i = 0; i < User_Var_Count; ++i)
	{
		if (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			return User_Var_Lookup_Table[i].userVarType;
		}
	}
	return -1;
}

void *getUserVar(const char *varKey)
{
	for (int i = 0; i < User_Var_Count; ++i)
	{
		if (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			return User_Var_Lookup_Table[i].userVar;
		}
	}
	return NULL;
}

int userFuncSchedule(const char *funcKey, const char *paramString)
{
	String pString(paramString);
	int i = 0;
	for(i = 0; i < User_Func_Count; i++)
	{
		if(NULL != paramString && (0 == strncmp(User_Func_Lookup_Table[i].userFuncKey, funcKey, USER_FUNC_KEY_LENGTH)))
		{
			size_t paramLength = strlen(paramString);
			if(paramLength > USER_FUNC_ARG_LENGTH)
				paramLength = USER_FUNC_ARG_LENGTH;
			memcpy(User_Func_Lookup_Table[i].userFuncArg, paramString, paramLength);
			User_Func_Lookup_Table[i].userFuncSchedule = true;
			//return User_Func_Lookup_Table[i].pUserFunc(User_Func_Lookup_Table[i].userFuncArg);
			return User_Func_Lookup_Table[i].pUserFunc(pString);
		}
	}
	return -1;
}

// Convert unsigned integer to ASCII in decimal base
/*
static unsigned char uitoa(unsigned int cNum, char *cString)
{
    char* ptr;
    unsigned int uTemp = cNum;
    unsigned char length;

    // value 0 is a special case
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    // Find out the length of the number, in decimal base
    length = 0;
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = digits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

// Convert ASCII to unsigned integer
static unsigned int atoui(char *cString)
{
	unsigned int cNum = 0;
	if (cString)
	{
		while (*cString && *cString <= '9' && *cString >= '0')
		{
			cNum = (cNum * 10) + (*cString - '0');
			cString++;
		}
	}
	return cNum;
}

//Convert nibble to hexdecimal from ASCII
static uint8_t atoc(char data)
{
	unsigned char ucRes = 0;

	if ((data >= 0x30) && (data <= 0x39))
	{
		ucRes = data - 0x30;
	}
	else
	{
		if (data == 'a')
		{
			ucRes = 0x0a;;
		}
		else if (data == 'b')
		{
			ucRes = 0x0b;
		}
		else if (data == 'c')
		{
			ucRes = 0x0c;
		}
		else if (data == 'd')
		{
			ucRes = 0x0d;
		}
		else if (data == 'e')
		{
			ucRes = 0x0e;
		}
		else if (data == 'f')
		{
			ucRes = 0x0f;
		}
	}
	return ucRes;
}
*/

/*
// Convert 2 nibbles in ASCII into a short number
static uint16_t atoshort(char b1, char b2)
{
	uint16_t usRes;
	usRes = (atoc(b1)) * 16 | atoc(b2);
	return usRes;
}

// Convert 2 bytes in ASCII into one character
static unsigned char ascii_to_char(char b1, char b2)
{
	unsigned char ucRes;

	ucRes = (atoc(b1)) << 4 | (atoc(b2));

	return ucRes;
}

// Various String Functions
static void str_cpy(char dest[], char src[])
{
	int i = 0;
	for(i = 0; src[i] != '\0'; i++)
		dest[i] = src[i];
	dest[i] = '\0';
}

static int str_cmp(char str1[], char str2[])
{
	int i = 0;
	while(1)
	{
		if(str1[i] != str2[i])
			return str1[i] - str2[i];
		if(str1[i] == '\0' || str2[i] == '\0')
			return 0;
		i++;
	}
}

static int str_len(char str[])
{
	int i;
	for(i = 0; str[i] != '\0'; i++);
	return i;
}

static void sub_str(char dest[], char src[], int offset, int len)
{
	int i;
	for(i = 0; i < len && src[offset + i] != '\0'; i++)
		dest[i] = src[i + offset];
	dest[i] = '\0';
}

*/
