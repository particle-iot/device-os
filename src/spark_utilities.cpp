#include "spark_utilities.h"
#include "socket.h"
#include "netapp.h"
#include "string.h"
#include <stdarg.h>

long sparkSocket;
sockaddr tSocketAddr;

timeval timeout;
fd_set readSet;

// Spark Messages
const char Device_Secret[] = "secret";
const char Device_Name[] = "sparkdemodevice";
const char Device_Ok[] = "OK ";
const char Device_Fail[] = "FAIL ";
const char Device_IWDGRST[] = "IWDGRST";
const char Device_CRLF[] = "\n";

const char Flash_Update[] = "update";
const char Flash_Begin[] = "flash begin";
const char Flash_End[] = "flash end";
const char Flash_NoFile[] = "flash no file";

const char API_Alive[] = "alive";
const char API_Who[] = "who";
const char API_HandleMessage[] = "USERFUNC ";
const char API_SendMessage[] = "CALLBACK ";
const char API_Update[] = "UPDATE";

char High_Dx[] = "HIGH D ";
char Low_Dx[] = "LOW D ";

char digits[] = "0123456789";

char recvBuff[SPARK_BUF_LEN];
int total_bytes_received = 0;

uint32_t chunkIndex;

void (*pHandleMessage)(void);
char msgBuff[SPARK_BUF_LEN];

int User_Var_Count;
int User_Func_Count;
int User_Event_Count;

struct User_Var_Lookup_Table_t
{
	void *userVar;
	char userVarKey[USER_VAR_KEY_LENGTH];
	Spark_Data_TypeDef userVarType;
	bool userVarSchedule;
	unsigned char token; //not sure we require this here
} User_Var_Lookup_Table[USER_VAR_MAX_COUNT];

struct User_Func_Lookup_Table_t
{
	int (*pUserFunc)(char *userArg);
	char userFuncKey[USER_FUNC_KEY_LENGTH];
	char userFuncArg[USER_FUNC_ARG_LENGTH];
	int userFuncRet;
	bool userFuncSchedule;
	unsigned char token; //not sure we require this here
} User_Func_Lookup_Table[USER_FUNC_MAX_COUNT];

struct User_Event_Lookup_Table_t
{
	char userEventName[USER_EVENT_NAME_LENGTH];
	char userEventResult[USER_EVENT_RESULT_LENGTH];
	bool userEventSchedule;
} User_Event_Lookup_Table[USER_EVENT_MAX_COUNT];

static void handle_message(void);
static int Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * cmdvalue);

static unsigned char uitoa(unsigned int cNum, char *cString);
static unsigned int atoui(char *cString);
static uint8_t atoc(char data);

/*
static uint16_t atoshort(char b1, char b2);
static unsigned char ascii_to_char(char b1, char b2);

static void str_cpy(char dest[], char src[]);
static int str_cmp(char str1[], char str2[]);
static int str_len(char str[]);
static void sub_str(char dest[], char src[], int offset, int len);
*/

SparkClass Spark;

void SparkClass::variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType)
{
	int i = 0;
	if(NULL != userVar && NULL != varKey)
	{
		if(User_Var_Count == USER_VAR_MAX_COUNT)
			return;

		for(i = 0; i < User_Var_Count; i++)
		{
			if(User_Var_Lookup_Table[i].userVar == userVar && (0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH)))
			{
				return;
			}
		}

		User_Var_Lookup_Table[User_Var_Count].userVar = userVar;
		User_Var_Lookup_Table[User_Var_Count].userVarType = userVarType;
		memset(User_Var_Lookup_Table[User_Var_Count].userVarKey, 0, USER_VAR_KEY_LENGTH);
		memcpy(User_Var_Lookup_Table[User_Var_Count].userVarKey, varKey, USER_VAR_KEY_LENGTH);
		User_Var_Lookup_Table[User_Var_Count].userVarSchedule = false;
		User_Var_Lookup_Table[User_Var_Count].token = 0;
		User_Var_Count++;
	}
}

