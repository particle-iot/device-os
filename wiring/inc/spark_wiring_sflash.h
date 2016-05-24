
#ifndef __SPARK_WIRING_SFLASH_H_
#define __SPARK_WIRING_SFLASH_H_

#if PLATFORM_ID == 88 // Duo


#include "spi_flash.h"


#define SFLASH_RESERVED_ADDRESS    0xC0000

class SerialFlash
{
public:
    SerialFlash(void);

    void eraseSector(uint32_t SectorAddr);
    void writeBuffer(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
    void readBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
    int selfTest(void);
};


extern SerialFlash sFLASH;

#endif

#endif
