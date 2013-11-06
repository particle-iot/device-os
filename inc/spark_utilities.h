/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "main.h"
#include "spark_wiring_string.h"

#define SPARK_BUF_LEN				600

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
	static void function(const char *funcKey, int (*pFunc)(String paramString));
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
void Spark_ConnectAbort_WLANReset(void);

void Spark_Protocol_Init(void);
int Spark_Handshake(void);
bool Spark_Communication_Loop(void);
void Multicast_Presence_Announcement(void);
void Spark_Signal(bool on);

void *getUserVar(const char *varKey);
int userFuncSchedule(const char *funcKey, const char *paramString);

void userEventSend(void);

void setup() __attribute__ ((weak));
void loop() __attribute__ ((weak));

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif  /* __SPARK_UTILITIES_H */