void SparkClass::function(const char *funcKey, int (*pFunc)(char *paramString))
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
		User_Func_Lookup_Table[User_Func_Count].token = 0;
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
	if(SPARK_DEVICE_ACKED)
		return true;
	else
		return false;
}

int SparkClass::connect(void)
{
	return Spark_Connect();
}

int SparkClass::disconnect(void)
{
	return Spark_Disconnect();
}

int Spark_Connect(void)
{
	int retVal = 0;

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
	tSocketAddr.sa_data[3] = 235;	// Second Octet of destination IP
	tSocketAddr.sa_data[4] = 79; 	// Third Octet of destination IP
	tSocketAddr.sa_data[5] = 249;	// Fourth Octet of destination IP

	retVal = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));

	if (retVal < 0)
	{
		// Unable to connect
		return -1;
	}
	else
	{
		retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Secret, NULL, NULL);
	}

    return retVal;
}

int Spark_Disconnect(void)
{
    int retVal = 0;

    retVal = closesocket(sparkSocket);

    if(retVal == 0)
    	sparkSocket = 0xFFFFFFFF;

    return retVal;
}

// receive from socket until we either find a newline or fill the buffer
// called repeatedly from an interrupt handler, so DO NOT BLOCK
// returns: -1 on error, signifying socket disconnected
//          0 if we have not yet received a full line
//          the number of bytes received when we have received a full line
int receive_line()
{
	if (0 == total_bytes_received)
	{
		memset(recvBuff, 0, SPARK_BUF_LEN);
	}

    // reset the fd_set structure
    FD_ZERO(&readSet);
    FD_SET(sparkSocket, &readSet);

    int buffer_bytes_available = SPARK_BUF_LEN - 1 - total_bytes_received;
    char *newline = NULL;

    // tell select to timeout after 500 microseconds
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

	int num_fds_ready = select(sparkSocket+1, &readSet, NULL, NULL, &timeout);

	if (0 < num_fds_ready)
	{
		if (FD_ISSET(sparkSocket, &readSet))
		{
			char *buffer_ptr = recvBuff + total_bytes_received;

			int bytes_received_once = recv(sparkSocket, buffer_ptr, buffer_bytes_available, 0);

			if (0 > bytes_received_once)
				return bytes_received_once;

			total_bytes_received += bytes_received_once;
			newline = strchr(recvBuff, '\n');
		}
	}

    if (NULL == newline && 0 < buffer_bytes_available)
    {
    	return 0;
    }
    else
    {
    	int retVal = total_bytes_received;
    	total_bytes_received = 0;
    	return retVal;
    }
}

void process_chunk(void)
{
	uint32_t chunkBytesAvailable = 0;
	uint32_t chunkCRCValue = 0;
	uint32_t computedCRCValue = 0;
	char *chunkToken;

	chunkToken = strtok(&recvBuff[chunkIndex], "\n");
	if(chunkToken != NULL)
	{
		chunkCRCValue = atoui(chunkToken);
		chunkIndex = chunkIndex + strlen(chunkToken) + 1;//+1 for '\n'
	}

	chunkToken = strtok(NULL, "\n");
	if(chunkToken != NULL)
	{
		chunkBytesAvailable = atoui(chunkToken);
		chunkIndex = chunkIndex + strlen(chunkToken) + 1;//+1 for '\n'
	}

	if(chunkBytesAvailable)
	{
		char CRCStr[11];
		memset(CRCStr, 0, 11);
		computedCRCValue = Compute_CRC32((uint8_t *)&recvBuff[chunkIndex], chunkBytesAvailable);
		if(chunkCRCValue == computedCRCValue)
		{
			FLASH_Update((uint8_t *)&recvBuff[chunkIndex], chunkBytesAvailable);
		}
		uitoa(computedCRCValue, CRCStr);
		Spark_Send_Device_Message(sparkSocket, (char *)CRCStr, NULL, NULL);
	}
}

