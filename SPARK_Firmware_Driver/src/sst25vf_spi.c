/**
  ******************************************************************************
  * @file    sst25vf_spi.c
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    15-May-2013
  * @brief   This file provides a set of functions needed to manage the
  *          SPI SST25xxxFLASH memory mounted on Spark boards
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sst25vf_spi.h"

/**
  * @brief Initializes SPI Flash
  * @param void
  * @return void
  */
void sFLASH_Init(void)
{
  uint32_t Device_ID = 0;

  /* Initializes the peripherals used by the SPI FLASH driver */
  sFLASH_SPI_Init();

  /* Disable the write access to the FLASH */
  sFLASH_WriteDisable();

  /* Read FLASH identification */
  Device_ID = sFLASH_ReadID();

  if(Device_ID == sFLASH_SST25VF040_ID || Device_ID == sFLASH_SST25VF016_ID)
  {
    /* Select the FLASH: Chip Select low */
    sFLASH_CS_LOW();
    /* Send "Disable SO RY/BY# Status" instruction */
    sFLASH_SendByte(sFLASH_CMD_DBSY);
    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();

    /* Select the FLASH: Chip Select low */
    sFLASH_CS_LOW();
    /* Send "Write Enable Status" instruction */
    sFLASH_SendByte(sFLASH_CMD_EWSR);
    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();

    /* Select the FLASH: Chip Select low */
    sFLASH_CS_LOW();
    /* Send "Write Status Register" instruction */
    sFLASH_SendByte(sFLASH_CMD_WRSR);
    sFLASH_SendByte(0);
    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();
  }
}

/**
  * @brief  Erases the specified FLASH sector.
  * @param  SectorAddr: address of the sector to erase.
  * @retval None
  */
void sFLASH_EraseSector(uint32_t SectorAddr)
{
  /* Enable the write access to the FLASH */
  sFLASH_WriteEnable();

  /* Sector Erase */
  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();
  /* Send Sector Erase instruction */
  sFLASH_SendByte(sFLASH_CMD_SE);
  /* Send SectorAddr high nibble address byte */
  sFLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
  /* Send SectorAddr medium nibble address byte */
  sFLASH_SendByte((SectorAddr & 0xFF00) >> 8);
  /* Send SectorAddr low nibble address byte */
  sFLASH_SendByte(SectorAddr & 0xFF);
  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();

  /* Wait till the end of Flash writing */
  sFLASH_WaitForWriteEnd();
}

/**
  * @brief  Erases the entire FLASH.
  * @param  None
  * @retval None
  */
void sFLASH_EraseBulk(void)
{
  /* Enable the write access to the FLASH */
  sFLASH_WriteEnable();

  /* Bulk Erase */
  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();
  /* Send Bulk Erase instruction  */
  sFLASH_SendByte(sFLASH_CMD_BE);
  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();

  /* Wait till the end of Flash writing */
  sFLASH_WaitForWriteEnd();
}

/**
  * @brief  Write one byte to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  byte: the data to be written.
  * @retval None
  */
void sFLASH_WriteByte(uint32_t WriteAddr, uint8_t byte)
{
  /* Enable the write access to the FLASH */
  sFLASH_WriteEnable();

  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();
  /* Send "Byte Program" instruction */
  sFLASH_SendByte(sFLASH_CMD_WRITE);
  /* Send WriteAddr high nibble address byte to write to */
  sFLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
  /* Send WriteAddr medium nibble address byte to write to */
  sFLASH_SendByte((WriteAddr & 0xFF00) >> 8);
  /* Send WriteAddr low nibble address byte to write to */
  sFLASH_SendByte(WriteAddr & 0xFF);
  /* Send the byte */
  sFLASH_SendByte(byte);
  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
  /* Wait till the end of Flash writing */
  sFLASH_WaitForWriteEnd();
}

/**
  * @brief  Writes more than one byte to the FLASH.
  * @note   The number of bytes can't exceed the FLASH page size.
  * @param  pBuffer: pointer to the buffer containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the FLASH, must be even,
  *         equal or less than "sFLASH_PAGESIZE" value.
  * @retval None
  */
