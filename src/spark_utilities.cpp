/**
 ******************************************************************************
 * @file    spark_utilities.cpp
 * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
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
#include "socket.h"
#include "netapp.h"
#include "string.h"
#include <stdarg.h>
#include "spark_protocol.h"

SparkProtocol spark_protocol;

long sparkSocket;
sockaddr tSocketAddr;

timeval timeout;
_types_fd_set_cc3000 readSet;

char digits[] = "0123456789";

int total_bytes_received = 0;

uint32_t chunkIndex;

extern unsigned int millis();
extern uint8_t LED_RGB_BRIGHTNESS;

// LED_Signaling_Override
__IO uint8_t LED_Spark_Signal;
__IO uint32_t LED_Signaling_Timing;
const uint32_t VIBGYOR_Colors[] = {
  0xEE82EE, 0x4B0082, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFFA500, 0xFF0000};
int VIBGYOR_Size = sizeof(VIBGYOR_Colors) / sizeof(uint32_t);
int VIBGYOR_Index;

int User_Var_Count;
int User_Func_Count;
int User_Event_Count;

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

struct User_Event_Lookup_Table_t
{
	char userEventName[USER_EVENT_NAME_LENGTH];
	char userEventResult[USER_EVENT_RESULT_LENGTH];
	bool userEventSchedule;
} User_Event_Lookup_Table[USER_EVENT_MAX_COUNT];


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

RGBClass RGB;
SparkClass Spark;

bool RGBClass::_control = false;

bool RGBClass::controlled(void)
{
	return _control;
}

void RGBClass::control(bool override)
{
#if !defined (RGB_NOTIFICATIONS_ON)
	if (override)
		LED_Signaling_Start();
	else
		LED_Signaling_Stop();

	_control = override;
#endif
}

void RGBClass::color(int red, int green, int blue)
{
#if !defined (RGB_NOTIFICATIONS_ON)
	if (true != _control)
		return;

	TIM1->CCR2 = (uint16_t)((red   * LED_RGB_BRIGHTNESS * (TIM1->ARR + 1)) >> 16);	// Red LED
	TIM1->CCR3 = (uint16_t)((green * LED_RGB_BRIGHTNESS * (TIM1->ARR + 1)) >> 16);	// Green LED
	TIM1->CCR1 = (uint16_t)((blue  * LED_RGB_BRIGHTNESS * (TIM1->ARR + 1)) >> 16);	// Blue LED
#endif
}

void RGBClass::brightness(uint8_t brightness)
{
#if !defined (RGB_NOTIFICATIONS_ON)
  LED_SetBrightness(brightness);
#endif
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

void SparkClass::event(const char *eventName, char *eventResult)
{
	int i = 0;
	if(NULL != eventName && NULL != eventResult)
	{
		if(User_Event_Count == USER_EVENT_MAX_COUNT)
			return;

		size_t resultLength = strlen(eventResult);
		if(resultLength > USER_EVENT_RESULT_LENGTH)
			resultLength = USER_EVENT_RESULT_LENGTH;

		for(i = 0; i < User_Event_Count; i++)
		{
			if(0 == strncmp(User_Event_Lookup_Table[i].userEventName, eventName, USER_EVENT_NAME_LENGTH))
			{
				memcpy(User_Event_Lookup_Table[i].userEventResult, eventResult, resultLength);
				User_Event_Lookup_Table[i].userEventSchedule = true;
				return;
			}
		}

		memset(User_Event_Lookup_Table[User_Event_Count].userEventName, 0, USER_EVENT_NAME_LENGTH);
		memset(User_Event_Lookup_Table[User_Event_Count].userEventResult, 0, USER_EVENT_RESULT_LENGTH);
		memcpy(User_Event_Lookup_Table[User_Event_Count].userEventName, eventName, USER_EVENT_NAME_LENGTH);
		memcpy(User_Event_Lookup_Table[User_Event_Count].userEventResult, eventResult, resultLength);
		User_Event_Lookup_Table[User_Event_Count].userEventSchedule = true;
		User_Event_Count++;
	}
}

void SparkClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds)
{
#if defined (SPARK_RTC_ENABLE)
	/* Set the RTC Alarm */
	RTC_SetAlarm(RTC_GetCounter() + (uint32_t)seconds);

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForLastTask();

	switch(sleepMode)
	{
	case SLEEP_MODE_WLAN:
		SPARK_WLAN_SLEEP = 1;
		break;

	case SLEEP_MODE_DEEP:
		Enter_STANDBY_Mode();
		break;
	}
