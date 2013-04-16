#include "spark_utilities.h"
#include "socket.h"
#include "netapp.h"
#include "string.h"

long sparkSocket;
sockaddr tSocketAddr;

timeval timeout;
fd_set readSet;

// Spark Messages
const char Device_Secret[] = "secret";
const char Device_Name[] = "satish";
const char Device_Ok[] = "OK ";
const char Device_Fail[] = "FAIL ";
const char Device_CRLF[] = "\n";
const char API_Who[] = "who";
const char API_UserFunc[] = "USERFUNC ";
const char API_Callback[] = "CALLBACK ";
char High_Dx[] = "HIGH D ";
char Low_Dx[] = "LOW D ";

char digits[] = "0123456789";

static int Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * cmdvalue);
static unsigned char itoa(int cNum, char *cString);
static uint8_t atoc(char data);

/*
static uint16_t atoshort(char b1, char b2);
static unsigned char ascii_to_char(char b1, char b2);

static void str_cpy(char dest[], char src[]);
static int str_cmp(char str1[], char str2[]);
static int str_len(char str[]);
static void sub_str(char dest[], char src[], int offset, int len);
*/

int Spark_Connect(void)
{
	int retVal = 0;

    sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sparkSocket < 0)
    {
        //wlan_stop();
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
	tSocketAddr.sa_data[4] = 79;	// Third Octet of destination IP
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

int Spark_Process_API_Response(void)
{
    char recvBuff[SPARK_BUF_LEN];
    int retVal = 0;

    memset(recvBuff, 0, SPARK_BUF_LEN);

    //select will block for 500 microseconds
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

    //reset the fd_set structure
    FD_ZERO(&readSet);
    FD_SET(sparkSocket, &readSet);

    //poll the client for receive events
    retVal = select(sparkSocket+1, &readSet, NULL, NULL, &timeout);

    //if data is available then receive
    if(FD_ISSET(sparkSocket, &readSet))
    {
    	retVal = recv(sparkSocket, recvBuff, SPARK_BUF_LEN, 0);
    }

    if(retVal < 0)
    	return retVal;

	//if(recvBuff[0] == API_Who[0] && recvBuff[1] == API_Who[1] && recvBuff[2] == API_Who[2])
	if(strncmp(recvBuff, API_Who, strlen(API_Who)) == 0)
	{
		retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Name, NULL, NULL);
	}
	//else if(recvBuff[0] == Device_Name[0] && recvBuff[1] == Device_Name[1] && recvBuff[2] == Device_Name[2]
	//   && recvBuff[3] == Device_Name[3] && recvBuff[4] == Device_Name[4] && recvBuff[5] == Device_Name[5])
	else if(strncmp(recvBuff, Device_Name, strlen(Device_Name)) == 0)
    {
    	LED_On(LED1);
    	return 0;
    }
	//else if(recvBuff[0] == High_Dx[0] && recvBuff[1] == High_Dx[1] && recvBuff[2] == High_Dx[2]
	//	&& recvBuff[3] == High_Dx[3] && recvBuff[4] == High_Dx[4] && recvBuff[5] == High_Dx[5])
	else if(strncmp(recvBuff, High_Dx, 6) == 0)
	{
		High_Dx[6] = recvBuff[6];

		if(DIO_SetState(atoc(High_Dx[6]), HIGH) == OK)
			retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)High_Dx, NULL);
		else
			retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)High_Dx, NULL);
	}
	//else if(recvBuff[0] == Low_Dx[0] && recvBuff[1] == Low_Dx[1] && recvBuff[2] == Low_Dx[2]
	//	&& recvBuff[3] == Low_Dx[3] && recvBuff[4] == Low_Dx[4])
	else if(strncmp(recvBuff, Low_Dx, 5) == 0)
	{
		Low_Dx[5] = recvBuff[5];

		if(DIO_SetState(atoc(Low_Dx[5]), LOW) == OK)
			retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)Low_Dx, NULL);
		else
			retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)Low_Dx, NULL);
	}
	else if(strncmp(recvBuff, API_UserFunc, 9) == 0)
	{
		if(userFunction != NULL)
		{
			char *user_arg = strchr(&recvBuff[9], '\n');

			if(strlen(user_arg) > 22)
			{
				retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, NULL, NULL);
			}
			else
			{
				char retCh, retLen, retStr[11];
				retCh = userFunction(user_arg);
				retLen = itoa(retCh, &retStr[0]);
				retStr[retLen] = '\0';
				retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)API_UserFunc, (char *)retStr);
			}
		}
	}
	else if((strncmp(recvBuff, Device_Ok, 3) == 0) && (strncmp(&recvBuff[3], API_Callback, 9) == 0))
	{
		char *status_code = strchr(&recvBuff[12], '\n');

		if(strcmp(status_code, "200") == 0)
		{
			//To Do
		}
		else if(strcmp(status_code, "404") == 0)
		{
			//To Do
		}
	}
	else if(strcmp(recvBuff, Device_CRLF) == 0)
	{
		return 0; //Do nothing for new line returned
	}
	else
	{
		retVal = Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)recvBuff, NULL);
	}

	return retVal;
}

void userCallback(char *callback_name)
{
	Spark_Send_Device_Message(sparkSocket, (char *)API_Callback, (char *)callback_name, NULL);
}

void userCallbackWithData(char *callback_name, char *callback_data, long data_length)
{
	char len, lenStr[11];
	len = itoa(data_length, &lenStr[0]);
	lenStr[len] = '\0';
	Spark_Send_Device_Message(sparkSocket, (char *)API_Callback, (char *)callback_name, (char *)lenStr);
}

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

// brief  Convert integer to ASCII in decimal base
static unsigned char itoa(int cNum, char *cString)
{
    char* ptr;
    int uTemp = cNum;
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

//Convert nibble to hexdecimal from ASCII
static uint8_t atoc(char data)
{
	unsigned char ucRes;

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
