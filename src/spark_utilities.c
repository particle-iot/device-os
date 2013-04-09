#include "spark_utilities.h"
#include "socket.h"
#include "string.h"

long sparkSocket;
sockaddr tSocketAddr;

__IO uint8_t SPARK_SERVER_FLAG;

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
		Spark_Send_Device_Message(sparkSocket, (char *)core_secret, NULL, buf);
		if(buf[0] == server_ready[0] && buf[1] == server_ready[1] && buf[2] == server_ready[2])
		{
			Spark_Send_Device_Message(sparkSocket, (char *)core_id, NULL, buf);
			if(buf[0] == core_id[0] && buf[1] == core_id[1] && buf[2] == core_id[2]
			   && buf[3] == core_id[3] && buf[4] == core_id[4] && buf[5] == core_id[5])
		    {
		    	SPARK_SERVER_FLAG = 1;
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
    SPARK_SERVER_FLAG = 0;
}

void Spark_Send_Device_Message(long socket, char * cmd, char * cmdparam, char * respBuf)
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

    //memcpy(&cmdBuf[sendLen], spark_crlf, strlen(spark_crlf));
    //sendLen += strlen(spark_crlf);

    send(socket, cmdBuf, sendLen, 0);

    if(respBuf != NULL)
    {
        memset(respBuf, 0, SPARK_BUF_LEN);
        recv(socket, respBuf, SPARK_BUF_LEN, 0);
        recv(sparkSocket, &tmp, 1, 0); //Receive and Discard '\n'
    }
}

void Spark_Process_API_Response()
{
    char recvBuff[SPARK_BUF_LEN], tmp;
    int recvError = 0;

    memset(recvBuff, 0, SPARK_BUF_LEN);

    recvError = recv(sparkSocket, recvBuff, SPARK_BUF_LEN, 0);

    if(recvError <= 1)
    	return;

	if(recvBuff[0] == spark_high[0] && recvBuff[1] == spark_high[1] && recvBuff[2] == spark_high[2] && recvBuff[3] == spark_high[3])
	{
		if(recvBuff[5] == spark_dx[0])
		{
			spark_dx[1] = recvBuff[6];
			DIO_SetState(atoc(spark_dx[1]), HIGH);
			//To Do - Send Success Response back to Server
		}
	}
	else if(recvBuff[0] == spark_low[0] && recvBuff[1] == spark_low[1] && recvBuff[2] == spark_low[2])
	{
		if(recvBuff[4] == spark_dx[0])
		{
			spark_dx[1] = recvBuff[5];
			DIO_SetState(atoc(spark_dx[1]), LOW);
			//To Do - Send Success Response back to Server
		}
	}
	else
	{
		//To Do - Send Failure Response back to Server
	}
}

/**
  * @brief  Convert nibble to hexdecimal from ASCII.
  * @param  None
  * @retval None
  */
uint8_t atoc(char data)
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
uint16_t atoshort(char b1, char b2)
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
unsigned char ascii_to_char(char b1, char b2)
{
	unsigned char ucRes;

	ucRes = (atoc(b1)) << 4 | (atoc(b2));

	return ucRes;
}

