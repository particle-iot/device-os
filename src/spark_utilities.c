#include "spark_utilities.h"
#include "socket.h"
#include "string.h"

long sparkSocket;
sockaddr tSocketAddr;
unsigned char spark_state;

int sparkServerPort = 0;
unsigned long sparkServerIP = 0;
signed char sparkServerFlag = 0;

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
unsigned char
ascii_to_char(char b1, char b2)
{
	unsigned char ucRes;

	ucRes = (atoc(b1)) << 4 | (atoc(b2));

	return ucRes;
}

char Spark_Connect(char * hname, int port)
{
    sparkSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sparkSocket == -1)
    {
        //wlan_stop();
        return -1;
    }

    sparkServerPort = port;

    //gethostbyname is blocking

	// the family is always AF_INET
    tSocketAddr.sa_family = AF_INET;	//2;

	// the destination port
    tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
    tSocketAddr.sa_data[1] = (port & 0x00FF);

/*
    if(gethostbyname(hname, strlen(hname), &sparkServerIP) > 0)
    {
    	// the destination IP address
    	// correcting the endianess
        tSocketAddr.sa_data[5] = BYTE_N(sparkServerIP, 0);	// First Octet of destination IP
        tSocketAddr.sa_data[4] = BYTE_N(sparkServerIP, 1);	// Second Octet of destination IP
        tSocketAddr.sa_data[3] = BYTE_N(sparkServerIP, 2);	// Third Octet of destination IP
        tSocketAddr.sa_data[2] = BYTE_N(sparkServerIP, 3);	// Fourth Octet of destination IP

        sparkServerFlag = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));

        if (sparkServerFlag < 0)
        {
            // Unable to connect
            return -1;
        }
    }
    else
*/
    {
    	// the destination IP address
        // Error with DNS. Use default IP of Server
        tSocketAddr.sa_data[2] = 54;	// First Octet of destination IP
        tSocketAddr.sa_data[3] = 235;	// Second Octet of destination IP
        tSocketAddr.sa_data[4] = 79;	// Third Octet of destination IP
        tSocketAddr.sa_data[5] = 249;	// Fourth Octet of destination IP

        sparkServerFlag = connect(sparkSocket, &tSocketAddr, sizeof(tSocketAddr));

        if (sparkServerFlag < 0)
        {
            // Unable to connect
            return -1;
        }
        else
        {
            sparkServerFlag = 1;
        }
    }

    // Success
    return 0;
}

void Spark_Disconnect(void)
{
    closesocket(sparkSocket);
    sparkSocket = 0xFFFFFFFF;
}

