
#ifndef SYSTEM_YMODEM_H
#define	SYSTEM_YMODEM_H

#ifdef __cplusplus
extern "C" {
#endif    

typedef struct Stream Stream;
#include <stdint.h>

bool Ymodem_Serial_Flash_Update(Stream *serialObj, uint32_t sFlashAddress);

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_YMODEM_H */

