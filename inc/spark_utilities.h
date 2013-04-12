/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPARK_UTILITIES_H
#define __SPARK_UTILITIES_H

#include "hw_config.h"

#define SPARK_BUF_LEN	32

//#define BYTE_N(x,n) (((x) >> n*8) & 0x000000FF)

//#define SPARK_SERVER_IP	"54.235.79.249"
#define SPARK_SERVER_PORT	8989

// end-of-line character
//const char Device_CRLF[] = "\r\n";

// Spark Messages
const char Device_Secret[] = {'s','e','c','r','e','t','\n'};	//"secret"
const char Device_Name[] = {'s','a','t','i','s','h','\n'};		//"satish"
const char Device_Ok[] = {'O','K',' '};							//"OK "
const char Device_Fail[] = {'F','A','I','L',' '};				//"FAIL "
const char API_Who[] = {'w','h','o'};							//"who"
char High_Dx[] = {'H','I','G','H',' ','D',' ','\n'};			//"HIGH D "
char Low_Dx[] = {'L','O','W',' ','D',' ','\n'};					//"LOW D "

char Spark_Connect(void);
void Spark_Disconnect(void);
void Spark_Process_API_Response();

#endif  /* __SPARK_UTILITIES_H */
