/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "main.h"

#define SPARK_BUF_LEN				600

//#define BYTE_N(x,n)					(((x) >> n*8) & 0x000000FF)

//#define SPARK_SERVER_IP				"54.235.79.249"
#define SPARK_SERVER_PORT			5683

#define USER_VAR_MAX_COUNT			10
#define USER_VAR_KEY_LENGTH			12

#define USER_FUNC_MAX_COUNT			4
#define USER_FUNC_KEY_LENGTH		12
#define USER_FUNC_ARG_LENGTH		64

#define USER_EVENT_MAX_COUNT		3
#define USER_EVENT_NAME_LENGTH		16
#define USER_EVENT_RESULT_LENGTH	64

typedef enum
{
	SLEEP_MODE_WLAN = 0, SLEEP_MODE_DEEP = 1
} Spark_Sleep_TypeDef;

typedef enum
{
	BOOLEAN = 1, INT = 2, STRING = 4, DOUBLE = 9
} Spark_Data_TypeDef;

class SparkClass {
public:
	static void variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType);
	static void function(const char *funcKey, int (*pFunc)(char *paramString));
	static void event(const char *eventName, char *eventResult);
	static void sleep(Spark_Sleep_TypeDef sleepMode, long seconds);
	static void sleep(long seconds);
	static bool connected(void);
	static int connect(void);
	static int disconnect(void);
};

extern SparkClass Spark;

#ifdef __cplusplus
extern "C" {
#endif

int Spark_Connect(void);
int Spark_Disconnect(void);
int Spark_Process_API_Response(void);

void Spark_Protocol_Init(void);
int Spark_Handshake(void);
bool Spark_Communication_Loop(void);

bool userVarSchedule(const char *varKey, unsigned char token);
void userVarReturn(void);

int userFuncSchedule(const char *funcKey, const char *paramString);
void userFuncExecute(void);

void userEventSend(void);

void sendMessage(char *message);
//void sendMessageWithData(char *message, char *data, long size);

//void handleMessage(void) __attribute__ ((weak, alias ("Default_Handler")));
char handleMessage(char *user_arg) __attribute__ ((weak));
void setup() __attribute__ ((weak));
void loop() __attribute__ ((weak));

extern void (*pHandleMessage)(void);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif  /* __SPARK_UTILITIES_H */