void sFLASH_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
  /* Enable the write access to the FLASH */
  sFLASH_WriteEnable();

  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();
  /* Send "Auto Address Increment Word-Program" instruction */
  sFLASH_SendByte(sFLASH_CMD_AAIP);
  /* Send WriteAddr high nibble address byte to write to */
  sFLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
  /* Send WriteAddr medium nibble address byte to write to */
  sFLASH_SendByte((WriteAddr & 0xFF00) >> 8);
  /* Send WriteAddr low nibble address byte to write to */
  sFLASH_SendByte(WriteAddr & 0xFF);
  /* Send the first byte */
  sFLASH_SendByte(*pBuffer++);
  /* Send the second byte */
  sFLASH_SendByte(*pBuffer++);
  /* Update NumByteToWrite */
  NumByteToWrite -= 2;
  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
  /* Wait till the end of Flash writing */
  sFLASH_WaitForWriteEnd();

  /* while there is data to be written on the FLASH */
  while (NumByteToWrite)
  {
	/* Select the FLASH: Chip Select low */
	sFLASH_CS_LOW();
	/* Send "Auto Address Increment Word-Program" instruction */
	sFLASH_SendByte(sFLASH_CMD_AAIP);
    /* Send the next byte and point on the next byte */
    sFLASH_SendByte(*pBuffer++);
    /* Send the next byte and point on the next byte */
    sFLASH_SendByte(*pBuffer++);
    /* Update NumByteToWrite */
    NumByteToWrite -= 2;
    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();
    /* Wait till the end of Flash writing */
    sFLASH_WaitForWriteEnd();
  }

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();

  /* Disable the write access to the FLASH */
  sFLASH_WriteDisable();
}

/**
  * @brief  Writes block of data to the FLASH.
  * @param  pBuffer: pointer to the buffer  containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the FLASH.
  * @retval None
  */
void sFLASH_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
  uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

  Addr = WriteAddr % sFLASH_PAGESIZE;
  count = sFLASH_PAGESIZE - Addr;
  NumOfPage =  NumByteToWrite / sFLASH_PAGESIZE;
  NumOfSingle = NumByteToWrite % sFLASH_PAGESIZE;

  if (Addr == 0) /* WriteAddr is sFLASH_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /* NumByteToWrite < sFLASH_PAGESIZE */
    {
      sFLASH_WritePage(pBuffer, WriteAddr, NumByteToWrite);
    }
    else /* NumByteToWrite > sFLASH_PAGESIZE */
    {
      while (NumOfPage--)
      {
        sFLASH_WritePage(pBuffer, WriteAddr, sFLASH_PAGESIZE);
        WriteAddr +=  sFLASH_PAGESIZE;
        pBuffer += sFLASH_PAGESIZE;
      }

      sFLASH_WritePage(pBuffer, WriteAddr, NumOfSingle);
    }
  }
  else /* WriteAddr is not sFLASH_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /* NumByteToWrite < sFLASH_PAGESIZE */
    {
      if (NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > sFLASH_PAGESIZE */
      {
        temp = NumOfSingle - count;

        sFLASH_WritePage(pBuffer, WriteAddr, count);
        WriteAddr +=  count;
        pBuffer += count;

        sFLASH_WritePage(pBuffer, WriteAddr, temp);
      }
      else
      {
        sFLASH_WritePage(pBuffer, WriteAddr, NumByteToWrite);
      }
    }
    else /* NumByteToWrite > sFLASH_PAGESIZE */
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / sFLASH_PAGESIZE;
      NumOfSingle = NumByteToWrite % sFLASH_PAGESIZE;

      sFLASH_WritePage(pBuffer, WriteAddr, count);
      WriteAddr +=  count;
      pBuffer += count;

      while (NumOfPage--)
      {
        sFLASH_WritePage(pBuffer, WriteAddr, sFLASH_PAGESIZE);
        WriteAddr +=  sFLASH_PAGESIZE;
        pBuffer += sFLASH_PAGESIZE;
      }

      if (NumOfSingle != 0)
      {
        sFLASH_WritePage(pBuffer, WriteAddr, NumOfSingle);
      }
    }
  }
}

/**
  * @brief  Reads a block of data from the FLASH.
  * @param  pBuffer: pointer to the buffer that receives the data read from the FLASH.
  * @param  ReadAddr: FLASH's internal address to read from.
  * @param  NumByteToRead: number of bytes to read from the FLASH.
  * @retval None
  */