// process the contents of recvBuff
// returns number of bytes transmitted or -1 on error
int process_command()
{
	int bytes_sent = 0;

	// who
	if (0 == strncmp(recvBuff, API_Who, strlen(API_Who)))
	{
		bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Name, NULL, NULL);
	}

	// API alive signal received and acknowledged by core, reset alive timeout
	else if (0 == strncmp(recvBuff, API_Alive, strlen(API_Alive)))
	{
		if(!SPARK_DEVICE_ACKED)
		{
			SPARK_DEVICE_ACKED = 1;//First alive received by Core means Server received Device ID
		}
		TimingSparkAliveTimeout = 0;

		bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)API_Alive, NULL, NULL);
	}

	// command to trigger OTA firmware upgrade
	else if (0 == strncmp(recvBuff, API_Update, strlen(API_Update)))
	{
		bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Flash_Update, NULL, NULL);
	}

	// signal the MCU to reset and start the marvin application
	else if (0 == strncmp(recvBuff, Flash_NoFile, strlen(Flash_NoFile)))
	{
		//FLASH_End();
	}

	// command to start download and flashing the firmware
	else if (0 == strncmp(recvBuff, Flash_Begin, strlen(Flash_Begin)))
	{
		SPARK_FLASH_UPDATE = 1;
		chunkIndex = strlen(Flash_Begin) + 1;//+1 for '\n'
		FLASH_Begin(EXTERNAL_FLASH_OTA_ADDRESS);
	}

	// command to end the flashing process and reset the MCU
	else if (0 == strncmp(recvBuff, Flash_End, strlen(Flash_End)))
	{
		SPARK_FLASH_UPDATE = 0;
		FLASH_End();
	}

	if(SPARK_FLASH_UPDATE)
	{
		TimingSparkAliveTimeout = 0;
		process_chunk();
		chunkIndex = 0;
	}

	// command to set a pin high
	else if (0 == strncmp(recvBuff, High_Dx, 6))
	{
		High_Dx[6] = recvBuff[6];

		if (OK == DIO_SetState((DIO_TypeDef)atoc(High_Dx[6]), HIGH))
			bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)High_Dx, NULL);
		else
			bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)High_Dx, NULL);
	}

	// command to set a pin low
	else if (0 == strncmp(recvBuff, Low_Dx, 5))
	{
		Low_Dx[5] = recvBuff[5];

		if (OK == DIO_SetState((DIO_TypeDef)atoc(Low_Dx[5]), LOW))
			bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)Low_Dx, NULL);
		else
			bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)Low_Dx, NULL);
	}

	// command to call the user-defined function
	else if (0 == strncmp(recvBuff, API_HandleMessage, strlen(API_HandleMessage)))
	{
		char *msg_arg = &recvBuff[strlen(API_HandleMessage)];
		char *newline = strchr(msg_arg, '\n');
		if (NULL != newline)
		{
			if ('\r' == *(newline - 1))
				newline--;
			*newline = '\0';
		}

	    memset(msgBuff, 0, SPARK_BUF_LEN);
	    if(NULL != msg_arg)
	    {
	    	memcpy(msgBuff, msg_arg, strlen(msg_arg));
	    }
	    pHandleMessage = handle_message;
	}

/*
//	else
//	{
//		bytes_sent = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)recvBuff, NULL);
//	}
*/

	return bytes_sent;
}

int Spark_Process_API_Response(void)
{
	int retVal = receive_line();

	if (0 < retVal)
		retVal = process_command();

	return retVal;
}

bool userVarSchedule(const char *varKey, unsigned char token)
{
	int i = 0;
	for(i = 0; i < User_Var_Count; i++)
	{
		if(0 == strncmp(User_Var_Lookup_Table[i].userVarKey, varKey, USER_VAR_KEY_LENGTH))
		{
			User_Var_Lookup_Table[i].userVarSchedule = true;
			User_Var_Lookup_Table[i].token = token;
			return true;
		}
	}
	return false;
}

