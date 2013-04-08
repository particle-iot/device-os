#include "hw_config.h"

#define BYTE_N(x,n) (((x) >> n*8) & 0x000000FF)

uint8_t atoc(char data);
uint16_t atoshort(char b1, char b2);
unsigned char ascii_to_char(char b1, char b2);

char Spark_Connect(char * hname, int port);
void Spark_Disconnect(void);