#endif
}

void SparkClass::sleep(long seconds)
{
	SparkClass::sleep(SLEEP_MODE_WLAN, seconds);
}

bool SparkClass::connected(void)
{
	if(SPARK_SOCKET_CONNECTED && SPARK_HANDSHAKE_COMPLETED)
		return true;
	else
		return false;
}

int SparkClass::connect(void)
{
	//Schedule Spark's cloud connection and handshake
	SPARK_SOCKET_HANDSHAKE = 1;
	return 0;
}

int SparkClass::disconnect(void)
{
	//Schedule Spark's cloud disconnection
	SPARK_SOCKET_HANDSHAKE = 0;
	return 0;
}

String SparkClass::deviceID(void)
{
	String deviceID;
	char hex_digit;
	char id[12];
	memcpy(id, (char *)ID1, 12);
	//OR
	//uint8_t id[12];
	//Get_Unique_Device_ID(id);

	for (int i = 0; i < 12; ++i)
	{
		hex_digit = 48 + (id[i] >> 4);
		if (57 < hex_digit)
		{
			hex_digit += 39;
		}
		deviceID.concat(hex_digit);

		hex_digit = 48 + (id[i] & 0xf);
		if (57 < hex_digit)
		{
			hex_digit += 39;
		}
		deviceID.concat(hex_digit);
	}

	return deviceID;
}

// Returns number of bytes sent or -1 if an error occurred
int Spark_Send(const unsigned char *buf, int buflen)
{
  return send(sparkSocket, buf, buflen, 0);
}

// Returns number of bytes received or -1 if an error occurred
int Spark_Receive(unsigned char *buf, int buflen)
{
  if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP)
  {
    //break from any blocking loop
    return -1;
  }

  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sparkSocket, &readSet);

  // tell select to timeout after the minimum 5000 microseconds
  // defined in the SimpleLink API as SELECT_TIMEOUT_MIN_MICRO_SECONDS
  timeout.tv_sec = 0;
  timeout.tv_usec = 5000;

  int bytes_received = 0;
  int num_fds_ready = select(sparkSocket + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sparkSocket, &readSet))
    {
      // recv returns negative numbers on error
      bytes_received = recv(sparkSocket, buf, buflen, 0);
      TimingSparkCommTimeout = 0;
    }
  }
  else if (0 > num_fds_ready)
  {
    // error from select
    return num_fds_ready;
  }

  return bytes_received;
}

void Spark_Prepare_For_Firmware_Update(void)
{
  SPARK_FLASH_UPDATE = 1;
  FLASH_Begin(EXTERNAL_FLASH_OTA_ADDRESS);
}

void Spark_Finish_Firmware_Update(void)
{
  SPARK_FLASH_UPDATE = 0;
  FLASH_End();
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
    callbacks.prepare_for_firmware_update = Spark_Prepare_For_Firmware_Update;
    callbacks.finish_firmware_update = Spark_Finish_Firmware_Update;
    callbacks.calculate_crc = Compute_CRC32;
    callbacks.save_firmware_chunk = FLASH_Update;
    callbacks.signal = Spark_Signal;
    callbacks.millis = millis;

    SparkDescriptor descriptor;
    descriptor.num_functions = numUserFunctions;
    descriptor.copy_function_key = copyUserFunctionKey;
    descriptor.call_function = userFuncSchedule;
    descriptor.num_variables = numUserVariables;
    descriptor.copy_variable_key = copyUserVariableKey;
    descriptor.variable_type = wrapVarTypeInEnum;
    descriptor.get_variable = getUserVar;
    descriptor.was_ota_upgrade_successful = OTA_Flashed_GetStatus;
    descriptor.ota_upgrade_status_sent = OTA_Flashed_ResetStatus;

    unsigned char pubkey[EXTERNAL_FLASH_SERVER_PUBLIC_KEY_LENGTH];
    unsigned char private_key[EXTERNAL_FLASH_CORE_PRIVATE_KEY_LENGTH];

    SparkKeys keys;
    keys.server_public = pubkey;
    keys.core_private = private_key;

    FLASH_Read_ServerPublicKey(pubkey);
    FLASH_Read_CorePrivateKey(private_key);

    spark_protocol.init((const char *)ID1, keys, callbacks, descriptor);
  }
}

