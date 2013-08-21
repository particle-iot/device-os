/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "main.h"

#define SPARK_BUF_LEN	256

//#define BYTE_N(x,n) (((x) >> n*8) & 0x000000FF)

//#define SPARK_SERVER_IP	"54.235.79.249"
#define SPARK_SERVER_PORT	8989

#define TIMING_SPARK_IWDG_RELOAD		1000	//1sec
#define TIMING_SPARK_PROCESS_API		200		//200ms
#define TIMING_SPARK_ALIVE_TIMEOUT		15000	//15sec
#define TIMING_SPARK_RESET_TIMEOUT		30000	//30sec

#define SOCKET_CONNECT_MAX_ATTEMPT		3		//Max no of connection attempts

#define USER_FUNC_MAX_COUNT				10
#define USER_FUNC_KEY_LENGTH			50

typedef struct Spark_Namespace {
	void (*variable)(void *);
	void (*function)(void (*pFunc)(void), const char *);
	void (*event)(char *, char *);
	void (*sleep)(int);
	bool (*connected)(void);
	int (*connect)(void);
	int (*disconnect)(void);
} Spark_Namespace;

void Spark_Variable(void *userVar);
void Spark_Function(void (*pFunc)(void), const char *funcKey);
bool Spark_User_Func_Invoke(char *funcKey);
void Spark_User_Func_Execute(void);
void Spark_Event(char *eventName, char *eventResult);
void Spark_Sleep(int millis);
bool Spark_Connected(void);
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
