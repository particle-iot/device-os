#include "spark_utilities.h"
#include "socket.h"
#include "netapp.h"
#include "string.h"

long sparkSocket;
sockaddr tSocketAddr;

timeval timeout;
fd_set readSet;

extern uint8_t SPARK_SERVER_CONNECTED;

static void Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * respBuf);
static uint8_t atoc(char data);
static uint16_t atoshort(char b1, char b2);
static unsigned char ascii_to_char(char b1, char b2);

char Spark_Connect(void)
{
	int serverFlag = 0;
	char buf[SPARK_BUF_LEN];

    sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sparkSocket == -1)
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

	serverFlag = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));

	if (serverFlag < 0)
	{
		// Unable to connect
		return -1;
	}
	else
	{
		Spark_Send_Device_Message(sparkSocket, (char *)Device_Secret, NULL, buf);

		if(buf[0] == API_Who[0] && buf[1] == API_Who[1] && buf[2] == API_Who[2])
		{
			Spark_Send_Device_Message(sparkSocket, (char *)Device_Name, NULL, buf);

			//if(buf[0] == Device_Name[0] && buf[1] == Device_Name[1] && buf[2] == Device_Name[2]
			//   && buf[3] == Device_Name[3] && buf[4] == Device_Name[4] && buf[5] == Device_Name[5])
		    {
		    	SPARK_SERVER_CONNECTED = 1;
		    	LED_On(LED1);
		    }
		}
	}

    // Success
    return 0;
}

void Spark_Disconnect(void)
{
    closesocket(sparkSocket);
    sparkSocket = 0xFFFFFFFF;
    SPARK_SERVER_CONNECTED = 0;
}

void Spark_Process_API_Response()
{
    char recvBuff[SPARK_BUF_LEN];
    int recvError = 0;

    memset(recvBuff, 0, SPARK_BUF_LEN);

    //select will block for 500 microseconds
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;

    //reset the fd_set structure
    FD_ZERO(&readSet);
    FD_SET(sparkSocket, &readSet);

    //poll the client for receive events
    select(sparkSocket+1, &readSet, NULL, NULL, &timeout);

    //if data is available then receive
    if(FD_ISSET(sparkSocket, &readSet))
    {
    	recvError = recv(sparkSocket, recvBuff, SPARK_BUF_LEN, 0);
    }

    if(recvError <= 1)
    	return;

	if(recvBuff[0] == High_Dx[0] && recvBuff[1] == High_Dx[1] && recvBuff[2] == High_Dx[2] && recvBuff[3] == High_Dx[3])
	{
		if(recvBuff[5] == High_Dx[5])
		{
			High_Dx[6] = recvBuff[6];

			if(DIO_SetState(atoc(High_Dx[6]), HIGH) == OK)
				Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)High_Dx, NULL);
			else
				Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)High_Dx, NULL);
		}
	}
	else if(recvBuff[0] == Low_Dx[0] && recvBuff[1] == Low_Dx[1] && recvBuff[2] == Low_Dx[2])
	{
		if(recvBuff[4] == Low_Dx[4])
		{
			Low_Dx[5] = recvBuff[5];

			if(DIO_SetState(atoc(Low_Dx[5]), LOW) == OK)
				Spark_Send_Device_Message(sparkSocket, (char *)Device_Ok, (char *)Low_Dx, NULL);
			else
				Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)Low_Dx, NULL);
		}
	}
	else
	{
		Spark_Send_Device_Message(sparkSocket, (char *)Device_Fail, (char *)recvBuff, NULL);
	}
}

static void Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * respBuf)
{
    char cmdBuf[SPARK_BUF_LEN], tmp;
    int sendLen = 0;

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

    //memcpy(&cmdBuf[sendLen], Device_CRLF, strlen(Device_CRLF));
    //sendLen += strlen(Device_CRLF);

    send(socket, cmdBuf, sendLen, 0);

    if(respBuf != NULL)
    {
        memset(respBuf, 0, SPARK_BUF_LEN);
        recv(socket, respBuf, SPARK_BUF_LEN, 0);
        recv(sparkSocket, &tmp, 1, 0); //Receive and Discard '\n'
    }
}

/**
  * @brief  Convert nibble to hexdecimal from ASCII.
  * @param  None
  * @retval None
  */
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

/**
  * @brief  converts 2 nibbles into a 16-bit number
  * @param  char1
  * @param  char2
  * @retval number
  */
static uint16_t atoshort(char b1, char b2)
{
	uint16_t usRes;
	usRes = (atoc(b1)) * 16 | atoc(b2);
	return usRes;
}

/**
  * @brief  Converts 2 bytes into an ASCII character
  * @param  None
  * @retval None
  */
static unsigned char ascii_to_char(char b1, char b2)
{
	unsigned char ucRes;

	ucRes = (atoc(b1)) << 4 | (atoc(b2));

	return ucRes;
}

