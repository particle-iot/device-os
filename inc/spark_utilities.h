/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "main.h"

#define SPARK_BUF_LEN	256

//#define BYTE_N(x,n) (((x) >> n*8) & 0x000000FF)

//#define SPARK_SERVER_IP	"54.235.79.249"
#define SPARK_SERVER_PORT	8989

#define TIMING_SPARK_PROCESS_API		200		//200ms
#define TIMING_SPARK_ALIVE_TIMEOUT		15000	//15sec
#define TIMING_SPARK_RESET_TIMEOUT		30000	//30sec

#define SOCKET_CONNECT_MAX_ATTEMPT		3		//Max no of connection attempts

int Spark_Connect(void);
int Spark_Disconnect(void);
int Spark_Process_API_Response(void);

void sendMessage(char *message);
//void sendMessageWithData(char *message, char *data, long size);

//void handleMessage(void) __attribute__ ((weak, alias ("Default_Handler")));
char handleMessage(char *user_arg) __attribute__ ((weak));
void setup() __attribute__ ((weak));
void loop() __attribute__ ((weak));

extern void (*pHandleMessage)(void);

#endif  /* __SPARK_UTILITIES_H */