int Spark_Handshake(void)
{
  Spark_Protocol_Init();
  spark_protocol.reset_updating();
  int err = spark_protocol.handshake();
  Multicast_Presence_Announcement();
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
  long multicast_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 > multicast_socket)
    return;

  unsigned char announcement[19];
  spark_protocol.presence_announcement(announcement, (const char *)ID1);

  // create multicast address 224.0.1.187 port 5683
  sockaddr addr;
  addr.sa_family = AF_INET;
  addr.sa_data[0] = 0x16; // port MSB
  addr.sa_data[1] = 0x33; // port LSB
  addr.sa_data[2] = 0xe0; // IP MSB
  addr.sa_data[3] = 0x00;
  addr.sa_data[4] = 0x01;
  addr.sa_data[5] = 0xbb; // IP LSB

  for (int i = 3; i > 0; i--)
  {
    sendto(multicast_socket, announcement, 19, 0, &addr, sizeof(sockaddr));
  }

  closesocket(multicast_socket);
}

/* This function MUST NOT BlOCK!
 * It will be executed every 1ms if LED_Signaling_Start() is called
 * and stopped as soon as LED_Signaling_Stop() is called */
void LED_Signaling_Override(void)
{
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

    testSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

	testResult = connect(testSocket, &testSocketAddr, sizeof(testSocketAddr));

	closesocket(testSocket);

	//if connection fails, testResult returns -1
    return testResult;
}

int Spark_Connect(void)
{
  Spark_Disconnect();

  sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sparkSocket < 0)
  {
    return -1;
  }

  // the family is always AF_INET
  tSocketAddr.sa_family = AF_INET;

  // the destination port
  tSocketAddr.sa_data[0] = (SPARK_SERVER_PORT & 0xFF00) >> 8;
  tSocketAddr.sa_data[1] = (SPARK_SERVER_PORT & 0x00FF);

  // the destination IP address
  tSocketAddr.sa_data[2] = 54;	// First Octet of destination IP
  tSocketAddr.sa_data[3] = 208;	// Second Octet of destination IP
  tSocketAddr.sa_data[4] = 229; 	// Third Octet of destination IP
  tSocketAddr.sa_data[5] = 4;	// Fourth Octet of destination IP

  return connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));
}

int Spark_Disconnect(void)
{
  int retVal = closesocket(sparkSocket);

  if(retVal == 0)
    sparkSocket = 0xFFFFFFFF;

  return retVal;
}

void Spark_ConnectAbort_WLANReset(void)
{
	//Work around to exit the blocking nature of socket calls
	tSLInformation.usEventOrDataReceived = 1;
	tSLInformation.usRxEventOpcode = 0;
	tSLInformation.usRxDataPending = 0;

	SPARK_WLAN_RESET = 1;
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

void userEventSend(void)
{
	int i = 0;
	for(i = 0; i < User_Event_Count; i++)
	{
		if(true == User_Event_Lookup_Table[i].userEventSchedule)
		{
			User_Event_Lookup_Table[i].userEventSchedule = false;
/*
			//Send the "Event" back to the server here OR in a separate thread
			unsigned char buf[256];
			memset(buf, 0, 256);
			spark_protocol.event(buf, User_Event_Lookup_Table[i].userEventName, strlen(User_Event_Lookup_Table[i].userEventName), User_Event_Lookup_Table[i].userEventResult, strlen(User_Event_Lookup_Table[i].userEventResult));
*/
		}
	}
}

long socket_connect(long sd, const sockaddr *addr, long addrlen)
{
	return connect(sd, addr, addrlen);
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
