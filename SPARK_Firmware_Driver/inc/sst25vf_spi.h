/**
  ******************************************************************************
  * @file    sst25vf_spi.h
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    15-May-2013
  * @brief   This file contains all the functions prototypes for the
  *          sst25vf_spi Flash firmware driver.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SST25VF_SPI_H
#define __SST25VF_SPI_H

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
#define sFLASH_PAGESIZE					0x1000		/* 4096 bytes */

#define sFLASH_SST25VF040_ID			0xBF258D	/* JEDEC Read-ID Data */
#define sFLASH_SST25VF016_ID			0xBF2541	/* JEDEC Read-ID Data */

/* High layer functions */
void sFLASH_Init(void);
void sFLASH_EraseSector(uint32_t SectorAddr);
void sFLASH_EraseBulk(void);
void sFLASH_WriteByte(uint32_t WriteAddr, uint8_t byte);
void sFLASH_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void sFLASH_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void sFLASH_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t sFLASH_ReadID(void);

/* Low layer functions */
uint8_t sFLASH_SendByte(uint8_t byte);
void sFLASH_WriteEnable(void);
void sFLASH_WriteDisable(void);
void sFLASH_WaitForWriteEnd(void);

/* Flash Self Test Routine */
int sFLASH_SelfTest(void);

extern void Delay(__IO uint32_t nTime);

#endif /* __SST25VF_SPI_H */