void sFLASH_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "Read from Memory " instruction */
  sFLASH_SendByte(sFLASH_CMD_READ);

  /* Send ReadAddr high nibble address byte to read from */
  sFLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  sFLASH_SendByte((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte to read from */
  sFLASH_SendByte(ReadAddr & 0xFF);

  while (NumByteToRead) /* while there is data to be read */
  {
    /* Read a byte from the FLASH and point to the next location */
    *pBuffer++ = sFLASH_SendByte(sFLASH_DUMMY_BYTE);
    /* Decrement NumByteToRead */
    NumByteToRead--;
  }

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
}

/**
  * @brief  Reads FLASH identification.
  * @param  None
  * @retval FLASH identification
  */
uint32_t sFLASH_ReadID(void)
{
  uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;

  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "JEDEC ID Read" instruction */
  sFLASH_SendByte(sFLASH_CMD_RDID);

  /* Read a byte from the FLASH */
  Temp0 = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Read a byte from the FLASH */
  Temp1 = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Read a byte from the FLASH */
  Temp2 = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();

  Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;

  return Temp;
}


/**
  * @brief  Sends a byte through the SPI interface and return the byte received
  *         from the SPI bus.
  * @param  byte: byte to send.
  * @retval The value of the received byte.
  */
uint8_t sFLASH_SendByte(uint8_t byte)
{
  /* Loop while DR register in not empty */
  while (SPI_I2S_GetFlagStatus(sFLASH_SPI, SPI_I2S_FLAG_TXE) == RESET);

  /* Send byte through the SPI peripheral */
  SPI_I2S_SendData(sFLASH_SPI, byte);

  /* Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(sFLASH_SPI, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(sFLASH_SPI);
}

/**
  * @brief  Enables the write access to the FLASH.
  * @param  None
  * @retval None
  */
void sFLASH_WriteEnable(void)
{
  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "Write Enable" instruction */
  sFLASH_SendByte(sFLASH_CMD_WREN);

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
}

/**
  * @brief  Disables the write access to the FLASH.
  * @param  None
  * @retval None
  */
void sFLASH_WriteDisable(void)
{
  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "Write Disable" instruction */
  sFLASH_SendByte(sFLASH_CMD_WRDI);

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
}

/**
  * @brief  Polls the status of the Write In Progress (WIP) flag in the FLASH's
  *         status register and loop until write operation has completed.
  * @param  None
  * @retval None
  */
void sFLASH_WaitForWriteEnd(void)
{
  uint8_t flashstatus = 0;

  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "Read Status Register" instruction */
  sFLASH_SendByte(sFLASH_CMD_RDSR);

  /* Loop as long as the memory is busy with a write cycle */
  do
  {
    /* Send a dummy byte to generate the clock needed by the FLASH
    and put the value of the status register in FLASH_Status variable */
    flashstatus = sFLASH_SendByte(sFLASH_DUMMY_BYTE);
  }
  while ((flashstatus & sFLASH_WIP_FLAG) == SET); /* Write in progress */

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();
}

int sFLASH_SelfTest(void)
{
  uint32_t FLASH_TestAddress = 0x000000;
  //Note: Make sure BufferSize should be Even and not Zero
  uint8_t Tx_Buffer[] = "Test communication with SPI FLASH!";//BufferSize = 34
  uint32_t BufferSize = (sizeof(Tx_Buffer) / sizeof(*(Tx_Buffer))) - 1;
  uint8_t Rx_Buffer[BufferSize];
  uint8_t Index = 0;
  uint32_t FlashID = 0;
  uint8_t LEDToggle = 0;
  int TestStatus = -1;

  /* Get SPI Flash ID */
  FlashID = sFLASH_ReadID();

  /* Check the SPI Flash ID */
  if(FlashID == sFLASH_SST25VF040_ID || FlashID == sFLASH_SST25VF016_ID)
  {
    /* Perform a write in the Flash followed by a read of the written data */
    /* Erase SPI FLASH Sector to write on */
    sFLASH_EraseSector(FLASH_TestAddress);

    /* Write Tx_Buffer data to SPI FLASH memory */
    sFLASH_WriteBuffer(Tx_Buffer, FLASH_TestAddress, BufferSize);

    /* Read data from SPI FLASH memory */
    sFLASH_ReadBuffer(Rx_Buffer, FLASH_TestAddress, BufferSize);

    /* Check the correctness of written data */
    for (Index = 0; Index < BufferSize; Index++)
    {
      if (Tx_Buffer[Index] != Rx_Buffer[Index])
      {
        //FAILED : Transmitted and Received data by SPI are different
    	TestStatus = -1;
      }
      else
      {
        //PASSED : Transmitted and Received data by SPI are same
    	TestStatus = 0;
      }
    }

    /* Perform an erase in the Flash followed by a read of the written data */
    /* Erase SPI FLASH Sector to write on */
    sFLASH_EraseSector(FLASH_TestAddress);

    /* Read data from SPI FLASH memory */
    sFLASH_ReadBuffer(Rx_Buffer, FLASH_TestAddress, BufferSize);

    /* Check the correctness of erasing operation data */
    for (Index = 0; Index < BufferSize; Index++)
    {
      if (Rx_Buffer[Index] != 0xFF)
      {
        //FAILED : Specified sector part is not well erased
    	TestStatus = -1;
      }
      else
      {
        //PASSED : Specified sector part is erased
    	TestStatus = 0;
      }
    }
  }

  return TestStatus;
}
