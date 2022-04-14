/**
 ******************************************************************************
 * @file    spi_flash.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Dec-2014
 * @brief   spi flash driver
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
#include "hw_config.h"

/* This was originally optimized for O2 to make sFlash work reliably
 * without knowing why it worked exactly. It was determined that a delay was needed after the
 * sFLASH_CS_LOW(); in sFLASH_WriteDisable/sFLASH_WriteEnable, so an asm("mov r2, r2"); was added
 * to the end of sFLASH_CS_LOW();
 */
// #pragma GCC optimize ("O1")

/* SST25 SPI Flash supported commands */
#define sFLASH_CMD_RDSR                 0x05        /* Read Status Register */
#define sFLASH_CMD_WRSR                 0x01        /* Write Status Register */
#define sFLASH_CMD_EWSR                 0x50        /* Write Enable Status */
#define sFLASH_CMD_WRDI                 0x04        /* Write Disable */
#define sFLASH_CMD_WREN                 0x06        /* Write Enable */
#define sFLASH_CMD_READ                 0x03        /* Read Data Bytes */
#define sFLASH_CMD_WRITE                0x02        /* Byte Program */
#define sFLASH_CMD_SE                   0x20        /* 4KB Sector Erase instruction */
#define sFLASH_CMD_BE                   0xC7        /* Bulk Chip Erase instruction */
#define sFLASH_CMD_RDID                 0x9F        /* JEDEC ID Read */

#define sFLASH_WIP_FLAG                 0x01        /* Write In Progress (WIP) flag */

#define sFLASH_DUMMY_BYTE               0xA5
#define sFLASH_PROGRAM_PAGESIZE         0x100       /* 256 bytes */

#define sFLASH_MX25L8006E_ID            0xC22014    /* JEDEC Read-ID Data */

/* Local function forward declarations ---------------------------------------*/
static void sFLASH_WritePage(const uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);
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
    /* Initializes the peripherals used by the SPI FLASH driver */
    sFLASH_SPI_Init();

    /* Disable the write access to the FLASH */
    sFLASH_WriteDisable();

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
 * @brief  Writes more than one byte to the FLASH with a single WRITE cycle
 *         (Page WRITE sequence).
 * @note   The number of byte can't exceed the FLASH page size.
 * @param  pBuffer: pointer to the buffer  containing the data to be written
 *         to the FLASH.
 * @param  WriteAddr: FLASH's internal address to write to.
 * @param  NumByteToWrite: number of bytes to write to the FLASH, must be equal
 *         or less than "sFLASH_PROGRAM_PAGESIZE" value.
 * @retval None
 */
static void sFLASH_WritePage(const uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    /* Enable the write access to the FLASH */
    sFLASH_WriteEnable();

    /* Select the FLASH: Chip Select low */
    sFLASH_CS_LOW();
    /* Send "Write to Memory " instruction */
    sFLASH_SendByte(sFLASH_CMD_WRITE);
    /* Send WriteAddr high nibble address byte to write to */
    sFLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
    /* Send WriteAddr medium nibble address byte to write to */
    sFLASH_SendByte((WriteAddr & 0xFF00) >> 8);
    /* Send WriteAddr low nibble address byte to write to */
    sFLASH_SendByte(WriteAddr & 0xFF);

    /* while there is data to be written on the FLASH */
    while (NumByteToWrite)
    {
        /* Send the current byte and point to the next location */
        sFLASH_SendByte(*pBuffer++);
        /* Decrement NumByteToWrite */
        NumByteToWrite--;
    }

    /* Deselect the FLASH: Chip Select high */
    sFLASH_CS_HIGH();

    /* Wait the end of Flash writing */
    sFLASH_WaitForWriteEnd();
}

/**
 * @brief  Writes block of data to the FLASH. In this function, the number of
 *         WRITE cycles are reduced, using Page WRITE sequence.
 * @note   Addresses to be written must be in the erased state
 * @param  pBuffer: pointer to the buffer  containing the data to be written
 *         to the FLASH.
 * @param  WriteAddr: FLASH's internal address to write to.
 * @param  NumByteToWrite: number of bytes to write to the FLASH.
 * @retval None
 */
void sFLASH_WriteBuffer(const uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint16_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    Addr = WriteAddr % sFLASH_PROGRAM_PAGESIZE;
    count = sFLASH_PROGRAM_PAGESIZE - Addr;
    NumOfPage =  NumByteToWrite / sFLASH_PROGRAM_PAGESIZE;
    NumOfSingle = NumByteToWrite % sFLASH_PROGRAM_PAGESIZE;

    if (Addr == 0) /* WriteAddr is sFLASH_PROGRAM_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /* NumByteToWrite < sFLASH_PROGRAM_PAGESIZE */
        {
            sFLASH_WritePage(pBuffer, WriteAddr, NumByteToWrite);
        }
        else /* NumByteToWrite > sFLASH_PROGRAM_PAGESIZE */
        {
            while (NumOfPage--)
            {
                sFLASH_WritePage(pBuffer, WriteAddr, sFLASH_PROGRAM_PAGESIZE);
                WriteAddr +=  sFLASH_PROGRAM_PAGESIZE;
                pBuffer += sFLASH_PROGRAM_PAGESIZE;
            }

            sFLASH_WritePage(pBuffer, WriteAddr, NumOfSingle);
        }
    }
    else /* WriteAddr is not sFLASH_PROGRAM_PAGESIZE aligned  */
    {
        if (NumOfPage == 0) /* NumByteToWrite < sFLASH_PROGRAM_PAGESIZE */
        {
            if (NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > sFLASH_PROGRAM_PAGESIZE */
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
        else /* NumByteToWrite > sFLASH_PROGRAM_PAGESIZE */
        {
            NumByteToWrite -= count;
            NumOfPage =  NumByteToWrite / sFLASH_PROGRAM_PAGESIZE;
            NumOfSingle = NumByteToWrite % sFLASH_PROGRAM_PAGESIZE;

            sFLASH_WritePage(pBuffer, WriteAddr, count);
            WriteAddr +=  count;
            pBuffer += count;

            while (NumOfPage--)
            {
                sFLASH_WritePage(pBuffer, WriteAddr, sFLASH_PROGRAM_PAGESIZE);
                WriteAddr +=  sFLASH_PROGRAM_PAGESIZE;
                pBuffer += sFLASH_PROGRAM_PAGESIZE;
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
static uint8_t __attribute__((optimize("O3"))) /* saves 4 bytes over Os */ sFLASH_SendByte(uint8_t byte)
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
    if(FlashID == sFLASH_MX25L8006E_ID)
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