void userVarReturn(void)
{
	int i = 0;
	for(i = 0; i < User_Var_Count; i++)
	{
		if(true == User_Var_Lookup_Table[i].userVarSchedule)
		{
			User_Var_Lookup_Table[i].userVarSchedule = false;

			//Send the "Variable value" back to the server here OR in a separate thread
			if(User_Var_Lookup_Table[i].token)
			{
/*
				bool boolVal;
				int intVal;
				char *stringVal;
				double doubleVal;
*/

				unsigned char buf[16];
				memset(buf, 0, 16);

				switch(User_Var_Lookup_Table[i].userVarType)
				{
				case BOOLEAN:
/*
					boolVal = *((bool*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, boolVal);
*/
					break;

				case INT:
/*
					intVal = *((int*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, intVal);
*/
					break;

				case STRING:
/*
					stringVal = ((char*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, stringVal, strlen(stringVal));
*/
					break;

				case DOUBLE:
/*
					doubleVal = *((double*)User_Var_Lookup_Table[i].userVar);
					//spark_protocol.variable_value(buf, User_Func_Lookup_Table[i].token, doubleVal);
*/
					break;
				}

				User_Var_Lookup_Table[i].token = 0;
			}
		}
	}
}

bool userFuncSchedule(const char *funcKey, unsigned char token, const char *paramString)
{
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
			User_Func_Lookup_Table[i].token = token;
			return true;
		}
	}
	return false;
}

void userFuncExecute(void)
{
	int i = 0;
	for(i = 0; i < User_Func_Count; i++)
	{
		if(true == User_Func_Lookup_Table[i].userFuncSchedule)
		{
			User_Func_Lookup_Table[i].userFuncSchedule = false;
			User_Func_Lookup_Table[i].userFuncRet = User_Func_Lookup_Table[i].pUserFunc(User_Func_Lookup_Table[i].userFuncArg);
/*
			//Send the "Function Return" back to the server here OR in a separate thread
			if(User_Func_Lookup_Table[i].token)
			{
				unsigned char buf[16];
				memset(buf, 0, 16);
				spark_protocol.function_return(buf, User_Func_Lookup_Table[i].token, User_Func_Lookup_Table[i].userFuncRet);
				User_Func_Lookup_Table[i].token = 0;
			}
*/
		}
	}
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

void sendMessage(char *message)
{
	Spark_Send_Device_Message(sparkSocket, (char *)API_SendMessage, (char *)message, NULL);
}

//void sendMessageWithData(char *message, char *data, long size)
//{
//	char lenStr[11];
//	unsigned char len = uitoa(size, &lenStr[0]);
//	lenStr[len] = '\0';
//	Spark_Send_Device_Message(sparkSocket, (char *)API_SendMessage, (char *)message, (char *)lenStr);
//}

static void handle_message(void)
{
	if (NULL != handleMessage)
	{
		pHandleMessage = NULL;
		char retStr[11];
		int msgResult = handleMessage(msgBuff);
		unsigned char retLen = uitoa(msgResult, retStr);
		retStr[retLen] = '\0';
		Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)API_HandleMessage, (char *)retStr);
	}
}

// returns number of bytes transmitted or -1 on error
static int Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * cmdvalue)
{
    char cmdBuf[SPARK_BUF_LEN];
    int sendLen = 0;
    int retVal = 0;

    memset(cmdBuf, 0, SPARK_BUF_LEN);

    if(cmd != NULL)
    {
        sendLen = strlen(cmd);
        memcpy(cmdBuf, cmd, strlen(cmd));
    }

    if(cmdparam != NULL)
    {
        memcpy(&cmdBuf[sendLen], cmdparam, strlen(cmdparam));
        sendLen += strlen(cmdparam);
    }

    if(cmdvalue != NULL)
    {
        memcpy(&cmdBuf[sendLen], cmdvalue, strlen(cmdvalue));
        sendLen += strlen(cmdvalue);
    }

    memcpy(&cmdBuf[sendLen], Device_CRLF, strlen(Device_CRLF));
    sendLen += strlen(Device_CRLF);

    retVal = send(socket, cmdBuf, sendLen, 0);

    return retVal;
}

// Convert unsigned integer to ASCII in decimal base
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
