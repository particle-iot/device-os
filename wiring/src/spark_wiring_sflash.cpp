
#if PLATFORM_ID == 88 // Duo

#include "spark_wiring_sflash.h"

SerialFlash::SerialFlash(void)
{
    sFLASH_Init();
}

void SerialFlash::eraseSector(uint32_t SectorAddr)
{
    if(SectorAddr >= SFLASH_RESERVED_ADDRESS) return;

    sFLASH_EraseSector(SectorAddr);
}

void SerialFlash::writeBuffer(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    if(WriteAddr >= SFLASH_RESERVED_ADDRESS) return;

    if((WriteAddr + NumByteToWrite) > SFLASH_RESERVED_ADDRESS)
    {
        NumByteToWrite = SFLASH_RESERVED_ADDRESS - WriteAddr;
    }

    sFLASH_WriteBuffer(pBuffer, WriteAddr, NumByteToWrite);
}

void SerialFlash::readBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
{
    if(ReadAddr >= SFLASH_RESERVED_ADDRESS) return;

    if((ReadAddr + NumByteToRead) > SFLASH_RESERVED_ADDRESS)
    {
    	NumByteToRead = SFLASH_RESERVED_ADDRESS - ReadAddr;
    }

    sFLASH_ReadBuffer(pBuffer, ReadAddr, NumByteToRead);
}

int SerialFlash::selfTest(void)
{
    return sFLASH_SelfTest();
}


SerialFlash sFLASH;

#endif
