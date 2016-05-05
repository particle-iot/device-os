/**
  ******************************************************************************
  * @file    sst25vf_spi.c
  * @author  Satish Nair, Zachary Crockett and Mohit Bhoite
  * @version V1.0.0
  * @date    15-May-2013
  * @brief   This file provides a set of functions needed to manage the
  *          SPI SST25xxxFLASH memory mounted on Spark boards
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sst25vf_spi.h"


/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"

/* SST25 SPI Flash supported commands */
#define sFLASH_CMD_RDSR					0x05		/* Read Status Register */
#define sFLASH_CMD_WRSR					0x01		/* Write Status Register */
#define sFLASH_CMD_EWSR					0x50		/* Write Enable Status */
#define sFLASH_CMD_WRDI					0x04		/* Write Disable */
#define sFLASH_CMD_WREN					0x06		/* Write Enable */
#define sFLASH_CMD_READ					0x03		/* Read Data Bytes */
#define sFLASH_CMD_WRITE 				0x02		/* Byte Program */
#define sFLASH_CMD_AAIP                 0xAD		/* Auto Address Increment */
#define sFLASH_CMD_SE             		0x20		/* 4KB Sector Erase instruction */
#define sFLASH_CMD_BE             		0xC7		/* Bulk Chip Erase instruction */
#define sFLASH_CMD_RDID            		0x9F		/* JEDEC ID Read */
#define sFLASH_CMD_EBSY                 0x70		/* Enable SO RY/BY# Status */
#define sFLASH_CMD_DBSY                 0x80		/* Disable SO RY/BY# Status */

#define sFLASH_WIP_FLAG           		0x01		/* Write In Progress (WIP) flag */

#define sFLASH_DUMMY_BYTE         		0xFF

#define sFLASH_SST25VF040_ID			0xBF258D	/* JEDEC Read-ID Data */
#define sFLASH_SST25VF016_ID			0xBF2541	/* JEDEC Read-ID Data */


/* Local function forward declarations ---------------------------------------*/
static void sFLASH_WriteByte(uint32_t WriteAddr, uint8_t byte);
static void sFLASH_WriteBytes(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
static void sFLASH_WriteEnable(void);
static void sFLASH_WriteDisable(void);
static void sFLASH_WaitForWriteEnd(void);
static uint8_t sFLASH_SendByte(uint8_t byte);

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
  * @note   Addresses to be written must be in the erased state
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  byte: the data to be written.
  * @retval None
  */
static void sFLASH_WriteByte(uint32_t WriteAddr, uint8_t byte)
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
  * @note   The address must be even and the number of bytes must be a multiple
  *         of two.
  * @note   Addresses to be written must be in the erased state
  * @param  pBuffer: pointer to the buffer containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to, must be even.
  * @param  NumByteToWrite: number of bytes to write to the FLASH, must be even.
  * @retval None
  */
static void sFLASH_WriteBytes(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
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
  * @note   Addresses to be written must be in the erased state
  * @param  pBuffer: pointer to the buffer  containing the data to be written
  *         to the FLASH.
  * @param  WriteAddr: FLASH's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the FLASH.
  * @retval None
  */
void sFLASH_WriteBuffer(const uint8_t *pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
  uint32_t evenBytes;

  /* If write starts at an odd address, need to use single byte write
   * to write the first address. */
  if ((WriteAddr & 0x1) == 0x1)
  {
    sFLASH_WriteByte(WriteAddr, *pBuffer++);
    ++WriteAddr;
    --NumByteToWrite;
  }

  /* Write bulk of bytes using auto increment write, with restriction
   * that address must always be even and two bytes are written at a time. */
  evenBytes = NumByteToWrite & ~0x1;
  if (evenBytes)
  {
    sFLASH_WriteBytes(pBuffer, WriteAddr, evenBytes);
    NumByteToWrite -= evenBytes;
  }

  /* If number of bytes to write is odd, need to use a single byte write
   * to write the last address. */
  if (NumByteToWrite)
  {
    pBuffer += evenBytes;
    WriteAddr += evenBytes;
    sFLASH_WriteByte(WriteAddr, *pBuffer++);
  }
}

/**
  * @brief  Reads a block of data from the FLASH.
  * @param  pBuffer: pointer to the buffer that receives the data read from the FLASH.
  * @param  ReadAddr: FLASH's internal address to read from.
  * @param  NumByteToRead: number of bytes to read from the FLASH.
  * @retval None
  */
void sFLASH_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
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
  uint8_t byte[3];

  /* Select the FLASH: Chip Select low */
  sFLASH_CS_LOW();

  /* Send "JEDEC ID Read" instruction */
  sFLASH_SendByte(sFLASH_CMD_RDID);

  /* Read a byte from the FLASH */
  byte[0] = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Read a byte from the FLASH */
  byte[1] = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Read a byte from the FLASH */
  byte[2] = sFLASH_SendByte(sFLASH_DUMMY_BYTE);

  /* Deselect the FLASH: Chip Select high */
  sFLASH_CS_HIGH();

  return (byte[0] << 16) | (byte[1] << 8) | byte[2];
}

/**
  * @brief  Sends a byte through the SPI interface and return the byte received
  *         from the SPI bus.
  * @param  byte: byte to send.
  * @retval The value of the received byte.
  */
static uint8_t sFLASH_SendByte(uint8_t byte)
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
static void sFLASH_WriteEnable(void)
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
static void sFLASH_WriteDisable(void)
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
static void sFLASH_WaitForWriteEnd(void)
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
